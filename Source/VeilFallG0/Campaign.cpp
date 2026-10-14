#include "Campaign.h"
#include "Combat.h"
#include "Coop.h"
#include "Tactical.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/Crc.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformFileManager.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/TextBlock.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "UObject/ConstructorHelpers.h"
#if PLATFORM_WINDOWS
#include "Windows/WindowsHWrapper.h"
#else
#include <cstdio>
#endif

namespace {
const FName Actions[]={"hq.brief","edge.arrive","relay.clear","relay.power","relay.establish","convoy.recover","facility.enter","chamber.reveal","hq.debrief"};
bool AtomicReplace(const FString& From,const FString& To) {
#if PLATFORM_WINDOWS
 return !!::MoveFileExW(*From,*To,MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH);
#else
 return std::rename(TCHAR_TO_UTF8(*From),TCHAR_TO_UTF8(*To))==0;
#endif
}
bool DurableWrite(const FString& Path,const TArray<uint8>& Bytes) {
 TUniquePtr<IFileHandle> File(FPlatformFileManager::Get().GetPlatformFile().OpenWrite(*Path));
 return File && File->Write(Bytes.GetData(),Bytes.Num()) && File->Flush(true);
}
bool Decode(const FString& Path,FDeadSignalRecord& Record,FString& Error) {
 auto& Files=FPlatformFileManager::Get().GetPlatformFile();
 if(Files.FileSize(*Path)<16 || Files.FileSize(*Path)>16*1024*1024){Error="Missing or oversized save";return false;}
 TArray<uint8> Bytes;
 if(!FFileHelper::LoadFileToArray(Bytes,*Path)){Error="Cannot read save";return false;}
 FMemoryReader Reader(Bytes); uint32 Magic=0,Format=0,Length=0,CRC=0;
 Reader<<Magic<<Format<<Length<<CRC;
 if(Magic!=0x56465331 || Format!=1 || Length!=uint32(Bytes.Num()-16) || FCrc::MemCrc32(Bytes.GetData()+16,Length)!=CRC){Error="Save envelope/checksum invalid";return false;}
 TArray<uint8> Payload; Payload.Append(Bytes.GetData()+16,Length);
 auto* Save=Cast<UDeadSignalSave>(UGameplayStatics::LoadGameFromMemory(Payload));
 if(!Save){Error="Unknown save class";return false;}
 Record=Save->Record;
 if(!Record.Migrate()){Error="Unsupported schema";return false;}
 if(!Record.Validate()){Error="Inconsistent mission record";return false;}
 return true;
}
}
bool FDeadSignalRecord::Validate() const {
 if(SchemaVersion!=2 || MissionId!="operation.dead_signal" || NextObjective<0 || NextObjective>9 || ClueMask<0 || ClueMask>3 || BankedEvidence<0 || Revision<0) return false;
 const ERelayState Expected=NextObjective<3?ERelayState::Hostile:NextObjective==3?ERelayState::Cleared:NextObjective==4?ERelayState::PowerRestored:ERelayState::Established;
 if(ConsumedPickups.Num()>256||KilledEnemies.Num()>64||HostField.Num()>16||SupplyStash.Num()>256||RecoveryPacks.Num()>128)return false;
 TSet<FName> Pickups,Enemies;for(FName Id:ConsumedPickups){if(Id.IsNone()||Pickups.Contains(Id))return false;Pickups.Add(Id);}for(FName Id:KilledEnemies){if(Id.IsNone()||Enemies.Contains(Id))return false;Enemies.Add(Id);}
 TSet<FGuid> Items,Packs;auto ValidateItems=[&Items](const TArray<FFieldItem>& Entries){for(const auto& Entry:Entries){if(!Entry.Validate()||Items.Contains(Entry.InstanceId))return false;Items.Add(Entry.InstanceId);}return true;};
 if(!ValidateItems(HostField)||!ValidateItems(SupplyStash))return false;
 for(const auto& Pack:RecoveryPacks){if(!Pack.PackId.IsValid()||Packs.Contains(Pack.PackId)||Pack.Location.ContainsNaN()||Pack.Location.GetAbsMax()>2000000||Pack.Items.IsEmpty()||Pack.Items.Num()>16||!ValidateItems(Pack.Items))return false;Packs.Add(Pack.PackId);}
 return Outpost==Expected && BankedEvidence==(NextObjective==9?1:0);
}
bool FDeadSignalRecord::Migrate() {
 // Schema 0 had no optional clues; preserve its objective/outpost/stash fields.
 if(SchemaVersion==0){ClueMask=0;SchemaVersion=1;}
 if(SchemaVersion==1){ConsumedPickups.Reset();KilledEnemies.Reset();HostField.Reset();SupplyStash.Reset();RecoveryPacks.Reset();SchemaVersion=2;} return SchemaVersion==2;
}
bool FDeadSignalRecord::Apply(FName Action) {
 if(!Validate())return false;
 if(Action=="clue.utility" || Action=="clue.archive") {
  const int32 Bit=Action=="clue.utility"?1:2;
  if((ClueMask&Bit)!=0 || NextObjective<1 || NextObjective==9)return false;
  ClueMask|=Bit; ++Revision; return true;
 }
 if(NextObjective>=9 || Action!=Actions[NextObjective])return false;
 ++NextObjective;
 if(NextObjective==3) Outpost=ERelayState::Cleared;
 if(NextObjective==4) Outpost=ERelayState::PowerRestored;
 if(NextObjective==5) Outpost=ERelayState::Established;
 if(NextObjective==9) BankedEvidence=1;
 ++Revision; return true;
}
FString FDeadSignalRecord::Summary() const { return FString::Printf(TEXT("stage=%d outpost=%d clues=%d stash=%d revision=%d"),NextObjective,int32(Outpost),ClueMask,BankedEvidence,Revision); }

bool FDeadSignalStore::Read(const FString& Path,FDeadSignalRecord& Record,FString& Error) {
 if(Decode(Path,Record,Error))return true;
 const FString PrimaryError=Error;
 // A valid future-format record is not corruption; never roll it back and overwrite it.
 if(PrimaryError=="Unsupported schema" || PrimaryError=="Unknown save class")return false;
 if(Decode(Path+".bak",Record,Error)){Error="Recovered backup: "+PrimaryError;return true;}
 Error=PrimaryError+"; backup unavailable/invalid: "+Error;return false;
}
bool FDeadSignalStore::Write(const FString& Path,const FDeadSignalRecord& Record,FString& Error) {
 if(!Record.Validate()){Error="Refusing invalid record";return false;}
 auto& Files=FPlatformFileManager::Get().GetPlatformFile();
 FDeadSignalRecord Existing;FString ExistingError;
 if(Files.FileExists(*Path) && !Decode(Path,Existing,ExistingError) && (ExistingError=="Unsupported schema" || ExistingError=="Unknown save class")){Error="Protected primary: "+ExistingError;return false;}
 Files.CreateDirectoryTree(*FPaths::GetPath(Path));
 auto* Save=NewObject<UDeadSignalSave>(); Save->Record=Record; TArray<uint8> Payload;
 if(!UGameplayStatics::SaveGameToMemory(Save,Payload)){Error="Serialization failed";return false;}
 TArray<uint8> Bytes; FMemoryWriter Writer(Bytes);
 uint32 Magic=0x56465331,Format=1,Length=Payload.Num(),CRC=FCrc::MemCrc32(Payload.GetData(),Payload.Num());
 Writer<<Magic<<Format<<Length<<CRC; Writer.Serialize(Payload.GetData(),Payload.Num()); Writer.Close();
 const FString Temp=Path+".tmp";
 if(!DurableWrite(Temp,Bytes)){Error="Temporary durable write failed";return false;}
 FDeadSignalRecord Old; FString OldError;
 if(Decode(Path,Old,OldError)) {
  TArray<uint8> Previous;
  if(!FFileHelper::LoadFileToArray(Previous,*Path) || !DurableWrite(Path+".bak.tmp",Previous) || !AtomicReplace(Path+".bak.tmp",Path+".bak")){Error="Backup commit failed; primary retained";return false;}
 }
 if(!AtomicReplace(Temp,Path)){Error="Atomic commit failed; prior primary retained";return false;}
 Error.Reset();return true;
}
ADeadSignalState::ADeadSignalState() { bReplicates=true;bAlwaysRelevant=true; }
FString ADeadSignalState::SavePath() const {
 FString Root=FPaths::Combine(FPaths::ProjectSavedDir(),"SaveGames");
 // Diagnostic runs isolate their saves from the player's campaign.
 if(FParse::Param(FCommandLine::Get(),TEXT("G1Smoke"))||FParse::Param(FCommandLine::Get(),TEXT("G3Smoke"))||FParse::Param(FCommandLine::Get(),TEXT("G3Capture"))||FParse::Param(FCommandLine::Get(),TEXT("G4Smoke"))||FParse::Param(FCommandLine::Get(),TEXT("G4Capture")))FParse::Value(FCommandLine::Get(),TEXT("G1SaveRoot="),Root);
 return FPaths::ConvertRelativePathToFull(FPaths::Combine(Root,"DeadSignal.vfs"));
}
void ADeadSignalState::BeginPlay() {
 Super::BeginPlay();
 if(HasAuthority()) {
  OnRep_Record();
 }
}
void ADeadSignalState::OnRep_Record() { UE_LOG(LogTemp,Log,TEXT("G1_STATE authority=%d %s"),HasAuthority(),*Record.Summary()); }
void ADeadSignalState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(ADeadSignalState,Record); DOREPLIFETIME(ADeadSignalState,Status); }
bool ADeadSignalState::Apply(FName Action,APawn* User) {
 if(!HasAuthority() || !IsValid(User) || !User->GetController())return false;
 if(auto* Coop=Cast<ACoopState>(this); Coop && ((Action=="relay.clear"&&!Coop->EncounterCleared("relay"))||(Action=="convoy.recover"&&!Coop->EncounterCleared("convoy")))){Status="Continuity squad still active. Clear the encounter before claiming this objective.";return false;}
 if(Action=="relay.resupply") {
  if(Record.Outpost!=ERelayState::Established)return false;
  if(auto* Combat=User->FindComponentByClass<UVeilCombatComponent>()){if(!Combat->Restock())return false;Status="Relay supplies replenished.";}else Status="Relay established: spawn/resupply and radio intelligence online.";ForceNetUpdate();return true;
 }
 if(!Record.Apply(Action)){Status="Objective unavailable or already completed. Follow the current objective.";return false;}
 Status=Action.ToString();OnRep_Record();ForceNetUpdate();
 UE_LOG(LogTemp,Log,TEXT("G1_ACTION user=%s action=%s"),*User->GetName(),*Action.ToString());
 // Each accepted authored checkpoint is persisted, including quest clues.
 SaveCampaign();return true;
}
bool ADeadSignalState::SaveCampaign() {
 if(!HasAuthority())return false;
 if(bSaveBlocked){Status="SAVE PROTECTED: repair or preserve/rename the existing save before restarting. Session progress is not being saved.";return false;}
 FString Error;const bool OK=FDeadSignalStore::Write(SavePath(),Record,Error);
 Status=OK?"Host campaign saved":"SAVE FAILED: "+Error;
 UE_LOG(LogTemp,Log,TEXT("G1_SAVE success=%d %s path=%s error=%s"),OK,*Record.Summary(),*SavePath(),*Error);return OK;
}
bool ADeadSignalState::RestoreCampaign() {
 if(!HasAuthority())return false;
 FDeadSignalRecord Loaded;FString Error;const bool OK=FDeadSignalStore::Read(SavePath(),Loaded,Error);
 if(OK){Loaded.Revision=FMath::Max(Loaded.Revision,Record.Revision+1);Record=Loaded;bSaveBlocked=false;OnRep_Record();ForceNetUpdate();Status="Host campaign restored. "+Error;}
 else{bSaveBlocked=true;Status="Save protected from overwrite: "+Error;}
 UE_LOG(LogTemp,Log,TEXT("G1_LOAD success=%d %s error=%s"),OK,*Record.Summary(),*Error);return OK;
}
ADeadSignalNode::ADeadSignalNode() {
 bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickInterval=0.2f;
 Mesh=CreateDefaultSubobject<UStaticMeshComponent>("Mesh");SetRootComponent(Mesh);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));Mesh->SetStaticMesh(Cube.Object);Mesh->SetCollisionProfileName("BlockAll");
 Label=CreateDefaultSubobject<UTextRenderComponent>("Label");Label->SetupAttachment(Mesh);Label->SetRelativeLocation(FVector(-55,0,95));Label->SetRelativeRotation(FRotator(0,180,0));Label->SetWorldSize(22);
}
void ADeadSignalNode::BeginPlay() {Super::BeginPlay();Tick(0.f);}
void ADeadSignalNode::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);auto* State=GetWorld()->GetGameState<ADeadSignalState>();if(!State)return;
 if(SeenRevision==State->Record.Revision)return;SeenRevision=State->Record.Revision;
 bool Available=false,Done=false;
 for(int32 i=0;i<9;++i)if(Action==Actions[i]){Available=i==State->Record.NextObjective;Done=i<State->Record.NextObjective;}
 if(Action=="clue.utility" || Action=="clue.archive"){Done=(State->Record.ClueMask&(Action=="clue.utility"?1:2))!=0;Available=!Done && State->Record.NextObjective>=1 && State->Record.NextObjective<9;}
 if(Action=="relay.resupply")Available=State->Record.Outpost==ERelayState::Established;
 Label->SetText(FText::FromString((Done?"DONE: ":Available?"F: ":"LOCKED: ")+Prompt));
 Label->SetTextRenderColor(Done?FColor::Green:Available?FColor::Yellow:FColor(150,150,150));
 // Authored persistent establishment changes, evaluated equally on late join.
 if(Action=="relay.resident" || Action=="relay.radio") {
  const bool Established=State->Record.Outpost==ERelayState::Established;
  SetActorHiddenInGame(!Established);SetActorEnableCollision(false);Label->SetText(FText::FromString(Prompt));
 }
}
bool ADeadSignalNode::Activate_Implementation(APawn* User) {
 auto* State=GetWorld()->GetGameState<ADeadSignalState>();
 if(!State || !State->Apply(Action,User))return false;
 if(!EvidenceStatement.IsEmpty()){State->Status=EvidenceStatement+"\n"+State->Status;State->ForceNetUpdate();}
 return true;
}
void UDeadSignalOverlay::NativeOnInitialized() {
 Super::NativeOnInitialized();
 auto* Canvas=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Canvas;
 Readout=WidgetTree->ConstructWidget<UTextBlock>();Readout->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f,0.95f,0.8f)));Readout->SetAutoWrapText(true);
 auto Font=Readout->GetFont();Font.Size=18;Readout->SetFont(Font);
 auto* ReadoutSlot=Canvas->AddChildToCanvas(Readout);ReadoutSlot->SetPosition(FVector2D(20,20));ReadoutSlot->SetSize(FVector2D(1100,380));
 auto* Cross=WidgetTree->ConstructWidget<UTextBlock>();Cross->SetText(FText::FromString("+"));
 auto* CrossSlot=Canvas->AddChildToCanvas(Cross);CrossSlot->SetAnchors(FAnchors(0.5f));CrossSlot->SetAlignment(FVector2D(0.5f));CrossSlot->SetAutoSize(true);
 SetVisibility(ESlateVisibility::HitTestInvisible);
}
void UDeadSignalOverlay::NativeTick(const FGeometry& Geometry,float DeltaSeconds) {
 Super::NativeTick(Geometry,DeltaSeconds);auto* State=GetWorld()->GetGameState<ADeadSignalState>();if(!State || !Readout)return;
 FString Objective="Debrief complete. Host evidence banked.";
 for(TActorIterator<ADeadSignalNode> It(GetWorld());It;++It)if(It->Definition && It->Definition->Objectives.IsValidIndex(State->Record.NextObjective)){Objective=It->Definition->Objectives[State->Record.NextObjective];break;}
 FString Waypoint;
 if(State->Record.NextObjective<9)for(TActorIterator<ADeadSignalNode> It(GetWorld());It;++It)if(It->Action==Actions[State->Record.NextObjective])if(auto* User=GetOwningPlayerPawn()) {
  const FVector Delta=It->GetActorLocation()-User->GetActorLocation();
  const float Bearing=FMath::FindDeltaAngleDegrees(User->GetControlRotation().Yaw,Delta.Rotation().Yaw);
  Waypoint=FString::Printf(TEXT(" | %.0f m | turn %.0f deg %s"),Delta.Size2D()/100,FMath::Abs(Bearing),Bearing>=0?TEXT("right"):TEXT("left"));break;
 }
 FString CombatReadout; if(auto* CombatPawn=Cast<ACombatCharacter>(GetOwningPlayerPawn())){const auto* Ammo=CombatPawn->Combat->CurrentAmmo();const auto& CS=CombatPawn->Combat->State;CombatReadout=FString::Printf(TEXT("EQUIPMENT | HP %.0f | %s | %s armor | %s %d / %d | med %d ammo kit %d\nLMB fire / RMB aim / R reload / 1-3 slots / G medical / H ammo / F revive / Enter respawn\n%s%s%s\n"),CombatPawn->Health->State.Health,CombatPawn->Health->State.Life==EVeilLife::Alive?TEXT("Alive"):CombatPawn->Health->State.Life==EVeilLife::Downed?TEXT("Downed"):TEXT("Dead"),CombatPawn->Health->State.Armor==EVeilArmor::Light?TEXT("Light"):CombatPawn->Health->State.Armor==EVeilArmor::Medium?TEXT("Medium"):TEXT("Heavy"),*CombatPawn->Combat->Spec(CombatPawn->Combat->EquippedId).DisplayName,Ammo?Ammo->Magazine:0,Ammo?Ammo->Reserve:0,CS.MedicalCharges,CS.AmmoCharges,CS.bReloading?TEXT("RELOADING | "):TEXT(""),CS.Use!=EVeilUse::None?TEXT("CHANNELING | "):TEXT(""),*CS.Feedback);}
 if(auto* User=Cast<ACoopCharacter>(GetOwningPlayerPawn())){CombatReadout+=FString::Printf(TEXT("Field supplies %d/16 | banked %d | recovery packs %d | enemies neutralized %d\n"),User->FieldItems.Num(),State->Record.SupplyStash.Num(),State->Record.RecoveryPacks.Num(),State->Record.KilledEnemies.Num());}
 if(auto* P=Cast<ATacticalCharacter>(GetOwningPlayerPawn()))CombatReadout+=FString::Printf(TEXT("Shift sprint | Ctrl/C toggle crouch | Q/E peek | bursts %d/3 | %s%s\n"),P->Sprint.Consecutive,P->Sprint.Active?TEXT("SPRINT | "):TEXT(""),P->Sprint.BreathingUntil>P->ServerTime()?TEXT("HEAVY BREATHING"):TEXT(""));
 Readout->SetText(FText::FromString(CombatReadout+FString::Printf(TEXT("OPERATION DEAD SIGNAL\n%s%s\nRelay: %d/3 | optional clues: %d/2 | banked evidence: %d\nWASD / mouse / Space / F | Host console: VFSave, VFLoad\n%s"),*Objective,*Waypoint,int32(State->Record.Outpost),((State->Record.ClueMask&1)?1:0)+((State->Record.ClueMask&2)?1:0),State->Record.BankedEvidence,*State->Status)));
}
void ADeadSignalController::VFSave() { if(HasAuthority() && IsLocalController())if(auto* State=GetWorld()->GetGameState<ADeadSignalState>())State->SaveCampaign(); }
void ADeadSignalController::VFLoad() { if(HasAuthority() && IsLocalController())if(auto* State=GetWorld()->GetGameState<ADeadSignalState>())State->RestoreCampaign(); }
void ADeadSignalController::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 if(IsLocalController() && !Overlay && !IsRunningCommandlet() && !FParse::Param(FCommandLine::Get(),TEXT("nullrhi"))) {Overlay=CreateWidget<UDeadSignalOverlay>(this);if(Overlay)Overlay->AddToViewport();}
 FString CaptureView;
 if(IsLocalController() && HasAuthority() && GetPawn() && FParse::Value(FCommandLine::Get(),TEXT("G1CaptureView="),CaptureView) && (CaptureView=="HQ" || CaptureView=="Chamber" || CaptureView=="Combat")) {
  if(!bCapturePositioned) {
   const bool Chamber=CaptureView=="Chamber";
   GetPawn()->SetActorLocation(Chamber?FVector(32500,54300,100):CaptureView=="Combat"?FVector(2680,4450,100):FVector(1700,4000,100),false,nullptr,ETeleportType::TeleportPhysics);
   SetControlRotation(Chamber?FRotator(8,65,0):CaptureView=="Combat"?FRotator(0,90,0):FRotator::ZeroRotator);
   ConsoleCommand(TEXT("r.MotionBlurQuality 0"));ConsoleCommand(TEXT("r.ScreenPercentage 100"));bCapturePositioned=true;
  }
  if(!bCaptureTaken && GetWorld()->GetTimeSeconds()>2){ConsoleCommand(TEXT("Shot SHOWUI"));bCaptureTaken=true;}
 }
 // Explicit opt-in test: server-authorized placement, client uses real interaction RPC.
 if(!FParse::Param(FCommandLine::Get(),TEXT("G1Smoke")) || !GetPawn())return;
 if(SmokeStart<0)SmokeStart=GetWorld()->GetTimeSeconds();
 auto* State=GetWorld()->GetGameState<ADeadSignalState>();if(!State)return;
 if(HasAuthority()) {
  FName Target=State->Record.NextObjective<9?Actions[State->Record.NextObjective]:NAME_None;
  if(State->Record.NextObjective==2 && !(State->Record.ClueMask&1))Target="clue.utility";
  if(State->Record.NextObjective==6 && !(State->Record.ClueMask&2))Target="clue.archive";
  for(TActorIterator<ADeadSignalNode> It(GetWorld());It;++It)if(It->Action==Target){
   FCollisionQueryParams Query(SCENE_QUERY_STAT(G1DiagnosticApproach),false,GetPawn());
   for(const FVector Offset:{FVector(-180,0,0),FVector(0,-180,0),FVector(180,0,0),FVector(0,180,0)}) {
    const FVector Position=It->GetActorLocation()+Offset;FHitResult Hit;
    if(GetWorld()->OverlapBlockingTestByChannel(Position,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96),Query))continue;
    if(!GetWorld()->LineTraceSingleByChannel(Hit,Position+FVector(0,0,64),It->GetActorLocation(),ECC_Visibility,Query) || Hit.GetActor()!=*It)continue;
    GetPawn()->SetActorLocation(Position,false,nullptr,ETeleportType::TeleportPhysics);
    FVector Eye;FRotator R;GetPawn()->GetActorEyesViewPoint(Eye,R);SetControlRotation((It->GetActorLocation()-Eye).Rotation());break;
   }
   break;
  }
 }
 if(IsLocalController() && GetWorld()->GetTimeSeconds()-SmokeStart>2 && !FParse::Param(FCommandLine::Get(),TEXT("G1Observer")))if(auto* SmokePawn=Cast<AFoundationCharacter>(GetPawn())) {
  // On clients aim from current replicated placement at nearest node.
  ADeadSignalNode* Closest=nullptr;double Distance=350;
  for(TActorIterator<ADeadSignalNode> It(GetWorld());It;++It){const double D=FVector::Dist(It->GetActorLocation(),SmokePawn->GetActorLocation());if(D<Distance){Distance=D;Closest=*It;}}
  if(Closest){FVector Eye;FRotator R;SmokePawn->GetActorEyesViewPoint(Eye,R);SetControlRotation((Closest->GetActorLocation()-Eye).Rotation());SmokePawn->Interaction->Request();}
 }
}
ADeadSignalGameMode::ADeadSignalGameMode() {GameStateClass=ADeadSignalState::StaticClass();PlayerControllerClass=ADeadSignalController::StaticClass();}
void ADeadSignalGameMode::InitGameState() {
 Super::InitGameState();
 if(auto* State=GetWorld()->GetGameState<ADeadSignalState>())if(FPaths::FileExists(State->SavePath()) || FPaths::FileExists(State->SavePath()+".bak"))State->RestoreCampaign();
}
AActor* ADeadSignalGameMode::ChoosePlayerStart_Implementation(AController* Player) {
 auto* State=GetWorld()->GetGameState<ADeadSignalState>();
 const FName Tag=State && State->Record.Outpost==ERelayState::Established?"RelayEstablished":"Headquarters";
 APlayerStart* Fallback=nullptr;
 for(TActorIterator<APlayerStart> It(GetWorld());It;++It)if(It->PlayerStartTag==Tag) {
  if(!Fallback)Fallback=*It;
  if(!GetWorld()->OverlapBlockingTestByChannel(It->GetActorLocation(),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,96))){UE_LOG(LogTemp,Log,TEXT("G1_SPAWN tag=%s location=%s"),*Tag.ToString(),*It->GetActorLocation().ToString());return *It;}
 }
 return Fallback?Fallback:Super::ChoosePlayerStart_Implementation(Player);
}








