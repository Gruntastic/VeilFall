#include "Coop.h"
#include "Squad.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/BoxComponent.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// All setup and injected life transitions require the explicit host diagnostic flag.
// Clients still use their ordinary owned interaction/respawn RPCs; the ack only synchronizes test observers.
void ACoopCharacter::ServerDiagnosticAck_Implementation(int32 Stage){if(HasAuthority()&&GetController()&&FParse::Param(FCommandLine::Get(),TEXT("G3Smoke")))if(auto* S=GetWorld()->GetGameState<ACoopState>())S->ProbeAck(this,Stage);}
void ACoopState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ACoopState,ProbeStage);DOREPLIFETIME(ACoopState,ProbeWinner);DOREPLIFETIME(ACoopState,ProbeReviver);DOREPLIFETIME(ACoopState,ProbePickup);}
void ACoopState::PlaceProbePlayer(ACoopCharacter* P,const FVector& Position,const FVector& Target){if(!IsValid(P)||!P->GetController())return;P->SetActorLocation(Position,false,nullptr,ETeleportType::TeleportPhysics);P->GetCharacterMovement()->SetMovementMode(MOVE_Flying);P->GetCharacterMovement()->MaxFlySpeed=0;const auto R=(Target-(Position+FVector(0,0,64))).Rotation();P->GetController()->SetControlRotation(R);if(auto* PC=Cast<APlayerController>(P->GetController()))PC->ClientSetRotation(R);}
void ACoopState::ProbeAck(ACoopCharacter* P,int32 Stage){if(!HasAuthority()||!FParse::Param(FCommandLine::Get(),TEXT("G3Smoke"))||Stage!=ProbeStage||!IsValid(P)||P->IsHost())return;if(Stage==3&&P!=ProbeWinner&&P!=ProbeReviver){ProbeLateJoiner=P;bLateAck=true;}if(Stage==8&&P!=ProbeReviver&&P!=ProbeLateJoiner.Get())bReconnectAck=true;}
void ACoopState::DiagnosticTick(){
 if(!HasAuthority()||!FParse::Param(FCommandLine::Get(),TEXT("G3Smoke")))return;
 FString Mode;FParse::Value(FCommandLine::Get(),TEXT("G3Mode="),Mode);if(Mode=="AI"){AIDiagnosticTick();return;}
 const double Now=GetWorld()->GetTimeSeconds();
 if(Mode=="Restore"){if(ProbeStage<0){ReconcilePacks();if(Record.SupplyStash.Num()==1&&Record.RecoveryPacks.Num()==1&&Record.ConsumedPickups.Contains("probe.first")&&Record.ConsumedPickups.Contains("probe.disconnect")){ProbeStage=10;ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_RESTORE_SERVER_PASS stash=1 packs=1 packId=%s"),*Record.RecoveryPacks[0].PackId.ToString());}}return;}
 TArray<ACoopCharacter*> Guests;for(TActorIterator<ACoopCharacter> It(GetWorld());It;++It)if(It->GetController()&&!It->IsHost())Guests.Add(*It);
 if(ProbeStage<0&&Guests.Num()>=2){
  ProbeStage=0;ProbeTime=Now;ProbePickup=GetWorld()->SpawnActor<AFieldPickup>(FVector(2200,4000,20),FRotator(0,180,0));ProbePickup->StableId="probe.first";ProbePickup->Definition=LoadObject<UFieldLootDefinition>(nullptr,TEXT("/Game/Coop/DA_medical.DA_medical"));
  PlaceProbePlayer(Guests[0],FVector(2200,3850,88),ProbePickup->GetActorLocation());PlaceProbePlayer(Guests[1],FVector(2350,4000,88),ProbePickup->GetActorLocation());ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_PICKUP_RACE_READY"));
 }
 if(ProbeStage==0&&Record.ConsumedPickups.Contains("probe.first")){for(auto* P:Guests)if(P->FieldItems.Num()==1)ProbeWinner=P;for(auto* P:Guests)if(P!=ProbeWinner)ProbeReviver=P;if(ProbeWinner&&ProbeReviver){ProbePickup=nullptr;PlaceProbePlayer(ProbeWinner,FVector(2200,4000,88),FVector(2300,4000,100));PlaceProbePlayer(ProbeReviver,FVector(2050,4000,88),ProbeWinner->GetActorLocation()+FVector(0,0,30));ProbeWinner->Health->TrainingDown();ProbeStage=1;ProbeTime=Now;ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_REVIVE_READY winner=%s reviver=%s"),*ProbeWinner->GetName(),*ProbeReviver->GetName());}}
 else if(ProbeStage==1&&IsValid(ProbeWinner)&&ProbeWinner->Health->State.Life==EVeilLife::Alive&&ProbeWinner->Health->State.Health==50&&ProbeReviver->Combat->State.MedicalCharges==1){ProbeStage=2;ProbeWinner->Health->TrainingDown();ProbeWinner->Health->State.BleedEndTime=Now+1;ProbeWinner->ForceNetUpdate();ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_TEAM_REVIVE_PASS hp=50 charge=1 fieldRetained=%d"),ProbeWinner->FieldItems.Num());}
 else if(ProbeStage==2&&Record.RecoveryPacks.Num()==1&&ProbeWinner->FieldItems.IsEmpty()){ProbeStage=3;ProbeTime=Now;ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_RECOVERY_READY packs=1 items=1 packId=%s"),*Record.RecoveryPacks[0].PackId.ToString());}
 else if(ProbeStage==3&&bLateAck&&Guests.Num()==3){ProbeStage=4;PlaceProbePlayer(ProbeReviver,Record.RecoveryPacks[0].Location+FVector(-150,0,68),Record.RecoveryPacks[0].Location);ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_LATE_JOIN_SERVER_PASS"));}
 else if(ProbeStage==4&&Record.RecoveryPacks.IsEmpty()&&ProbeReviver->FieldItems.Num()==1&&ProbeWinner->Health->State.Life==EVeilLife::Alive){ProbeStage=5;for(TActorIterator<AFieldBank> It(GetWorld());It;++It){PlaceProbePlayer(ProbeReviver,It->GetActorLocation()+FVector(0,-150,48),It->GetActorLocation());break;}ForceNetUpdate();}
 else if(ProbeStage==5&&Record.SupplyStash.Num()==1&&ProbeReviver->FieldItems.IsEmpty()){
  ProbePickup=GetWorld()->SpawnActor<AFieldPickup>(FVector(2450,4200,20),FRotator(0,180,0));ProbePickup->StableId="probe.disconnect";ProbePickup->Definition=LoadObject<UFieldLootDefinition>(nullptr,TEXT("/Game/Coop/DA_parts.DA_parts"));PlaceProbePlayer(ProbeWinner,FVector(2450,4050,88),ProbePickup->GetActorLocation());ProbeStage=6;ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_RECOVERY_BANK_PASS stash=1 packs=0"));
 }
 else if(ProbeStage==6&&ProbeWinner->FieldItems.Num()==1&&Record.ConsumedPickups.Contains("probe.disconnect")){ProbeStage=7;ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_DISCONNECT_READY"));}
 else if(ProbeStage==7&&!IsValid(ProbeWinner)&&Record.RecoveryPacks.Num()==1&&Record.SupplyStash.Num()==1){int32 Pawns=0;for(TActorIterator<ACoopCharacter> It(GetWorld());It;++It)++Pawns;if(Pawns==3){ProbeStage=8;ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_DISCONNECT_SERVER_PASS packs=1 stash=1 playerPawns=3"));}}
 else if(ProbeStage==8&&bReconnectAck&&Guests.Num()==3){bool AmmoClean=true;for(TActorIterator<ACoopCharacter> It(GetWorld());It;++It)AmmoClean&=It->Combat->State.AcceptedShots==0&&It->Combat->ValidateAmmo();if(AmmoClean){ProbeStage=9;ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_LIFECYCLE_SERVER_PASS pickupClaims=2 stash=1 packs=1 ownershipAmmoUnchanged=1 playerCount=4"));}}
}
void ACoopCharacter::DiagnosticTick(){
 if(HasAuthority()||!IsLocallyControlled()||!GetController()||!FParse::Param(FCommandLine::Get(),TEXT("G3Smoke")))return;
 auto* S=GetWorld()->GetGameState<ACoopState>();if(!S)return;const double Now=GetWorld()->GetTimeSeconds();const bool Observer=FParse::Param(FCommandLine::Get(),TEXT("G3Observer"));
 if(!Observer&&!bOwnershipProbeSent&&S->ProbeStage>=0&&S->ProbeStage<=1){for(TActorIterator<ACoopCharacter> It(GetWorld());It;++It)if(*It!=this&&!It->IsLocallyControlled()){It->Combat->ServerFire(FVector::ForwardVector,99999);bOwnershipProbeSent=true;UE_LOG(LogTemp,Log,TEXT("G3_UNOWNED_RPC_PROBE_SENT"));break;}}
 if(S->ProbeStage==1&&S->ProbeWinner==this&&LoggedProbeStage!=1){LoggedProbeStage=1;UE_LOG(LogTemp,Log,TEXT("G3_WINNER"));}
 if(S->ProbeStage==3&&Observer){int32 Count=0;for(TActorIterator<ARecoveryPack> It(GetWorld());It;++It)if(S->Record.RecoveryPacks.ContainsByPredicate([&It](const FRecoveryRecord& R){return R.PackId==It->PackId;}))++Count;if(Count==1&&S->Record.RecoveryPacks.Num()==1){if(LoggedProbeStage!=3){LoggedProbeStage=3;UE_LOG(LogTemp,Log,TEXT("G3_LATE_JOIN_CLIENT_PASS packs=1 actors=1 stash=0"));ServerDiagnosticAck(3);}}return;}
 if(S->ProbeStage==8&&Observer&&S->Record.RecoveryPacks.Num()==1&&S->Record.SupplyStash.Num()==1){if(LoggedProbeStage!=8){LoggedProbeStage=8;UE_LOG(LogTemp,Log,TEXT("G3_RECONNECT_CLIENT_PASS stash=1 packs=1"));ServerDiagnosticAck(8);}return;}
 if(S->ProbeStage==9&&S->Record.SupplyStash.Num()==1&&S->Record.RecoveryPacks.Num()==1&&Combat->ValidateAmmo()){if(LoggedProbeStage!=9){LoggedProbeStage=9;UE_LOG(LogTemp,Log,TEXT("G3_LIFECYCLE_CLIENT_PASS stash=1 packs=1"));}return;}
 if(S->ProbeStage==10&&S->Record.SupplyStash.Num()==1&&S->Record.RecoveryPacks.Num()==1){int32 Count=0;for(TActorIterator<ARecoveryPack> It(GetWorld());It;++It)if(It->PackId==S->Record.RecoveryPacks[0].PackId)++Count;if(Count==1&&LoggedProbeStage!=10){LoggedProbeStage=10;UE_LOG(LogTemp,Log,TEXT("G3_RESTORE_CLIENT_PASS stash=1 packs=1 actors=1 packId=%s"),*S->Record.RecoveryPacks[0].PackId.ToString());}return;}
 if(Now<NextProbeInput)return;NextProbeInput=Now+.25;
 if(S->ProbeStage==4&&S->ProbeWinner==this&&Health->State.Life==EVeilLife::Dead){Combat->RequestRespawn();return;}
 AActor* Target=nullptr;
 if(S->ProbeStage==0&&!Observer)Target=S->ProbePickup;
 if(S->ProbeStage==1&&S->ProbeReviver==this)Target=S->ProbeWinner;
 if(S->ProbeStage==4&&S->ProbeReviver==this&&FieldItems.IsEmpty())for(TActorIterator<ARecoveryPack> It(GetWorld());It;++It){Target=*It;break;}
 if(S->ProbeStage==5&&S->ProbeReviver==this)for(TActorIterator<AFieldBank> It(GetWorld());It;++It){Target=*It;break;}
 if(S->ProbeStage==6&&S->ProbeWinner==this)Target=S->ProbePickup;
 if(IsValid(Target)){FVector Eye;FRotator R;GetActorEyesViewPoint(Eye,R);FVector Point=Target->GetActorLocation();if(Cast<ACoopCharacter>(Target))Point.Z+=30;GetController()->SetControlRotation((Point-Eye).Rotation());Interaction->Request();}
}

void ACoopState::AIDiagnosticTick(){
 auto* Host=Cast<ACoopCharacter>(GetWorld()->GetFirstPlayerController()?GetWorld()->GetFirstPlayerController()->GetPawn():nullptr);if(!Host)return;const double Now=GetWorld()->GetTimeSeconds();
 if(FParse::Param(FCommandLine::Get(),TEXT("G3AutoExit"))&&((ProbeStage==23&&Now-ProbeTime>2)||(ProbeStage>=20&&ProbeStage<23&&Now>45))){GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("quit"));return;}
 if(ProbeStage<0){Apply("hq.brief",Host);Apply("edge.arrive",Host);PlaceProbePlayer(Host,FVector(15000,22100,88),FVector(16000,22300,100));Host->Health->bDiagnosticSingleHitShield=true;for(TActorIterator<ASquadEnemy> It(GetWorld());It;++It)if(It->Encounter=="relay")AIInitialPositions.Add(It->StableId,It->GetActorLocation());ProbeStage=20;ProbeTime=Now;ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_AI_FIXTURE_READY"));}
 if(ProbeStage==20){int32 Queries=0,Moved=0,Remembering=0;for(TActorIterator<ASquadEnemy> It(GetWorld());It;++It)if(It->Encounter=="relay")if(auto* AI=Cast<ASquadController>(It->GetController())){Queries+=AI->SuccessfulQueries;Moved+=FVector::Dist2D(It->GetActorLocation(),AIInitialPositions.FindRef(It->StableId))>100?1:0;Remembering+=AI->Memory.Fresh(Now)?1:0;}
  if(Now>=NextAIReport){NextAIReport=Now+3;UE_LOG(LogTemp,Log,TEXT("G3_AI_PROGRESS eqs=%d moved=%d memory=%d health=%.0f life=%d stage=%d"),Queries,Moved,Remembering,Host->Health->State.Health,int32(Host->Health->State.Life),Record.NextObjective);for(TActorIterator<ASquadEnemy> It(GetWorld());It;++It)if(It->Encounter=="relay")if(auto* AI=Cast<ASquadController>(It->GetController()))UE_LOG(LogTemp,Log,TEXT("G3_AI_ROLE id=%s visible=%d shots=%d location=%s memoryAge=%.1f team=%d feedback=%s hostTeam=%d hostLoc=%s"),*It->StableId.ToString(),AI->HasVisibleTarget(),It->Combat->State.AcceptedShots,*It->GetActorLocation().ToString(),Now-AI->Memory.Time,int32(It->Health->Team),*It->Combat->State.Feedback,int32(Host->Health->Team),*Host->GetActorLocation().ToString());}
  if(Queries>=3&&Moved>=1&&Remembering==3&&Host->Health->State.Health<100&&Host->Health->State.Life==EVeilLife::Alive){
   UE_LOG(LogTemp,Log,TEXT("G3_AI_NAV_PASS eqs=%d moved=%d roleSightings=%d authoritativePlayerHP=%.0f"),Queries,Moved,Remembering,Host->Health->State.Health);
   if(FParse::Param(FCommandLine::Get(),TEXT("G3Capture")))GetWorld()->GetFirstPlayerController()->ConsoleCommand(TEXT("Shot SHOWUI"));
   const FVector Center=Host->GetActorLocation();for(int32 i=0;i<4;++i){auto* A=GetWorld()->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(A);A->SetRootComponent(B);B->RegisterComponent();B->SetCollisionProfileName("BlockAll");B->SetCanEverAffectNavigation(false);B->SetBoxExtent(i<2?FVector(10,400,300):FVector(400,10,300));A->SetActorLocation(Center+FVector(i==0?-400:i==1?400:0,i==2?-400:i==3?400:0,100));}
   ProbeStage=21;ProbeTime=Now;
  }
 }else if(ProbeStage==21&&Now-ProbeTime>1.5){for(TActorIterator<ASquadEnemy> It(GetWorld());It;++It)if(It->Encounter=="relay")if(auto* AI=Cast<ASquadController>(It->GetController())){AIMemorySnapshot.Add(It->StableId,AI->Memory.Location);AIShotSnapshot.Add(It->StableId,It->Combat->State.AcceptedShots);}
  PlaceProbePlayer(Host,FVector(1700,4000,88),FVector(2000,4000,100));ProbeStage=22;ProbeTime=Now;
 }else if(ProbeStage==22&&Now-ProbeTime>2){bool Stable=true;int32 Count=0;for(TActorIterator<ASquadEnemy> It(GetWorld());It;++It)if(It->Encounter=="relay")if(auto* AI=Cast<ASquadController>(It->GetController())){++Count;Stable&=!AI->HasVisibleTarget()&&AI->Memory.Location.Equals(AIMemorySnapshot.FindRef(It->StableId),1)&&It->Combat->State.AcceptedShots==AIShotSnapshot.FindRef(It->StableId);}
  if(Stable&&Count==3){ProbeStage=23;ProbeTime=Now;ForceNetUpdate();UE_LOG(LogTemp,Log,TEXT("G3_AI_MEMORY_PASS unseenMovementNotTracked=1 blockedShots=0 roles=3"));}
 }
}





