#include "Coop.h"
#include "Squad.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "Misc/Paths.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameSession.h"
#include "UObject/ConstructorHelpers.h"

ACoopCharacter::ACoopCharacter(const FObjectInitializer& Initializer):Super(Initializer){}
void ACoopCharacter::BeginPlay(){Super::BeginPlay();if(HasAuthority()&&IsHost())if(auto* S=GetWorld()->GetGameState<ACoopState>())FieldItems=S->Record.HostField;}
bool ACoopCharacter::IsHost()const{const auto* PC=Cast<APlayerController>(GetController());return HasAuthority()&&PC&&PC->IsLocalController();}
void ACoopCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME_CONDITION(ACoopCharacter,FieldItems,COND_OwnerOnly);}
bool ACoopCharacter::DropFieldLoot(){if(!HasAuthority())return false;if(FieldItems.IsEmpty())return true;auto* S=GetWorld()->GetGameState<ACoopState>();return S&&S->DropItems(this);}
void ACoopCharacter::Tick(float D){Super::Tick(D);DiagnosticTick();if(IsHost()&&!bHostFieldInitialized){if(auto* S=GetWorld()->GetGameState<ACoopState>())FieldItems=S->Record.HostField;bHostFieldInitialized=true;ForceNetUpdate();}if(HasAuthority()&&Health->State.Life==EVeilLife::Dead&&!FieldItems.IsEmpty()&&GetWorld()->GetTimeSeconds()>=NextDropRetry){NextDropRetry=GetWorld()->GetTimeSeconds()+1;DropFieldLoot();}}
void ACoopCharacter::EndPlay(EEndPlayReason::Type R){if(HasAuthority()&&R==EEndPlayReason::Destroyed&&!FieldItems.IsEmpty())if(auto* S=GetWorld()->GetGameState<ACoopState>()){if(!DropFieldLoot()){S->QueueDisconnectItems(FieldItems,GetActorLocation());FieldItems.Reset();}}Super::EndPlay(R);}

ACoopState::ACoopState(){PrimaryActorTick.bCanEverTick=true;}
FString ACoopState::SavePath()const{if(!AutomationSavePath.IsEmpty())return AutomationSavePath;if(FParse::Param(FCommandLine::Get(),TEXT("G3Smoke"))||FParse::Param(FCommandLine::Get(),TEXT("G3Capture")))return Super::SavePath();return FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CampaignG3/DeadSignal.vfs"));}
void ACoopState::BeginPlay(){Super::BeginPlay();if(!HasAuthority())return;if(auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld())){int32 Count=0;for(TActorIterator<ANavMeshBoundsVolume> It(GetWorld());It;++It){++Count;Nav->OnNavigationBoundsUpdated(*It);UE_LOG(LogTemp,Log,TEXT("G3_NAV_BOUNDS name=%s bounds=%s"),*It->GetName(),*It->GetComponentsBoundingBox(true).ToString());}Nav->Build();UE_LOG(LogTemp,Log,TEXT("G3_NAV_BUILD volumes=%d data=%s"),Count,*GetNameSafe(Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate)));}else UE_LOG(LogTemp,Warning,TEXT("G3_NAV_SYSTEM_MISSING"));}
bool ACoopState::Eligible(ACoopCharacter* P)const{return HasAuthority()&&IsValid(P)&&P->HasAuthority()&&P->GetController()&&P->GetController()->GetPawn()==P&&P->Health->State.Life==EVeilLife::Alive;}
bool ACoopState::Commit(FDeadSignalRecord Candidate){
 if(!HasAuthority()||bSaveBlocked||!Candidate.Validate()){Status="Supplies transaction blocked: save protected or invalid record.";return false;}
 ++Candidate.Revision;FString Error;
 if(!FDeadSignalStore::Write(SavePath(),Candidate,Error)){Status="Supplies not changed: durable save failed. "+Error;UE_LOG(LogTemp,Error,TEXT("G3_TRANSACTION_FAILED %s"),*Error);return false;}
 Record=MoveTemp(Candidate);OnRep_Record();ForceNetUpdate();return true;
}
bool ACoopState::ClaimPickup(FName Id,FName Type,ACoopCharacter* P){
 if(!Eligible(P)||Id.IsNone()||Record.ConsumedPickups.Contains(Id)||P->FieldItems.Num()>=16)return false;
 FFieldItem Item;Item.InstanceId=FGuid::NewGuid();Item.Type=Type;if(!Item.Validate())return false;
 auto Candidate=Record;Candidate.ConsumedPickups.Add(Id);auto Next=P->FieldItems;Next.Add(Item);if(P->IsHost())Candidate.HostField=Next;
 if(!Commit(MoveTemp(Candidate)))return false;P->FieldItems=MoveTemp(Next);P->ForceNetUpdate();Status="Field supplies acquired. Bank at headquarters; death drops a recovery pack.";
 UE_LOG(LogTemp,Log,TEXT("G3_PICKUP_CLAIM id=%s user=%s count=%d"),*Id.ToString(),*P->GetName(),P->FieldItems.Num());return true;
}
bool ACoopState::ClaimPack(FGuid Id,ACoopCharacter* P){
 if(!Eligible(P))return false;const auto* Pack=Record.RecoveryPacks.FindByPredicate([Id](const FRecoveryRecord& R){return R.PackId==Id;});
 if(!Pack||P->FieldItems.Num()+Pack->Items.Num()>16)return false;
 auto Candidate=Record;auto Next=P->FieldItems;Next.Append(Pack->Items);Candidate.RecoveryPacks.RemoveAll([Id](const FRecoveryRecord& R){return R.PackId==Id;});if(P->IsHost())Candidate.HostField=Next;
 if(!Commit(MoveTemp(Candidate)))return false;P->FieldItems=MoveTemp(Next);P->ForceNetUpdate();Status="Recovery pack claimed once.";ReconcilePacks();UE_LOG(LogTemp,Log,TEXT("G3_PACK_CLAIM id=%s user=%s"),*Id.ToString(),*P->GetName());return true;
}
bool ACoopState::DropItems(ACoopCharacter* P){
 if(!HasAuthority()||!IsValid(P)||P->FieldItems.IsEmpty())return false;
 FRecoveryRecord Pack;Pack.PackId=FGuid::NewGuid();Pack.Items=P->FieldItems;Pack.Location=P->GetActorLocation();
 FCollisionQueryParams Q(SCENE_QUERY_STAT(G3PackGround),false,P);FHitResult Hit;
 if(GetWorld()->LineTraceSingleByChannel(Hit,Pack.Location+FVector(0,0,30),Pack.Location-FVector(0,0,400),ECC_Visibility,Q))Pack.Location=Hit.ImpactPoint+FVector(0,0,20);
 auto Candidate=Record;Candidate.RecoveryPacks.Add(Pack);if(P->IsHost())Candidate.HostField.Reset();
 if(!Commit(MoveTemp(Candidate)))return false;P->FieldItems.Reset();P->ForceNetUpdate();Status="Unbanked supplies retained in a persistent recovery pack.";ReconcilePacks();UE_LOG(LogTemp,Log,TEXT("G3_PACK_DROP id=%s items=%d"),*Pack.PackId.ToString(),Pack.Items.Num());return true;
}
bool ACoopState::BankItems(ACoopCharacter* P){if(!Eligible(P)||P->FieldItems.IsEmpty())return false;auto Candidate=Record;Candidate.SupplyStash.Append(P->FieldItems);if(P->IsHost())Candidate.HostField.Reset();if(!Commit(MoveTemp(Candidate)))return false;P->FieldItems.Reset();P->ForceNetUpdate();Status="Supplies banked in the host campaign stash.";UE_LOG(LogTemp,Log,TEXT("G3_BANK count=%d"),Record.SupplyStash.Num());return true;}
bool ACoopState::RegisterCasualty(FName Id){if(!HasAuthority()||Id.IsNone())return false;if(Record.KilledEnemies.Contains(Id))return true;auto Candidate=Record;Candidate.KilledEnemies.Add(Id);return Commit(MoveTemp(Candidate));}
bool ACoopState::EncounterCleared(FName Group)const{int32 Count=0;for(TActorIterator<ASquadEnemy> It(GetWorld());It;++It)if(It->Encounter==Group){++Count;if(It->Health->State.Life!=EVeilLife::Dead)return false;}return Count>0;}
void ACoopState::ReconcilePacks(){
 if(!HasAuthority())return;TSet<FGuid> Present;
 for(TActorIterator<ARecoveryPack> It(GetWorld());It;++It){if(!Record.RecoveryPacks.ContainsByPredicate([&It](const FRecoveryRecord& R){return R.PackId==It->PackId;}))It->Destroy();else if(Present.Contains(It->PackId))It->Destroy();else Present.Add(It->PackId);}
 for(const auto& R:Record.RecoveryPacks)if(!Present.Contains(R.PackId)){FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;auto* P=GetWorld()->SpawnActor<ARecoveryPack>(R.Location,FRotator::ZeroRotator,Params);if(P){P->PackId=R.PackId;P->ItemCount=R.Items.Num();P->ForceNetUpdate();}}
}
void ACoopState::QueueDisconnectItems(const TArray<FFieldItem>& Items,const FVector& Location){FRecoveryRecord R;R.PackId=FGuid::NewGuid();R.Items=Items;R.Location=Location;PendingDisconnectPacks.Add(R);Status="Disconnected supplies pending durable save; retrying.";}
void ACoopState::Tick(float D){Super::Tick(D);DiagnosticTick();if(!HasAuthority()||GetWorld()->GetTimeSeconds()<NextSync)return;NextSync=GetWorld()->GetTimeSeconds()+.5;ReconcilePacks();if(!PendingDisconnectPacks.IsEmpty()){auto Candidate=Record;Candidate.RecoveryPacks.Append(PendingDisconnectPacks);TSet<FGuid> PendingIds;for(const auto& Pack:PendingDisconnectPacks)for(const auto& Item:Pack.Items)PendingIds.Add(Item.InstanceId);Candidate.HostField.RemoveAll([&PendingIds](const FFieldItem& Item){return PendingIds.Contains(Item.InstanceId);});if(Commit(MoveTemp(Candidate))){PendingDisconnectPacks.Reset();ReconcilePacks();}}}
bool ACoopState::RestoreCampaign(){for(TActorIterator<ACoopCharacter> It(GetWorld());It;++It)if(It->GetController()){Status="Restart the host to restore G3 state safely. Live rollback is disabled to prevent duplicate guest loot.";return false;}return Super::RestoreCampaign();}

AFieldPickup::AFieldPickup(){bReplicates=true;PrimaryActorTick.bCanEverTick=true;Mesh=CreateDefaultSubobject<UStaticMeshComponent>("Mesh");SetRootComponent(Mesh);static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));Mesh->SetStaticMesh(Cube.Object);Mesh->SetRelativeScale3D(FVector(.3));Mesh->SetCollisionProfileName("BlockAll");Label=CreateDefaultSubobject<UTextRenderComponent>("Label");Label->SetupAttachment(Mesh);Label->SetRelativeLocation(FVector(0,0,130));Label->SetWorldSize(28);}
void AFieldPickup::Tick(float D){Super::Tick(D);if(auto* S=GetWorld()->GetGameState<ACoopState>()){const bool Gone=S->Record.ConsumedPickups.Contains(StableId);SetActorHiddenInGame(Gone);SetActorEnableCollision(!Gone);Label->SetText(FText::FromString("F: "+(Definition?Definition->DisplayName:FString("Missing loot definition"))));}}
bool AFieldPickup::Activate_Implementation(APawn* User){auto* P=Cast<ACoopCharacter>(User);auto* S=GetWorld()->GetGameState<ACoopState>();return HasAuthority()&&P&&S&&Definition&&FVector::DistSquared(GetActorLocation(),P->GetActorLocation())<=FMath::Square(350.f)&&S->ClaimPickup(StableId,Definition->StableId,P);}
ARecoveryPack::ARecoveryPack(){bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=true;Mesh=CreateDefaultSubobject<UStaticMeshComponent>("Mesh");SetRootComponent(Mesh);static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));Mesh->SetStaticMesh(Cube.Object);Mesh->SetRelativeScale3D(FVector(.4,.3,.3));Mesh->SetCollisionProfileName("BlockAll");Label=CreateDefaultSubobject<UTextRenderComponent>("Label");Label->SetupAttachment(Mesh);Label->SetRelativeLocation(FVector(0,0,130));Label->SetWorldSize(25);}
void ARecoveryPack::Tick(float D){Super::Tick(D);Label->SetText(FText::FromString(FString::Printf(TEXT("F: recovery pack (%d supplies)"),ItemCount)));}
void ARecoveryPack::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ARecoveryPack,PackId);DOREPLIFETIME(ARecoveryPack,ItemCount);}
bool ARecoveryPack::Activate_Implementation(APawn* User){auto* P=Cast<ACoopCharacter>(User);auto* S=GetWorld()->GetGameState<ACoopState>();return HasAuthority()&&P&&S&&FVector::DistSquared(GetActorLocation(),P->GetActorLocation())<=FMath::Square(350.f)&&S->ClaimPack(PackId,P);}
AFieldBank::AFieldBank(){bReplicates=true;Mesh=CreateDefaultSubobject<UStaticMeshComponent>("Mesh");SetRootComponent(Mesh);static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));Mesh->SetStaticMesh(Cube.Object);Mesh->SetRelativeScale3D(FVector(.6,.4,.6));Mesh->SetCollisionProfileName("BlockAll");Label=CreateDefaultSubobject<UTextRenderComponent>("Label");Label->SetupAttachment(Mesh);Label->SetRelativeLocation(FVector(0,0,100));Label->SetWorldSize(24);Label->SetText(FText::FromString("F: bank field supplies"));}
bool AFieldBank::Activate_Implementation(APawn* User){auto* P=Cast<ACoopCharacter>(User);auto* S=GetWorld()->GetGameState<ACoopState>();return HasAuthority()&&P&&S&&FVector::DistSquared(GetActorLocation(),P->GetActorLocation())<=FMath::Square(350.f)&&S->BankItems(P);}
ACoopGameMode::ACoopGameMode(){DefaultPawnClass=ACoopCharacter::StaticClass();GameStateClass=ACoopState::StaticClass();}
void ACoopGameMode::PreLogin(const FString& O,const FString& A,const FUniqueNetIdRepl& I,FString& E){Super::PreLogin(O,A,I,E);if(E.IsEmpty()&&GetNumPlayers()>=4)E="VEILFALL session full (4 players).";}
void ACoopGameMode::Logout(AController* Exiting){if(IsValid(Exiting))if(auto* P=Cast<ACoopCharacter>(Exiting->GetPawn()))if(!P->FieldItems.IsEmpty()&&!P->DropFieldLoot())if(auto* S=GetWorld()->GetGameState<ACoopState>()){S->QueueDisconnectItems(P->FieldItems,P->GetActorLocation());P->FieldItems.Reset();}UE_LOG(LogTemp,Log,TEXT("G3_LOGOUT controller=%s"),*GetNameSafe(Exiting));Super::Logout(Exiting);}







void AFieldBank::BeginPlay(){Super::BeginPlay();Label->SetText(FText::FromString("F: bank field supplies"));}
