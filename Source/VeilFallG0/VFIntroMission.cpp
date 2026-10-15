// Intro mission flow. Each state has an Enter() script; ArrangeWorld() rebuilds the world for any state so
// checkpoints and the developer state menu use the same path as normal play.
#include "VFIntro.h"
#include "VFIntroShared.h"
#include "Camera/CameraActor.h"
#include "Components/AudioComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "NavigationSystem.h"
#include "NavigationData.h"
#include "NavMesh/RecastNavMesh.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace {
using S=EVFIntroState;
const FName WIFE="wife",OLDEST="son_oldest",MIDDLE="son_middle",YOUNGEST="son_youngest",SHERIFF="sheriff",MAYOR="mayor",NATHAN="nathan",VGLEAD="vg_lead";
const TArray<FName>& Sons(){static TArray<FName> R{OLDEST,MIDDLE,YOUNGEST};return R;}
const TArray<FName>& RequiredGear(){static TArray<FName> R{"equip.rifle","equip.sidearm","equip.armor","equip.ammo","equip.radio"};return R;}
USoundAttenuation* Attenuation(UObject* Outer,float Falloff){
 auto* A=NewObject<USoundAttenuation>(Outer);A->Attenuation.bAttenuate=true;A->Attenuation.bSpatialize=true;A->Attenuation.FalloffDistance=Falloff;A->Attenuation.AttenuationShapeExtents=FVector(400,0,0);return A;
}
}

AVFMissionController::AVFMissionController(){
 PrimaryActorTick.bCanEverTick=false;SetRootComponent(CreateDefaultSubobject<USceneComponent>("Root"));
 Dialogue=TSoftObjectPtr<UDataTable>(FSoftObjectPath(TEXT("/Game/VeilFall/Data/DT_VF_IntroDialogue.DT_VF_IntroDialogue")));
 AlertSound=TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/VeilFall/Audio/TEMP_SFX_EmergencyTone.TEMP_SFX_EmergencyTone")));
 ExplosionSound=TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/VeilFall/Audio/TEMP_SFX_Explosion.TEMP_SFX_Explosion")));
 GunshotSound=TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/VeilFall/Audio/TEMP_SFX_Gunshot.TEMP_SFX_Gunshot")));
 EngineSound=TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/VeilFall/Audio/TEMP_SFX_EngineLoop.TEMP_SFX_EngineLoop")));
 PASound=TSoftObjectPtr<USoundBase>(FSoftObjectPath(TEXT("/Game/VeilFall/Audio/TEMP_SFX_PAChime.TEMP_SFX_PAChime")));
}
AVFMissionController* AVFMissionController::Get(const UObject* Ctx){
 const UWorld* W=Ctx?Ctx->GetWorld():nullptr;if(!W)return nullptr;
 for(TActorIterator<AVFMissionController> It(W);It;++It)return *It;return nullptr;
}
FString AVFMissionController::StateName(EVFIntroState V){return StaticEnum<EVFIntroState>()->GetNameStringByValue(int64(V));}

void AVFMissionController::BeginPlay(){
 Super::BeginPlay();
 DialogueLoaded=Dialogue.LoadSynchronous();AlertLoaded=AlertSound.LoadSynchronous();ExplosionLoaded=ExplosionSound.LoadSynchronous();
 GunshotLoaded=GunshotSound.LoadSynchronous();EngineLoaded=EngineSound.LoadSynchronous();PALoaded=PASound.LoadSynchronous();
 if(!DialogueLoaded)UE_LOG(LogTemp,Error,TEXT("VF_INTRO dialogue table missing: %s"),*Dialogue.ToString());
 for(TActorIterator<AVFMarker> It(GetWorld());It;++It)if(!It->MarkerId.IsNone())Markers.Add(It->MarkerId,*It);
 for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag("VF_Scuttle"))ScuttleOrigins.Add(*It,It->GetActorTransform());
 GetWorldTimerManager().SetTimer(WatchTimer,this,&AVFMissionController::WatchPlayer,.25f,true,1.f);
 // Navigation health check: report (and if needed create) nav data so story NPCs can path through the house and shop.
 GetWorldTimerManager().SetTimer(NavTimer,FTimerDelegate::CreateWeakLambda(this,[this]{
  auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());if(!Nav){UE_LOG(LogTemp,Error,TEXT("VF_NAV no navigation system"));return;}
  ANavigationData* Data=Nav->GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
  if(!Data){UE_LOG(LogTemp,Warning,TEXT("VF_NAV no nav data; creating and building"));Data=Nav->GetDefaultNavDataInstance(FNavigationSystem::Create);Nav->Build();}
  int32 Ok=0,Total=0;for(const FName M:{FName("player_start"),FName("wife_vault"),FName("vault_1"),FName("player_port"),FName("ncd_front"),FName("column_t_01")}){bool b=false;const FTransform T=MarkerTransform(M,&b);if(!b)continue;++Total;FNavLocation L;const bool P=Nav->ProjectPointToNavigation(T.GetLocation(),L,FVector(100,100,250));if(P)++Ok;UE_LOG(LogTemp,Display,TEXT("VF_NAV_POINT %s %d z=%.0f"),*M.ToString(),P,L.Location.Z);}
  const auto* Recast=Cast<ARecastNavMesh>(Data);UE_LOG(LogTemp,Display,TEXT("VF_NAV t=%.0f data=%s tiles=%d building=%d projected=%d/%d"),GetWorld()->GetTimeSeconds(),*GetNameSafe(Data),Recast?Recast->GetNavMeshTilesCount():-1,Nav->IsNavigationBuildInProgress(),Ok,Total);
  // Saved tiles from the editor can be stale/partial for this generated map; rebuild once at runtime if key points miss.
  if(Ok<Total&&NavReports==0){UE_LOG(LogTemp,Warning,TEXT("VF_NAV rebuilding navigation at runtime"));Nav->Build();}
  if(Ok==Total||++NavReports>=6)GetWorldTimerManager().ClearTimer(NavTimer);
 }),15.f,true,5.f);
 if(FParse::Param(FCommandLine::Get(),TEXT("VFAutoplay")))GetWorldTimerManager().SetTimer(AutoTimer,this,&AVFMissionController::Autoplay,1.f,true,3.f);
 UE_LOG(LogTemp,Display,TEXT("VF_INTRO_READY markers=%d scuttle=%d dialogue=%d"),Markers.Num(),ScuttleOrigins.Num(),DialogueLoaded?DialogueLoaded->GetRowMap().Num():0);
 // Start a frame later so the player pawn and HUD exist.
 FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[this]{
  FString Start;if(FParse::Value(FCommandLine::Get(),TEXT("VFState="),Start)){const int64 V=StaticEnum<EVFIntroState>()->GetValueByNameString(Start);if(V!=INDEX_NONE){JumpToState(EVFIntroState(V));return;}}
  JumpToState(S::NORMAL_HOME);
 }),.2f,false);
}

// ---------------------------------------------------------------- lookups
FTransform AVFMissionController::MarkerTransform(FName Id,bool* bFound)const{
 const auto* M=Markers.Find(Id);if(bFound)*bFound=M&&*M;
 if(!M||!*M){UE_LOG(LogTemp,Warning,TEXT("VF_INTRO missing marker %s"),*Id.ToString());return FTransform(DefendPoint());}
 return (*M)->GetActorTransform();
}
TArray<AVFMarker*> AVFMissionController::MarkersWithPrefix(const FString& Prefix)const{
 TArray<AVFMarker*> R;for(const auto& P:Markers)if(P.Value&&P.Key.ToString().StartsWith(Prefix))R.Add(P.Value);
 R.Sort([](const AVFMarker& A,const AVFMarker& B){return A.MarkerId.LexicalLess(B.MarkerId);});return R;
}
AVFIntroNPC* AVFMissionController::NPC(FName Who)const{for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It)if(It->RoleId==Who&&!It->IsActorBeingDestroyed())return *It;return nullptr;}
AVFInteractable* AVFMissionController::Item(FName Id)const{for(TActorIterator<AVFInteractable> It(GetWorld());It;++It)if(It->InteractionId==Id)return *It;return nullptr;}
AVFAnimatedProp* AVFMissionController::Prop(FName Id)const{for(TActorIterator<AVFAnimatedProp> It(GetWorld());It;++It)if(It->PropId==Id)return *It;return nullptr;}
AVFScriptedVehicle* AVFMissionController::Vehicle(FName Id)const{for(TActorIterator<AVFScriptedVehicle> It(GetWorld());It;++It)if(It->VehicleId==Id&&!It->IsActorBeingDestroyed())return *It;return nullptr;}
AVFIntroPlayerController* AVFMissionController::PC()const{return Cast<AVFIntroPlayerController>(GetWorld()->GetFirstPlayerController());}
AVFIntroCharacter* AVFMissionController::Player()const{auto* P=GetWorld()->GetFirstPlayerController();return P?Cast<AVFIntroCharacter>(P->GetPawn()):nullptr;}
FVector AVFMissionController::DefendPoint()const{const auto* M=Markers.Find("defend_point");return M&&*M?(*M)->GetActorLocation():FVector(800,1100,0);}
int32 AVFMissionController::LivingNCD()const{int32 N=0;for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It)if(It->Faction==EVFFaction::NCD&&It->IsAlive()&&!It->IsHidden())++N;return N;}

// ---------------------------------------------------------------- helpers
void AVFMissionController::After(float Seconds,TFunction<void()> Fn){
 const int32 E=Epoch;FTimerHandle H;
 GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[this,E,Fn]{if(E==Epoch&&Fn)Fn();}),FMath::Max(.01f,Seconds),false);
}
void AVFMissionController::Lines(FName Beat,TFunction<void()> Then){
 TArray<FVFDialogueLine> Rows;
 if(DialogueLoaded)for(const auto& R:DialogueLoaded->GetRowMap())if(const auto* L=reinterpret_cast<const FVFDialogueLine*>(R.Value);L&&L->Beat==Beat)Rows.Add(*L);
 if(Rows.IsEmpty())UE_LOG(LogTemp,Warning,TEXT("VF_INTRO no dialogue rows for beat %s"),*Beat.ToString());
 const int32 Serial=++LineSerial;float T=0;
 for(const auto& L:Rows){
  const float D=L.Duration>0?L.Duration:FMath::Clamp(1.3f+L.Text.Len()*.052f,1.8f,8.f);
  After(T,[this,L,D,Serial]{if(Serial!=LineSerial)return;if(auto* P=PC())if(P->HUD)P->HUD->ShowSubtitle(L.Speaker,L.Text,D);
   if(L.Speaker.StartsWith("NCD")&&PALoaded){bool bFound=false;const FTransform T=MarkerTransform("pa_source",&bFound);UGameplayStatics::PlaySoundAtLocation(this,PALoaded,bFound?T.GetLocation():DefendPoint(),.9f);}});
  T+=D+.3f;
 }
 if(Then)After(T+.05f,Then);
}
void AVFMissionController::Bark(FName Beat){
 if(DialogueLoaded)for(const auto& R:DialogueLoaded->GetRowMap())if(const auto* L=reinterpret_cast<const FVFDialogueLine*>(R.Value);L&&L->Beat==Beat){if(auto* P=PC())if(P->HUD)P->HUD->ShowSubtitle(L->Speaker,L->Text,2.2f);return;}
}
void AVFMissionController::Objective(const FString& Text){if(auto* P=PC())if(P->HUD)P->HUD->ShowObjective(Text);UE_LOG(LogTemp,Display,TEXT("VF_INTRO_OBJECTIVE %s"),*Text);}
void AVFMissionController::Hint(const FString& Text){if(auto* P=PC())if(P->HUD)P->HUD->ShowHint(Text);}
void AVFMissionController::Checkpoint(){CheckpointState=State;CheckpointWave=State==S::HOLD_SHOP?Wave:0;UE_LOG(LogTemp,Display,TEXT("VF_INTRO_CHECKPOINT %s wave=%d"),*StateName(State),CheckpointWave);}
void AVFMissionController::SetStateValue(EVFIntroState V){
 State=V;StateStart=GetWorld()->GetTimeSeconds();UE_LOG(LogTemp,Display,TEXT("VF_INTRO_STATE %s"),*StateName(V));
 KeypadPrompt();OnStateChanged.Broadcast(V);ReceiveStateEntered(V);
#if !UE_BUILD_SHIPPING
 if(FParse::Param(FCommandLine::Get(),TEXT("VFCapture")))After(4.5f,[this]{FString Look;if(FParse::Value(FCommandLine::Get(),TEXT("VFLookAt="),Look))if(auto* N=NPC(FName(*Look)))if(auto* P=Player()){FVector Eye;FRotator R;P->GetActorEyesViewPoint(Eye,R);P->GetController()->SetControlRotation((N->GetActorLocation()-Eye).Rotation());}
  if(auto* C=GetWorld()->GetFirstPlayerController())C->ConsoleCommand(TEXT("Shot SHOWUI"));UE_LOG(LogTemp,Display,TEXT("VF_CAPTURE %s"),*StateName(State));});
#endif
}
void AVFMissionController::SpawnBlast(const FVector& At,float Scale,bool bFire,bool bSound){
 auto* B=GetWorld()->SpawnActor<AVFBlastEffect>(AVFBlastEffect::StaticClass(),At,FRotator::ZeroRotator);if(!B)return;Spawned.Add(B);B->Init(Scale,bSound?ExplosionLoaded.Get():nullptr,bFire);
 if(auto* P=Player())if(auto* C=PC())if(C->HUD){const float D=FVector::Dist(P->GetActorLocation(),At);if(D<6000)C->HUD->PulseDamage(FMath::Clamp(.35f*Scale*(1-D/6000),0.f,.4f));}
}
void AVFMissionController::PlayShot(const FVector& At){
 if(!GunshotLoaded)return;const double Now=GetWorld()->GetTimeSeconds();if(Now-LastShotSound<.03)return;LastShotSound=Now;
 if(!ShotAttenuation)ShotAttenuation=Attenuation(this,9000);
 UGameplayStatics::PlaySoundAtLocation(this,GunshotLoaded,At,.55f,FMath::FRandRange(.9f,1.08f),0,ShotAttenuation);
}
void AVFMissionController::Grenade(const FVector& At,AActor* Source){
 auto* P=Player();if(!P||FVector::Dist2D(P->GetActorLocation(),At)>1100)return;
 if(auto* C=PC())if(C->HUD)C->HUD->GrenadeWarning(3);
 if(GetWorld()->GetTimeSeconds()-LastGrenade>8){LastGrenade=GetWorld()->GetTimeSeconds();Bark("bark_grenade");}
 if(!bGrenadeHinted){bGrenadeHinted=true;Hint("GRENADE: move away from it, or put hard cover between you and it.");}
}
bool AVFMissionController::RequestShotToken(AActor* Shooter,AActor* Target){
 const double Now=GetWorld()->GetTimeSeconds();for(auto It=ShotTokens.CreateIterator();It;++It)if(!It.Key().IsValid()||It.Value()<Now)It.RemoveCurrent();
 if(auto* T=ShotTokens.Find(Shooter)){*T=Now+2.5;return true;}
 if(ShotTokens.Num()>=(Wave>=4?4:3))return false;ShotTokens.Add(Shooter,Now+2.5);return true;
}
void AVFMissionController::ReserveCover(AVFCoverPoint* Cover,AController* User){for(auto It=CoverUse.CreateIterator();It;++It)if(It.Value()==User)It.RemoveCurrent();CoverUse.Add(Cover,User);}
bool AVFMissionController::CoverFree(const AVFCoverPoint* Cover,const AController* User)const{
 const auto* U=CoverUse.Find(Cover);if(!U||!U->IsValid()||U->Get()==User)return true;
 const auto* N=Cast<AVFIntroNPC>(U->Get()->GetPawn());return !N||!N->IsAlive();
}
void AVFMissionController::OnPawnKilled(AVFIntroNPC* Pawn){
 if(!Pawn||Pawn->bEssential||Killed.Contains(Pawn))return;Killed.Add(Pawn);
 for(auto It=CoverUse.CreateIterator();It;++It)if(It.Value().IsValid()&&It.Value()->GetPawn()==Pawn)It.RemoveCurrent();
 ShotTokens.Remove(Pawn);
}

// ---------------------------------------------------------------- spawning / placement
AVFIntroNPC* AVFMissionController::SpawnNPC(TSubclassOf<AVFIntroNPC> Class,FName Who,const FString& Name,EVFFaction Faction,const FVector& At,const FRotator& Rot,TFunction<void(AVFIntroNPC*)> Configure){
 UClass* C=Class?Class.Get():AVFIntroNPC::StaticClass();
 auto* N=GetWorld()->SpawnActorDeferred<AVFIntroNPC>(C,FTransform(FRotator(0,Rot.Yaw,0),At+FVector(0,0,95)),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn);
 if(!N)return nullptr;N->RoleId=Who;N->DisplayName=Name;N->Faction=Faction;
 if(Faction==EVFFaction::NCD){N->ClothingColor=FLinearColor(.018f,.022f,.03f);N->bHelmet=true;N->bArmed=true;if(N->WeaponDamage<=0)N->WeaponDamage=14;}
 else if(Faction==EVFFaction::Vanguard){static const FLinearColor Tones[]={FLinearColor(.16f,.15f,.09f),FLinearColor(.1f,.12f,.08f),FLinearColor(.2f,.17f,.12f)};N->ClothingColor=Tones[FMath::RandRange(0,2)];N->bHelmet=Who!=VGLEAD&&FMath::RandBool();N->bArmed=true;N->bEssential=Who==VGLEAD;if(N->WeaponDamage<=0)N->WeaponDamage=45;}
 if(Configure)Configure(N);
 UGameplayStatics::FinishSpawningActor(N,FTransform(FRotator(0,Rot.Yaw,0),At+FVector(0,0,95)));
 if(auto* AI=Cast<AVFIntroAIController>(N->GetController())){AI->Accuracy=Faction==EVFFaction::NCD?3.4f:Faction==EVFFaction::Vanguard?1.1f:2.f;}
 Spawned.Add(N);return N;
}
AVFScriptedVehicle* AVFMissionController::SpawnVehicle(FName Id,const FVector& At,float Yaw,FLinearColor Color,bool bArmored){
 UClass* C=VehicleClass?VehicleClass.Get():AVFScriptedVehicle::StaticClass();
 auto* V=GetWorld()->SpawnActorDeferred<AVFScriptedVehicle>(C,FTransform(FRotator(0,Yaw,0),FVector(At.X,At.Y,0)),nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
 if(!V)return nullptr;V->VehicleId=Id;V->Color=Color;V->bArmored=bArmored;UGameplayStatics::FinishSpawningActor(V,FTransform(FRotator(0,Yaw,0),FVector(At.X,At.Y,0)));
 if(EngineLoaded){V->Engine->SetSound(EngineLoaded);if(!EngineAttenuation)EngineAttenuation=Attenuation(this,6000);V->Engine->AttenuationSettings=EngineAttenuation;}
 Spawned.Add(V);return V;
}
void AVFMissionController::PlaceNPC(FName Who,FName Station){
 auto* N=NPC(Who);if(!N)return;const FTransform T=MarkerTransform(Station);
 N->SetPresent(true);N->SetActorLocationAndRotation(T.GetLocation()+FVector(0,0,N->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2),FRotator(0,T.Rotator().Yaw,0),false,nullptr,ETeleportType::TeleportPhysics);
 if(auto* AI=Cast<AVFIntroAIController>(N->GetController()))AI->Idle(T.Rotator());
}
void AVFMissionController::SendNPC(FName Who,FName Station,TFunction<void()> Then,bool bRun){
 auto* N=NPC(Who);if(!N){if(Then)After(.1f,Then);return;}
 const FTransform T=MarkerTransform(Station);auto* AI=Cast<AVFIntroAIController>(N->GetController());
 if(!AI){PlaceNPC(Who,Station);if(Then)After(.1f,Then);return;}
 const int32 E=Epoch;TWeakObjectPtr<AVFIntroNPC> Weak=N;const FRotator Face=T.Rotator();
 AI->GoTo(T.GetLocation(),[this,E,Then,Weak,Face]{if(E!=Epoch)return;if(Weak.IsValid())if(auto* A=Cast<AVFIntroAIController>(Weak->GetController()))A->Idle(Face);if(Then)Then();},bRun);
}
void AVFMissionController::HideNPC(FName Who,bool bHide){if(auto* N=NPC(Who))N->SetPresent(!bHide);}
void AVFMissionController::PlacePlayer(FName Station){
 auto* P=Player();if(!P)return;if(P->bRiding)P->SetRiding(nullptr);
 const FTransform T=MarkerTransform(Station);P->SetActorLocation(T.GetLocation()+FVector(0,0,P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2),false,nullptr,ETeleportType::TeleportPhysics);
 if(auto* C=P->GetController())C->SetControlRotation(FRotator(0,T.Rotator().Yaw,0));
}
void AVFMissionController::CleanupSpawned(){
 for(auto& A:Spawned)if(IsValid(A))A->Destroy();Spawned.Reset();Killed.Reset();CoverUse.Reset();ShotTokens.Reset();
 GetWorldTimerManager().ClearTimer(WaveTimer);
}

// ---------------------------------------------------------------- arrange the world for a state (jumps, checkpoints)
void AVFMissionController::ArrangeWorld(EVFIntroState V){
 const auto At=[V](S From,S To){return int32(V)>=int32(From)&&int32(V)<=int32(To);};
 auto* P=Player();if(P){P->Health->ResetHealth();P->Combat->Restock();P->SetArmed(int32(V)>=int32(S::LOCALS_ARRIVE));P->Health->SetArmor(int32(V)>=int32(S::LOCALS_ARRIVE)?EVeilArmor::Medium:EVeilArmor::Light);if(P->bArmed)P->Combat->State.MedicalCharges=4;P->RefreshPresentation();}
 static const TMap<S,FName> PlayerStations{{S::NORMAL_HOME,"player_start"},{S::EMERGENCY_ALERT,"player_start"},{S::MOVE_TO_SHOP,"player_kitchen"},{S::OPEN_VAULT,"player_shop"},{S::EQUIP,"player_vault"},
  {S::LOCALS_ARRIVE,"player_vault"},{S::SECURE_CHILDREN,"player_panel"},{S::NCD_ARRIVAL,"player_vault_front"},{S::NCD_DEMAND,"player_monitors"},{S::DEFENSE_ARMED,"player_panel"},{S::PROPERTY_ATTACK,"player_panel"},
  {S::HOLD_SHOP,"player_port"},{S::VANGUARD_ARRIVAL,"player_port"},{S::EXTRACT_FAMILY,"player_port"},{S::EVACUATE,"player_vault_front"},{S::SCUTTLE_PROPERTY,"player_vault_front"},{S::MISSION_COMPLETE,"player_vault_front"}};
 PlacePlayer(PlayerStations.FindRef(V));
 if(auto* C=PC()){C->SetViewTargetWithBlend(P,0);if(C->HUD){C->HUD->ShowAlert(false);C->HUD->ShowTitle(false);C->HUD->FadeTo(0,.6f);C->HUD->ShowKeypad(false);}}
 Talked.Reset();bRadioGiven=int32(V)>int32(S::SECURE_CHILDREN);bVaultOpening=false;bColumnHalted=false;bScuttled=false;bThrown=false;bVanguardIn=false;Wave=0;ForcedPrompt.Reset();
 // Home props
 for(TActorIterator<AVFInteractable> It(GetWorld());It;++It){
  const FString Id=It->InteractionId.ToString();
  if(Id.StartsWith("equip.")){if(int32(V)>=int32(S::LOCALS_ARRIVE)){It->bUsed=true;It->Mesh->SetVisibility(false,true);It->Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);It->Screen->SetVisibility(false);}else It->ResetUse();}
  else It->ResetUse();
 }
 if(auto* TV=Item("home.tv"))TV->SetScreenText(At(S::EMERGENCY_ALERT,S::MISSION_COMPLETE)?TEXT("NATIONAL EMERGENCY ALERT\nSHELTER IN PLACE IMMEDIATELY"):TEXT("~ Saturday cartoons ~"),At(S::EMERGENCY_ALERT,S::MISSION_COMPLETE)?FLinearColor(1,.2f,.15f):FLinearColor(.6f,.8f,1));
 if(auto* Road=Cast<AVFDefensePanel>(Item("panel.road"))){Road->bTriggerReady=V==S::DEFENSE_ARMED;Road->SetDefenseState(int32(V)>=int32(S::PROPERTY_ATTACK)?EVFDefenseState::Triggered:V==S::DEFENSE_ARMED?EVFDefenseState::Armed:EVFDefenseState::Safe);}
 if(auto* Bay=Cast<AVFDefensePanel>(Item("panel.bay")))Bay->SetDefenseState(int32(V)>=int32(S::HOLD_SHOP)?EVFDefenseState::Triggered:EVFDefenseState::Safe);
 if(auto* Mon=Item("shop.monitors"))Mon->SetScreenText(TEXT("CAM 1 GATE   CAM 2 DRIVE\nCAM 3 BARN   CAM 4 EAST"),FLinearColor(.5f,1,.6f));
 for(TActorIterator<AVFScriptedDamageZone> It(GetWorld());It;++It)It->SetArmed(V==S::DEFENSE_ARMED);
 // Doors
 if(auto* D=Prop("vault_door"))D->SetOpen(At(S::EQUIP,S::SECURE_CHILDREN)||int32(V)>=int32(S::EVACUATE),true);
 if(auto* D=Prop("defense_wall"))D->SetOpen(int32(V)>=int32(S::HOLD_SHOP),true);
 if(auto* D=Prop("gate"))D->SetOpen(int32(V)<int32(S::SECURE_CHILDREN)||int32(V)>=int32(S::DEFENSE_ARMED),true);
 if(auto* D=Prop("bay1"))D->SetOpen(At(S::SECURE_CHILDREN,S::NCD_ARRIVAL)||int32(V)>=int32(S::EVACUATE),true);
 if(auto* D=Prop("bay2"))D->SetOpen(false,true);
 if(auto* D=Prop("house_door"))D->SetOpen(true,true);
 // House restored unless already scuttled
 for(auto& Pair:ScuttleOrigins)if(Pair.Key.IsValid()){Pair.Key->SetActorTransform(Pair.Value);Pair.Key->SetActorHiddenInGame(false);}
 GetWorldTimerManager().ClearTimer(CollapseTimer);
 // Family
 auto Kids=[this](const TCHAR* Prefix){int32 i=1;for(const FName& K:Sons())PlaceNPC(K,FName(*FString::Printf(TEXT("%s%d"),Prefix,i++)));};
 if(At(S::NORMAL_HOME,S::EMERGENCY_ALERT)){PlaceNPC(OLDEST,"oldest_living");PlaceNPC(MIDDLE,"middle_room");PlaceNPC(YOUNGEST,"youngest_living");}
 else if(At(S::MOVE_TO_SHOP,S::SECURE_CHILDREN))Kids(TEXT("hallway_"));
 else if(At(S::NCD_ARRIVAL,S::EXTRACT_FAMILY))Kids(TEXT("vault_"));
 else if(V==S::EVACUATE)Kids(TEXT("kids_shop_"));
 else for(const FName& K:Sons())HideNPC(K,true);
 for(const FName& K:Sons())if(auto* N=NPC(K)){N->bAlert=int32(V)>=int32(S::EMERGENCY_ALERT);N->bHuddle=At(S::NCD_ARRIVAL,S::EXTRACT_FAMILY);}
 static const TMap<S,FName> WifeStations{{S::NORMAL_HOME,"wife_kitchen"},{S::EMERGENCY_ALERT,"wife_kitchen"},{S::MOVE_TO_SHOP,"wife_kitchen"},{S::OPEN_VAULT,"wife_vault"},{S::EQUIP,"wife_vault"},{S::LOCALS_ARRIVE,"wife_vault"},
  {S::SECURE_CHILDREN,"wife_shop"},{S::NCD_ARRIVAL,"wife_shop"},{S::NCD_DEMAND,"wife_shop"},{S::DEFENSE_ARMED,"wife_shop"},{S::PROPERTY_ATTACK,"wife_shop"},{S::HOLD_SHOP,"wife_port"},{S::VANGUARD_ARRIVAL,"wife_port"},{S::EXTRACT_FAMILY,"wife_port"},{S::EVACUATE,"wife_kids"}};
 if(const FName* W=WifeStations.Find(V))PlaceNPC(WIFE,*W);else HideNPC(WIFE,true);
 if(auto* W=NPC(WIFE)){W->SetArmed(int32(V)>=int32(S::LOCALS_ARRIVE));W->Health->SetArmor(W->bArmed?EVeilArmor::Medium:EVeilArmor::Light);}
 // Locals (spawned from the truck)
 if(int32(V)>=int32(S::SECURE_CHILDREN)&&int32(V)<=int32(S::EVACUATE)){
  if(auto* T=SpawnVehicle("truck_locals",MarkerTransform("truck_park").GetLocation(),MarkerTransform("truck_park").Rotator().Yaw,FLinearColor(.35f,.33f,.3f),false)){T->bBloodied=true;T->OnConstruction(T->GetActorTransform());T->Headlights->SetVisibility(false);}
  SpawnLocals(MarkerTransform("truck_park").GetLocation());
  const bool Fight=At(S::HOLD_SHOP,S::EVACUATE);
  PlaceNPC(SHERIFF,Fight?"sheriff_port":"sheriff_shop");PlaceNPC(MAYOR,Fight?"mayor_cover":"mayor_shop");PlaceNPC(NATHAN,Fight?"nathan_port":"nathan_monitors");
 }
 // NCD vehicles and troops at their state positions
 if(At(S::NCD_DEMAND,S::NCD_DEMAND)){for(int32 i=1;i<=3;++i){const FTransform T=MarkerTransform(FName(*FString::Printf(TEXT("ncd_gate_%d"),i)));SpawnVehicle(FName(*FString::Printf(TEXT("ncd_%d"),i)),T.GetLocation(),T.Rotator().Yaw,FLinearColor(.02f,.022f,.026f),false);}SpawnGateTroops();}
 if(V==S::DEFENSE_ARMED){
  for(int32 i=1;i<=3;++i){const FTransform T=MarkerTransform(FName(*FString::Printf(TEXT("ncd_halt_%d"),i)));SpawnVehicle(FName(*FString::Printf(TEXT("ncd_%d"),i)),T.GetLocation(),T.Rotator().Yaw,FLinearColor(.02f,.022f,.026f),false);}
  for(auto* M:MarkersWithPrefix("column_t_"))if(auto* N=SpawnNPC(NCDClass,"ncd_column",TEXT("NCD"),EVFFaction::NCD,M->GetActorLocation(),M->GetActorRotation()))N->Tags.Add("W1");
  bColumnHalted=true;
 }
 if(int32(V)>=int32(S::PROPERTY_ATTACK)&&int32(V)<=int32(S::EVACUATE)){
  for(int32 i=1;i<=3;++i){const FTransform T=MarkerTransform(FName(*FString::Printf(TEXT("ncd_halt_%d"),i)));if(auto* W=SpawnVehicle(FName(*FString::Printf(TEXT("ncd_%d"),i)),T.GetLocation(),T.Rotator().Yaw,FLinearColor(.02f,.022f,.026f),false)){W->bWrecked=true;W->OnConstruction(W->GetActorTransform());W->Headlights->SetVisibility(false);}}
 }
 // Vanguard after the fight
 if(int32(V)>=int32(S::EXTRACT_FAMILY)&&int32(V)<=int32(S::EVACUATE)){SpawnVanguard(true);}
}

void AVFMissionController::SpawnLocals(const FVector& Near){
 struct FL{FName Id;const TCHAR* Name;FLinearColor Color;bool bArmed;};
 const FL L[]={{SHERIFF,TEXT("Sheriff Mercer"),FLinearColor(.32f,.26f,.16f),true},{MAYOR,TEXT("Mayor Vale"),FLinearColor(.18f,.19f,.21f),false},{NATHAN,TEXT("Nathan Cole"),FLinearColor(.24f,.14f,.08f),true}};
 int32 i=0;for(const auto& X:L){if(NPC(X.Id))continue;SpawnNPC(LocalClass,X.Id,X.Name,EVFFaction::Local,Near+FVector(0,-250+250*i++,0),FRotator::ZeroRotator,[X](AVFIntroNPC* N){N->ClothingColor=X.Color;N->bEssential=true;N->bArmed=X.bArmed;});}
}
void AVFMissionController::SpawnGateTroops(){
 for(auto* M:MarkersWithPrefix("gate_t_"))if(auto* N=SpawnNPC(NCDClass,"ncd_gate",TEXT("NCD"),EVFFaction::NCD,M->GetActorLocation(),M->GetActorRotation()))N->Tags.Add("Gate");
}
void AVFMissionController::SpawnVanguard(bool bSettled){
 const struct{FName Id;const TCHAR* Start;const TCHAR* Stop;}Sides[]={{"vg_1",TEXT("vg_w_start"),TEXT("vg_w_stop")},{"vg_2",TEXT("vg_e_start"),TEXT("vg_e_stop")}};
 int32 Count=0;
 for(const auto& Side:Sides){
  const FTransform T=MarkerTransform(bSettled?FName(Side.Stop):FName(Side.Start));auto* V=SpawnVehicle(Side.Id,T.GetLocation(),T.Rotator().Yaw,FLinearColor(.13f,.12f,.08f),false);
  if(!V||!bSettled)continue;
  for(int32 i=0;i<3;++i){const FName Id=Count==0?VGLEAD:FName("vanguard");auto* N=SpawnNPC(VanguardClass,Id,Count==0?TEXT("Vanguard Lead"):TEXT("Vanguard"),EVFFaction::Vanguard,V->Dismount(i),V->GetActorRotation());++Count;
   if(N&&Id!=VGLEAD)if(auto* AI=Cast<AVFIntroAIController>(N->GetController()))AI->Idle(FRotator(0,FMath::FRandRange(0.f,360.f),0));}
 }
 if(bSettled){PlaceNPC(VGLEAD,"vg_meet");bVanguardIn=true;}
}

// ---------------------------------------------------------------- state machine
void AVFMissionController::JumpToState(EVFIntroState Target){
 ++Epoch;++LineSerial;bRestarting=false;CleanupSpawned();ArrangeWorld(Target);SetStateValue(Target);Enter(Target);
}
void AVFMissionController::Go(EVFIntroState Next){if(int32(Next)<=int32(State)&&State!=S::NORMAL_HOME)return;SetStateValue(Next);Enter(Next);}
void AVFMissionController::RestartCheckpoint(){
 const int32 W=CheckpointWave;JumpToState(CheckpointState);
 if(CheckpointState==S::HOLD_SHOP&&W>1){++Epoch;CleanupSpawned();ArrangeWorld(S::HOLD_SHOP);SetStateValue(S::HOLD_SHOP);Objective(TEXT("Hold the shop."));BeginWave(W);}
}
void AVFMissionController::ResetPropertyFight(){JumpToState(S::HOLD_SHOP);}
void AVFMissionController::DebugVault(bool bOpen){if(auto* D=Prop("vault_door"))D->SetOpen(bOpen);}

void AVFMissionController::WatchPlayer(){
 auto* P=Player();if(!P)return;
 if(P->Health->State.Life==EVeilLife::Dead&&!bRestarting){
  bRestarting=true;if(auto* C=PC())if(C->HUD){C->HUD->FadeTo(1,1.2f);C->HUD->ShowSubtitle(TEXT(""),TEXT("Restarting from the last checkpoint..."),3);}
  const int32 E=Epoch;FTimerHandle H;GetWorldTimerManager().SetTimer(H,FTimerDelegate::CreateWeakLambda(this,[this,E]{if(E==Epoch)RestartCheckpoint();}),2.8f,false);return;
 }
 const FVector L=P->GetActorLocation();
 if(State==S::NORMAL_HOME&&GetWorld()->GetTimeSeconds()-StateStart>300)TryAlert(true);
 if(State==S::MOVE_TO_SHOP){bool bF=false;const FTransform T=MarkerTransform("player_shop",&bF);if(bF&&FVector::Dist2D(L,T.GetLocation())<450)Go(S::OPEN_VAULT);}
 if(State==S::LOCALS_ARRIVE&&bTruckStopped&&!bStandoff){bool bF=false;const FTransform T=MarkerTransform("player_baydoor",&bF);if(bF&&FVector::Dist2D(L,T.GetLocation())<700){bStandoff=true;LocalsStandoff();}}
 if(State==S::HOLD_SHOP&&Wave==3&&!bEastHinted){bool bF=false;const FTransform T=MarkerTransform("player_east",&bF);if(bF&&FVector::Dist2D(L,T.GetLocation())<300){bEastHinted=true;Hint(TEXT("Good angle. Lean with Q/E at the window frame."));}}
 if(P->bArmed&&P->Health->State.Health<60&&P->Combat->State.MedicalCharges>0&&!bHealHinted){bHealHinted=true;Hint(TEXT("[G] Use a medical kit. Do it behind hard cover."));}
}
void AVFMissionController::TryAlert(bool bForce){
 if(State!=S::NORMAL_HOME||bAlertQueued)return;
 int32 Family=0;for(const FName& N:{WIFE,OLDEST,MIDDLE,YOUNGEST})if(Talked.Contains(FName(*(FString("talk.")+N.ToString()))))++Family;
 if(!bForce&&(Family<3||!Talked.Contains("talk.wife")))return;
 bAlertQueued=true;After(bForce?0.5f:6.f,[this]{Go(S::EMERGENCY_ALERT);});
}

bool AVFMissionController::Talkable(const AVFIntroNPC* N)const{
 if(!N||N->IsHidden()||!N->IsAlive())return false;const FName T(*(FString("talk.")+N->RoleId.ToString()));
 switch(State){
 case S::NORMAL_HOME:return (N->Faction==EVFFaction::Family)&&!Talked.Contains(T);
 case S::LOCALS_ARRIVE:return N->RoleId==NATHAN&&bLocalsInside&&!Talked.Contains(T);
 case S::SECURE_CHILDREN:return N->RoleId==OLDEST&&bKidsInVault&&!bRadioGiven;
 default:return false;
 }
}
FText AVFMissionController::TalkPromptFor(const AVFIntroNPC* N)const{
 if(State==S::SECURE_CHILDREN&&N->RoleId==OLDEST)return FText::FromString(TEXT("Give your oldest the radio"));
 return FText::FromString(TEXT("Talk to ")+N->DisplayName);
}
bool AVFMissionController::CanUse(const AVFInteractable* I)const{
 const FName Id=I->InteractionId;const FString Str=Id.ToString();
 if(Str.StartsWith("home."))return !Talked.Contains(Id);
 if(Str.StartsWith("equip."))return State==S::EQUIP;
 // The keypad doubles as the vault door control (code entry, then seal, then open).
 if(Id=="vault.keypad")return State==S::OPEN_VAULT||(State==S::SECURE_CHILDREN&&bRadioGiven)||(State==S::EXTRACT_FAMILY&&!bVaultOpening);
 if(Id=="shop.monitors")return State==S::NCD_ARRIVAL?bCamerasReady:(int32(State)<int32(S::EMERGENCY_ALERT)&&!Talked.Contains(Id));
 if(Id=="shop.workbench")return int32(State)<int32(S::EMERGENCY_ALERT)&&!Talked.Contains(Id);
 if(Id=="panel.road"){const auto* P=Cast<AVFDefensePanel>(I);return (State==S::LOCALS_ARRIVE&&bLocalsBriefed)||(State==S::NCD_DEMAND&&bRefused&&P&&P->DefenseState==EVFDefenseState::Safe)||(State==S::DEFENSE_ARMED&&P&&P->bTriggerReady);}
 if(Id=="panel.intercom")return State==S::NCD_DEMAND&&bDemandDone&&!bRefused;
 if(Id=="panel.bay")return State==S::PROPERTY_ATTACK&&bWallReady;
 if(Id=="evac.board")return State==S::EVACUATE&&bEvacReady;
 return true;
}

void AVFMissionController::OnInteract(AActor* Source,FName Id,APawn* User){
 UE_LOG(LogTemp,Display,TEXT("VF_INTRO_INTERACT %s state=%s"),*Id.ToString(),*StateName(State));
 const FString Str=Id.ToString();
 if(Str.StartsWith("talk.")){
  Talked.Add(Id);
  // The wife's conversation is cut off by the national alert; the sons are optional beforehand.
  if(State==S::NORMAL_HOME&&Id=="talk.wife"){bAlertQueued=true;Lines("talk_wife",[this]{Go(S::EMERGENCY_ALERT);});return;}
  if(State==S::NORMAL_HOME){Lines(FName(*Str.Replace(TEXT("."),TEXT("_"))));return;}
  if(Id=="talk.nathan"){Lines("locals_nathan_aside");return;}
  if(Id=="talk.son_oldest"&&State==S::SECURE_CHILDREN){bRadioGiven=true;KeypadPrompt();Lines("radio_handoff",[this]{Objective(TEXT("Seal the vault."));});return;}
  return;
 }
 if(Str.StartsWith("home.")){Talked.Add(Id);Lines(FName(*Str.Replace(TEXT("."),TEXT("_"))));return;}
 if(Id=="shop.workbench"||(Id=="shop.monitors"&&int32(State)<int32(S::EMERGENCY_ALERT))){Talked.Add(Id);Lines(FName(*Str.Replace(TEXT("."),TEXT("_"))));return;}
 if(Id=="vault.keypad"&&State==S::OPEN_VAULT){if(auto* C=PC())C->OpenKeypad();return;}
 if(Str.StartsWith("equip.")){
  auto* P=Player();if(!P)return;
  if(Id=="equip.rifle"){P->SetArmed(true);Hint(TEXT("[LMB] Fire   [RMB] Aim down sights   [R] Reload"));}
  else if(Id=="equip.sidearm"){Hint(TEXT("[1] Rifle   [3] Sidearm"));}
  else if(Id=="equip.armor"){P->Health->SetArmor(EVeilArmor::Medium);}
  else if(Id=="equip.ammo"){P->Combat->Restock();}
  else if(Id=="equip.medical"){P->Combat->State.MedicalCharges=4;Hint(TEXT("[G] Medical kit (4)"));}
  Gear.Add(Id);Lines(FName(*Str.Replace(TEXT("."),TEXT("_"))));CheckEquip();return;
 }
 if(Id=="panel.road"){
  auto* Panel=Cast<AVFDefensePanel>(Source);
  if(State==S::LOCALS_ARRIVE){bLocalsBriefed=false;Lines("panel_explosives",[this]{Go(S::SECURE_CHILDREN);});return;}
  if(State==S::NCD_DEMAND&&Panel){Panel->SetDefenseState(EVFDefenseState::Armed);for(TActorIterator<AVFScriptedDamageZone> It(GetWorld());It;++It)It->SetArmed(true);Go(S::DEFENSE_ARMED);return;}
  if(State==S::DEFENSE_ARMED&&Panel&&Panel->bTriggerReady){Panel->bTriggerReady=false;Panel->SetDefenseState(EVFDefenseState::Triggered);Go(S::PROPERTY_ATTACK);return;}
  return;
 }
 if(Id=="panel.intercom"){bRefused=true;Lines("ncd_refusal",[this]{StartColumn();});return;}
 if(Id=="panel.bay"){bWallReady=false;if(auto* P=Cast<AVFDefensePanel>(Source))P->SetDefenseState(EVFDefenseState::Triggered);FoldWall();return;}
 if(Id=="shop.monitors"&&State==S::NCD_ARRIVAL){
  bCamerasReady=false;auto* C=PC();bool bF=false;const FTransform T=MarkerTransform("cam_gate",&bF);
  if(C&&bF){auto* Cam=GetWorld()->SpawnActor<ACameraActor>(ACameraActor::StaticClass(),T);Spawned.Add(Cam);C->SetViewTargetWithBlend(Cam,.4f);
   After(7.f,[this]{if(auto* C2=PC())C2->SetViewTargetWithBlend(Player(),.4f);});}
  Lines("ncd_cameras",[this]{Go(S::NCD_DEMAND);});return;
 }
 if(Id=="vault.keypad"){
  if(State==S::SECURE_CHILDREN){if(auto* D=Prop("vault_door"))D->SetOpen(false);Lines("vault_sealed",[this]{Go(S::NCD_ARRIVAL);});return;}
  if(State==S::EXTRACT_FAMILY){bVaultOpening=true;if(auto* D=Prop("vault_door"))D->SetOpen(true);After(1.8f,[this]{
   int32 i=1;for(const FName& K:Sons()){if(auto* N=NPC(K))N->bHuddle=false;SendNPC(K,FName(*FString::Printf(TEXT("kids_shop_%d"),i++)));}SendNPC(WIFE,"wife_kids");
   After(3.5f,[this]{Lines("family_reunion",[this]{Go(S::EVACUATE);});});});return;}
 }
 if(Id=="evac.board"){Board();return;}
}

void AVFMissionController::OnKeypadResult(bool bCorrect){
 if(State!=S::OPEN_VAULT)return;
 if(!bCorrect){Lines("vault_wrong_code");return;}
 if(auto* D=Prop("vault_door")){D->SetOpen(true);}
 Lines("vault_open",[this]{Go(S::EQUIP);});
}
void AVFMissionController::CheckEquip(){
 if(State!=S::EQUIP||bGearDone)return;for(const FName& G:RequiredGear())if(!Gear.Contains(G))return;
 bGearDone=true;Lines("equip_done",[this]{Go(S::LOCALS_ARRIVE);});
}

void AVFMissionController::Enter(EVFIntroState V){
 auto* C=PC();
 switch(V){
 case S::NORMAL_HOME:
  bAlertQueued=false;Gear.Reset();bGearDone=false;
  if(C&&C->HUD){C->HUD->FadeTo(1,0);C->HUD->FadeTo(0,2.5f);}
  After(1.5f,[this]{Lines("opening");});
  break;
 case S::EMERGENCY_ALERT:
  for(const FName& K:Sons())if(auto* N=NPC(K))N->bAlert=true;
  if(auto* TV=Item("home.tv"))TV->SetScreenText(TEXT("NATIONAL EMERGENCY ALERT\nSHELTER IN PLACE IMMEDIATELY"),FLinearColor(1,.2f,.15f));
  if(AlertLoaded)for(int32 i=0;i<3;++i)After(i*2.7f,[this]{UGameplayStatics::PlaySound2D(this,AlertLoaded,.8f);});
  if(C&&C->HUD)C->HUD->ShowAlert(true);
  SendNPC(WIFE,"wife_living");
  After(11.f,[this]{if(auto* C2=PC())if(C2->HUD)C2->HUD->ShowAlert(false);Lines("alert_reaction",[this]{
   int32 i=1;for(const FName& K:Sons())SendNPC(K,FName(*FString::Printf(TEXT("hallway_%d"),i++)));Go(S::MOVE_TO_SHOP);});});
  break;
 case S::MOVE_TO_SHOP:
  Checkpoint();Objective(TEXT("Go to the shop."));
  SendNPC(WIFE,"wife_vault",nullptr,true);
  if(auto* D=Prop("house_door"))D->SetOpen(true);
  break;
 case S::OPEN_VAULT:
  Objective(TEXT("Open the vault."));SendNPC(WIFE,"wife_vault",[this]{Lines("vault_code");},true);
  break;
 case S::EQUIP:
  Checkpoint();Objective(TEXT("Gear up: rifle, sidearm, armor, ammunition, radio."));Gear.Reset();bGearDone=false;
  SendNPC(WIFE,"wife_vault_in",[this]{Lines("equip_wife");if(auto* W=NPC(WIFE))W->SetArmed(true);});
  break;
 case S::LOCALS_ARRIVE:{
  Objective(TEXT(""));bTruckStopped=false;bStandoff=false;bLocalsInside=false;bLocalsBriefed=false;
  auto Points=MarkersWithPrefix("route_truck_");if(Points.IsEmpty())break;
  const FVector Start=Points[0]->GetActorLocation();auto* T=SpawnVehicle("truck_locals",Start,Points[0]->GetActorRotation().Yaw,FLinearColor(.35f,.33f,.3f),false);
  if(T){T->bBloodied=true;T->OnConstruction(T->GetActorTransform());TArray<FVector> R;for(int32 i=1;i<Points.Num();++i)R.Add(Points[i]->GetActorLocation());
   After(2.f,[this]{Lines("locals_incoming");SendNPC(WIFE,"wife_baydoor",nullptr,true);if(auto* D=Prop("bay1"))D->SetOpen(true);});
   T->Drive(R,1700,[this]{bTruckStopped=true;if(auto* Tr=Vehicle("truck_locals")){SpawnLocals(Tr->GetActorLocation()+Tr->GetActorRightVector()*260);
     int32 i=0;for(const FName& L:{SHERIFF,MAYOR,NATHAN}){if(auto* N=NPC(L)){N->SetActorLocation(Tr->Dismount(i++),false,nullptr,ETeleportType::TeleportPhysics);if(auto* AI=Cast<AVFIntroAIController>(N->GetController()))AI->Idle((DefendPoint()-N->GetActorLocation()).Rotation());}}}
    Objective(TEXT("Cover the bay door."));Hint(TEXT("[RMB] Raise your weapon / aim down sights"));},true);}
  break;}
 case S::SECURE_CHILDREN:
  Objective(TEXT(""));bKidsInVault=false;bRadioGiven=false;
  if(auto* D=Prop("gate"))D->SetOpen(false);
  Lines("children_go",[this]{
   SendNPC(WIFE,"wife_hall",[this]{
    int32 i=1;for(const FName& K:Sons())SendNPC(K,FName(*FString::Printf(TEXT("vault_%d"),i++)),[this,K]{if(auto* N=NPC(K))N->bHuddle=true;});
    SendNPC(WIFE,"wife_vault",[this]{bKidsInVault=true;Objective(TEXT("Give your oldest son the radio."));Lines("kids_in_vault");});});});
  break;
 case S::NCD_ARRIVAL:{
  Objective(TEXT(""));bCamerasReady=false;SendNPC(WIFE,"wife_shop");SendNPC(NATHAN,"nathan_monitors");
  if(auto* D=Prop("bay1"))D->SetOpen(false);
  auto Road=MarkersWithPrefix("route_ncd_");
  for(int32 i=1;i<=3;++i){
   const FTransform Gate=MarkerTransform(FName(*FString::Printf(TEXT("ncd_gate_%d"),i)));
   const FVector Start=Road.IsEmpty()?Gate.GetLocation()+FVector(0,9000,0):Road[0]->GetActorLocation()+FVector(0,1400*(i-1),0);
   if(auto* V2=SpawnVehicle(FName(*FString::Printf(TEXT("ncd_%d"),i)),Start,-90,FLinearColor(.02f,.022f,.026f),false)){TArray<FVector> R;for(int32 k=1;k<Road.Num();++k)R.Add(Road[k]->GetActorLocation()+FVector(0,1400*(i-1),0));R.Add(Gate.GetLocation());V2->Drive(R,1500,nullptr,true);}
  }
  After(9.f,[this]{SpawnGateTroops();});
  After(5.f,[this]{Lines("ncd_engines",[this]{bCamerasReady=true;Objective(TEXT("Check the camera monitors."));});});
  break;}
 case S::NCD_DEMAND:
  Checkpoint();Objective(TEXT(""));bDemandDone=false;bRefused=false;
  Lines("ncd_demand",[this]{Lines("ncd_reveal",[this]{bDemandDone=true;Objective(TEXT("Answer them on the shop intercom."));});});
  break;
 case S::DEFENSE_ARMED:
  Objective(TEXT(""));Lines("defense_armed");
  if(bColumnHalted)After(2.f,[this]{ColumnReady();});
  break;
 case S::PROPERTY_ATTACK:TriggerDriveway();break;
 case S::HOLD_SHOP:
  Objective(TEXT("Hold the shop."));
  SendNPC(WIFE,"wife_port",[this]{if(auto* N=NPC(WIFE))if(auto* AI=Cast<AVFIntroAIController>(N->GetController()))AI->HoldAt(N->GetActorLocation(),MarkerTransform("wife_port").Rotator());},true);
  SendNPC(SHERIFF,"sheriff_port",[this]{if(auto* N=NPC(SHERIFF))if(auto* AI=Cast<AVFIntroAIController>(N->GetController()))AI->HoldAt(N->GetActorLocation(),MarkerTransform("sheriff_port").Rotator());},true);
  SendNPC(NATHAN,"nathan_port",[this]{if(auto* N=NPC(NATHAN))if(auto* AI=Cast<AVFIntroAIController>(N->GetController()))AI->HoldAt(N->GetActorLocation(),MarkerTransform("nathan_port").Rotator());},true);
  SendNPC(MAYOR,"mayor_cover",nullptr,true);
  BeginWave(1);
  break;
 case S::VANGUARD_ARRIVAL:VanguardEnters();break;
 case S::EXTRACT_FAMILY:
  Objective(TEXT("Open the vault."));bVaultOpening=false;
  for(const FName& R:{WIFE,SHERIFF,NATHAN})if(auto* N=NPC(R))if(auto* AI=Cast<AVFIntroAIController>(N->GetController()))AI->Idle(N->GetActorRotation());
  break;
 case S::EVACUATE:{
  Objective(TEXT(""));bEvacReady=false;
  Lines("evac_order");
  for(int32 i=1;i<=2;++i)if(auto* V2=Vehicle(FName(*FString::Printf(TEXT("vg_%d"),i)))){
   const FTransform T=MarkerTransform(FName(*FString::Printf(TEXT("evac_%d"),i)));
   V2->Drive({T.GetLocation()},900,[this,i]{if(i!=1)return;bEvacReady=true;Objective(TEXT("Get in the Vanguard vehicle."));
    if(auto* V1=Vehicle("vg_1")){auto* B=GetWorld()->SpawnActor<AVFInteractable>(AVFInteractable::StaticClass(),V1->GetActorLocation()-V1->GetActorForwardVector()*300+FVector(0,0,120),V1->GetActorRotation());
     if(B){B->InteractionId="evac.board";B->Prompt=FText::FromString(TEXT("Climb in"));B->Mesh->SetRelativeScale3D(FVector(.6f,1.6f,.8f));VF::Paint(B->Mesh,FLinearColor(.1f,.09f,.06f));Spawned.Add(B);}}
    // Everyone else loads up.
    int32 k=0;for(const FName& R:{WIFE,OLDEST,MIDDLE,YOUNGEST,SHERIFF,MAYOR,NATHAN}){const FName Seat=(k++%2)?"vg_2":"vg_1";
     if(auto* N=NPC(R))if(auto* AI=Cast<AVFIntroAIController>(N->GetController()))if(auto* V3=Vehicle(Seat)){TWeakObjectPtr<AVFIntroNPC> W=N;AI->GoTo(V3->Dismount(k%4),[W]{if(W.IsValid())W->SetPresent(false);},true);}}
   },false);
   if(EngineLoaded&&!V2->Engine->IsPlaying())V2->Engine->Play();
  }
  break;}
 case S::SCUTTLE_PROPERTY:
  Objective(TEXT(""));
  After(5.f,[this]{LookAtHouse();Lines("scuttle_wife",[this]{ForcedPrompt=TEXT("Detonate");});});
  break;
 case S::MISSION_COMPLETE:
  ForcedPrompt.Reset();Objective(TEXT(""));
  if(C&&C->HUD){C->HUD->FadeTo(1,3.f);}
  After(3.6f,[this]{if(auto* C2=PC())if(C2->HUD)C2->HUD->ShowTitle(true);UE_LOG(LogTemp,Display,TEXT("VF_INTRO_COMPLETE"));});
  break;
 default:break;
 }
}

void AVFMissionController::LocalsStandoff(){
 Lines("locals_standoff",[this]{
  bLocalsInside=true;SendNPC(SHERIFF,"sheriff_shop");SendNPC(MAYOR,"mayor_shop");SendNPC(NATHAN,"nathan_shop");SendNPC(WIFE,"wife_shop");
  After(4.f,[this]{Lines("locals_brief",[this]{bLocalsBriefed=true;Objective(TEXT("Check the property defense panel."));});});
 });
}
void AVFMissionController::StartColumn(){
 if(auto* G=Prop("gate")){SpawnBlast(G->GetActorLocation()+FVector(0,0,100),1.4f);G->SetOpen(true);}
 for(int32 i=1;i<=3;++i)if(auto* V=Vehicle(FName(*FString::Printf(TEXT("ncd_%d"),i)))){
  TArray<FVector> R;for(auto* M:MarkersWithPrefix("route_column_"))R.Add(M->GetActorLocation());R.Add(MarkerTransform(FName(*FString::Printf(TEXT("ncd_halt_%d"),i))).GetLocation());
  After(1.f+(i-1)*2.2f,[this,V,R]{if(IsValid(V))V->Drive(R,900,[this]{if(++ColumnArrived>=3){bColumnHalted=true;if(State==S::DEFENSE_ARMED)ColumnReady();}});});
 }
 ColumnArrived=0;
 auto Spots=MarkersWithPrefix("column_t_");int32 i=0;
 for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It)if(It->ActorHasTag("Gate")&&It->IsAlive()&&Spots.IsValidIndex(i)){It->Tags.Add("W1");const FVector Goal=Spots[i++]->GetActorLocation();
  // Dismounts follow the column on foot; start them behind it so the walk stays within one nav query (240 m from the house, unseen).
  It->SetActorLocation(Goal+FVector(4500,0,100),false,nullptr,ETeleportType::TeleportPhysics);if(auto* AI=Cast<AVFIntroAIController>(It->GetController()))AI->GoTo(Goal,nullptr,true);}
 Lines("column_advancing",[this]{if(State==S::NCD_DEMAND)Objective(TEXT("Arm the road-denial system."));});
}
void AVFMissionController::ColumnReady(){
 if(State!=S::DEFENSE_ARMED)return;if(auto* P=Cast<AVFDefensePanel>(Item("panel.road")))P->bTriggerReady=true;
 Lines("defense_now",[this]{if(State==S::DEFENSE_ARMED)Objective(TEXT("TRIGGER the road-denial system."));});
}
void AVFMissionController::TriggerDriveway(){
 Objective(TEXT(""));
 TArray<AVFScriptedDamageZone*> Zones;for(TActorIterator<AVFScriptedDamageZone> It(GetWorld());It;++It)Zones.Add(*It);
 for(auto* Z:Zones)Z->Trigger(this);
 After(4.5f,[this]{
  // Survivors reorganize: column troops outside the zones plus a few dismounts from the rear of the column.
  for(auto* M:MarkersWithPrefix("w1_"))if(auto* N=SpawnNPC(NCDClass,"ncd",TEXT("NCD"),EVFFaction::NCD,M->GetActorLocation(),M->GetActorRotation())){N->Tags.Add("W1");if(auto* AI=Cast<AVFIntroAIController>(N->GetController()))AI->Idle(M->GetActorRotation());}
  Lines("after_blast",[this]{bWallReady=true;Objective(TEXT("Open the shop bay defenses."));});
 });
}
void AVFMissionController::FoldWall(){
 Objective(TEXT(""));
 if(auto* W=Prop("defense_wall")){
  const FVector Base=W->GetActorLocation();const FVector Side=W->GetActorRightVector();
  // Fictional lock releases: small scripted pops along the hinge line, then the panel folds out and down.
  for(int32 i=0;i<4;++i)After(.15f*i,[this,Base,Side,i]{SpawnBlast(Base+Side*(-300+200*i)+FVector(0,0,120),.18f,false,i==0);});
  After(.8f,[W]{if(IsValid(W))W->SetOpen(true);});
 }
 After(2.4f,[this]{Go(S::HOLD_SHOP);});
}
void AVFMissionController::BeginWave(int32 Number){
 Wave=Number;WaveStart=GetWorld()->GetTimeSeconds();const int32 E=Epoch;
 auto Spawn=[this](const FString& Prefix,FName Group,bool bGrenades,float Interval){for(auto* M:MarkersWithPrefix(Prefix))if(auto* N=SpawnNPC(NCDClass,"ncd",TEXT("NCD"),EVFFaction::NCD,M->GetActorLocation(),M->GetActorRotation()))if(auto* AI=Cast<AVFIntroAIController>(N->GetController())){AI->bCanThrowGrenades=bGrenades;AI->Assault(Group,Interval);}};
 auto Vehicles=[this](const FString& Prefix,bool bArmored,FName Group,int32 Dismounts){
  int32 i=1;for(auto* Stop:MarkersWithPrefix(Prefix+TEXT("stop_"))){
   auto Road=MarkersWithPrefix("route_wave_");const FVector Start=Road.IsEmpty()?Stop->GetActorLocation()+FVector(12000,0,0):Road[0]->GetActorLocation();
   if(auto* V=SpawnVehicle(FName(*FString::Printf(TEXT("%s%d"),*Prefix,i)),Start,180,FLinearColor(.02f,.022f,.026f),bArmored)){
    TArray<FVector> R;for(int32 k=1;k<Road.Num();++k)R.Add(Road[k]->GetActorLocation());R.Add(Stop->GetActorLocation());
    V->Drive(R,1300,[this,V,Group,Dismounts,bArmored]{for(int32 d=0;d<Dismounts;++d)if(auto* N=SpawnNPC(NCDClass,"ncd",TEXT("NCD"),EVFFaction::NCD,V->Dismount(d),V->GetActorRotation()))if(auto* AI=Cast<AVFIntroAIController>(N->GetController())){AI->bCanThrowGrenades=bArmored;AI->Assault(Group,11);}},true);
   }
   ++i;
  }
 };
 switch(Number){
 case 1:
  Hint(TEXT("[RMB] Aim through the firing ports. [Ctrl] Crouch behind the plate to break line of sight."));
  for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It)if(It->ActorHasTag("W1")&&It->IsAlive())if(auto* AI=Cast<AVFIntroAIController>(It->GetController()))AI->Assault("drive",10);
  if(LivingNCD()==0)Spawn(TEXT("w1_"),"drive",false,10);
  Lines("wave1");break;
 case 2:Checkpoint();Vehicles(TEXT("w2_"),false,"drive",2);Spawn(TEXT("w2_tree_"),"west",false,12);Lines("wave2");Hint(TEXT("[R] Reload behind the plate, not in the port."));break;
 case 3:Spawn(TEXT("w3_"),"east",true,8);Lines("wave3");SendNPC(WIFE,"wife_east",[this]{if(auto* N=NPC(WIFE))if(auto* AI=Cast<AVFIntroAIController>(N->GetController()))AI->HoldAt(N->GetActorLocation(),MarkerTransform("wife_east").Rotator());},true);Hint(TEXT("Reposition: the east window covers the side yard."));break;
 case 4:Checkpoint();Vehicles(TEXT("w4_"),true,"drive",3);Spawn(TEXT("w4_tree_"),"west",true,9);Lines("wave4");break;
 default:break;
 }
 GetWorldTimerManager().SetTimer(WaveTimer,FTimerDelegate::CreateWeakLambda(this,[this,E]{if(E==Epoch)CheckWave();}),1.f,true,4.f);
}
void AVFMissionController::CheckWave(){
 if(State!=S::HOLD_SHOP)return;const float T=GetWorld()->GetTimeSeconds()-WaveStart;const int32 Alive=LivingNCD();
 if(Wave==1&&(Alive<=1||T>55))BeginWave(2);
 else if(Wave==2&&(Alive<=2||T>70))BeginWave(3);
 else if(Wave==3&&(Alive<=2||T>60)){BeginWave(4);After(14.f,[this]{Lines("wave4_radio");});}
 else if(Wave==4&&T>32&&(Alive<=4||T>40)){GetWorldTimerManager().ClearTimer(WaveTimer);Go(S::VANGUARD_ARRIVAL);}
}
void AVFMissionController::VanguardEnters(){
 // Distant anti-armor shot from the north-east ridge; no weapon detail, only sound and impact.
 bool bF=false;const FTransform Ridge=MarkerTransform("aa_origin",&bF);
 if(bF)SpawnBlast(Ridge.GetLocation(),.35f,false,true);
 After(1.3f,[this]{AVFScriptedVehicle* Target=Vehicle("w4_1");if(!Target)Target=Vehicle("w2_1");if(Target){Target->Wreck(this);for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It)if(It->Faction==EVFFaction::NCD&&It->IsAlive()&&FVector::Dist(It->GetActorLocation(),Target->GetActorLocation())<700){VF::Kill(It->Health);OnPawnKilled(*It);}}
  Lines("vanguard_hit");});
 After(4.f,[this]{
  SpawnVanguard(false);
  const struct{FName Id;const TCHAR* Stop;}Sides[]={{"vg_1",TEXT("vg_w_stop")},{"vg_2",TEXT("vg_e_stop")}};int32 Count=0;
  for(const auto& Side:Sides)if(auto* V=Vehicle(Side.Id)){const int32 Base=Count;Count+=3;
   V->Drive({MarkerTransform(FName(Side.Stop)).GetLocation()},1900,[this,V,Base]{
    for(int32 i=0;i<3;++i){const bool Lead=Base==0&&i==0;if(auto* N=SpawnNPC(VanguardClass,Lead?VGLEAD:FName("vanguard"),Lead?TEXT("Vanguard Lead"):TEXT("Vanguard"),EVFFaction::Vanguard,V->Dismount(i),V->GetActorRotation()))
     if(auto* AI=Cast<AVFIntroAIController>(N->GetController())){AI->AdvanceToward=MarkerTransform("ncd_front").GetLocation();AI->Assault("vanguard",7);}}
    bVanguardIn=true;},true);}
  Lines("vanguard_radio");
 });
 const int32 E=Epoch;
 GetWorldTimerManager().SetTimer(WaveTimer,FTimerDelegate::CreateWeakLambda(this,[this,E]{
  if(E!=Epoch||State!=S::VANGUARD_ARRIVAL||!bVanguardIn)return;const float T=GetWorld()->GetTimeSeconds()-StateStart;
  // Vanguard finishes the remaining assault force.
  if(T>42)for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It)if(It->Faction==EVFFaction::NCD&&It->IsAlive()){PlayShot(It->GetActorLocation());VF::Kill(It->Health);OnPawnKilled(*It);break;}
  if(LivingNCD()==0){GetWorldTimerManager().ClearTimer(WaveTimer);After(2.5f,[this]{CommanderReveal();});}
 }),1.f,true,8.f);
}
void AVFMissionController::CommanderReveal(){
 Objective(TEXT(""));
 for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It)if(It->bArmed&&It->RoleId!=VGLEAD)if(auto* AI=Cast<AVFIntroAIController>(It->GetController()))AI->Idle(It->GetActorRotation());
 SendNPC(VGLEAD,"vg_meet",[this]{if(auto* L=NPC(VGLEAD))if(auto* P=Player()){L->SetActorRotation(FRotator(0,(P->GetActorLocation()-L->GetActorLocation()).Rotation().Yaw,0));}
  Lines("commander_reveal",[this]{Go(S::EXTRACT_FAMILY);});});
}
void AVFMissionController::Board(){
 auto* P=Player();auto* V=Vehicle("vg_1");if(!P||!V)return;
 P->SetRiding(V->Seat);LookAtHouse();
 for(const FName& R:{WIFE,OLDEST,MIDDLE,YOUNGEST,SHERIFF,MAYOR,NATHAN})HideNPC(R,true);
 for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It)if(It->Faction==EVFFaction::Vanguard)It->SetPresent(false);
 TArray<FVector> R;for(auto* M:MarkersWithPrefix("route_convoy_"))R.Add(M->GetActorLocation());
 if(!R.IsEmpty()){V->Drive(R,650,nullptr);if(auto* V2=Vehicle("vg_2")){TArray<FVector> R2=R;After(2.5f,[V2,R2]{if(IsValid(V2))V2->Drive(R2,650,nullptr);});}}
 Go(S::SCUTTLE_PROPERTY);
}
void AVFMissionController::OnPlayerUse(){
 if(ForcedPrompt.IsEmpty())return;
 if(State==S::SCUTTLE_PROPERTY&&!bScuttled){ForcedPrompt.Reset();Scuttle();return;}
 if(State==S::SCUTTLE_PROPERTY&&bScuttled&&!bThrown){ForcedPrompt.Reset();ThrowTrigger();return;}
}
void AVFMissionController::Scuttle(){
 bScuttled=true;
 TArray<FVector> Points;for(auto* M:MarkersWithPrefix("scuttle_"))Points.Add(M->GetActorLocation());
 if(Points.IsEmpty())Points.Add(DefendPoint());
 for(int32 i=0;i<Points.Num();++i)After(.35f*i,[this,P=Points[i],i]{SpawnBlast(P,3.2f,i%2==0,true);});
 const float Collapse=.35f*Points.Num()+.2f;
 After(Collapse,[this]{
  // Simplified collapse: tagged greybox pieces sag, tilt and drop over ~2.5 s; fires keep burning.
  CollapseAlpha=0;CollapseTargets.Reset();FRandomStream Rand(17);
  for(auto& Pair:ScuttleOrigins)if(Pair.Key.IsValid()){FTransform T=Pair.Value;const FVector Ext=Pair.Key->GetComponentsBoundingBox().GetExtent();
   T.SetLocation(T.GetLocation()-FVector(Rand.FRandRange(-80.f,80.f),Rand.FRandRange(-80.f,80.f),Ext.Z*Rand.FRandRange(.7f,1.3f)));
   T.SetRotation((T.Rotator()+FRotator(Rand.FRandRange(-25.f,25.f),Rand.FRandRange(-10.f,10.f),Rand.FRandRange(-25.f,25.f))).Quaternion());CollapseTargets.Add(Pair.Key,T);}
  GetWorldTimerManager().SetTimer(CollapseTimer,FTimerDelegate::CreateWeakLambda(this,[this]{
   CollapseAlpha=FMath::Min(1.f,CollapseAlpha+1/(30*2.5f));const float A=CollapseAlpha*CollapseAlpha;
   for(auto& Pair:CollapseTargets)if(Pair.Key.IsValid())if(const FTransform* O=ScuttleOrigins.Find(Pair.Key)){FTransform T;T.Blend(*O,Pair.Value,A);Pair.Key->SetActorTransform(T);}
   if(CollapseAlpha>=1)GetWorldTimerManager().ClearTimer(CollapseTimer);}),1/30.f,true);
 });
 After(Collapse+6.f,[this]{Lines("scuttle_after",[this]{ForcedPrompt=TEXT("Throw the trigger away");});});
}
void AVFMissionController::ThrowTrigger(){
 bThrown=true;auto* P=Player();
 if(P){FVector Eye;FRotator R;P->GetActorEyesViewPoint(Eye,R);auto* Remote=GetWorld()->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(),Eye+P->GetActorRightVector()*40+FVector(0,0,-20),R);
  if(Remote){Spawned.Add(Remote);auto* M=Remote->GetStaticMeshComponent();M->SetMobility(EComponentMobility::Movable);M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));Remote->SetActorScale3D(FVector(.12f,.06f,.04f));VF::Paint(M,FLinearColor(.05f,.05f,.05f));
   M->SetCollisionProfileName("PhysicsActor");M->SetSimulatePhysics(true);M->AddImpulse((P->GetActorRightVector()*500+FVector(0,0,350)+R.Vector()*200)*M->GetMass());M->AddAngularImpulseInDegrees(FVector(400,900,200)*M->GetMass());}}
 After(3.5f,[this]{Go(S::MISSION_COMPLETE);});
}
void AVFMissionController::KeypadPrompt(){
 auto* K=Item("vault.keypad");if(!K)return;
 const TCHAR* P=State==S::EXTRACT_FAMILY?TEXT("Open the vault"):(State==S::SECURE_CHILDREN&&bRadioGiven)?TEXT("Seal the vault"):TEXT("Enter the code");
 K->Prompt=FText::FromString(P);
}
void AVFMissionController::LookAtHouse(){
 // Controlled beat: turn the rider back toward the property; the player keeps free look afterwards.
 auto* P=Player();if(!P)return;bool b=false;const FTransform H=MarkerTransform("scuttle_02",&b);if(!b)return;FVector Eye;FRotator R;P->GetActorEyesViewPoint(Eye,R);
 if(auto* C=P->GetController())C->SetControlRotation((H.GetLocation()-Eye).Rotation());
}
