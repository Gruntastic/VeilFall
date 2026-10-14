#include "Squad.h"
#include "Coop.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "Perception/AISense_Sight.h"
#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/Composites/BTComposite_Sequence.h"
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryOption.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryInstanceBlueprintWrapper.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/TextRenderComponent.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace { const ASquadController* QueryController(const FEnvQueryInstance& Q){if(const auto* AI=Cast<ASquadController>(Q.Owner.Get()))return AI;const auto* P=Cast<APawn>(Q.Owner.Get());return P?Cast<ASquadController>(P->GetController()):nullptr;} }
ASquadCoordinator::ASquadCoordinator(){bReplicates=false;}
bool ASquadCoordinator::Report(FName Group,APawn* Target,FVector Location,double ObservedTime){if(!HasAuthority()||!IsValid(Target)||Group.IsNone()||Location.ContainsNaN()||!FMath::IsFinite(ObservedTime)||ObservedTime>GetWorld()->GetTimeSeconds()+.1)return false;auto& S=Sightings.FindOrAdd(Group);if(ObservedTime<S.Time)return false;S.Target=Target;S.Location=Location;S.Time=ObservedTime;return true;}
bool ASquadCoordinator::Recall(FName Group,FSquadSighting& Out)const{const auto* S=Sightings.Find(Group);if(!S||!S->Fresh(GetWorld()->GetTimeSeconds()))return false;Out=*S;return true;}
bool ASquadCoordinator::Available(FName Id,const ASquadController* User)const{const auto* R=Reservations.Find(Id);return !R||!R->User.IsValid()||R->Until<GetWorld()->GetTimeSeconds()||R->User.Get()==User;}
bool ASquadCoordinator::Reserve(FName Id,ASquadController* User){if(!HasAuthority()||Id.IsNone()||!IsValid(User)||!Available(Id,User))return false;Release(User);FReservation R;R.User=User;R.Until=GetWorld()->GetTimeSeconds()+6;Reservations.Add(Id,R);return true;}
void ASquadCoordinator::Release(const ASquadController* User){for(auto It=Reservations.CreateIterator();It;++It)if(!It.Value().User.IsValid()||It.Value().User.Get()==User)It.RemoveCurrent();}
ASquadCover::ASquadCover(){auto* Root=CreateDefaultSubobject<USceneComponent>("Root");SetRootComponent(Root);}
ASquadEnemy::ASquadEnemy(){AIControllerClass=ASquadController::StaticClass();AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;Health->Team=EVeilTeam::Continuity;Health->bAllowDowned=false;GetCharacterMovement()->MaxWalkSpeed=300;GetCharacterMovement()->NavAgentProps.bCanCrouch=true;Label=CreateDefaultSubobject<UTextRenderComponent>("EnemyLabel");Label->SetupAttachment(GetRootComponent());Label->SetRelativeLocation(FVector(0,0,110));Label->SetWorldSize(12);}
void ASquadEnemy::BeginPlay(){Super::BeginPlay();if(!HasAuthority())return;Health->SetArmor(EVeilArmor::Light);if(SquadRole==ESquadRole::Marksman){Combat->SwapPrimary();Combat->ServerEquip_Implementation(0);}if(auto* S=GetWorld()->GetGameState<ACoopState>())if(S->Record.KilledEnemies.Contains(StableId)){Health->State.Health=0;Health->State.Life=EVeilLife::Dead;Health->OnRep_State();ForceNetUpdate();}}
void ASquadEnemy::RefreshPresentation(){Super::RefreshPresentation();if(Health&&Health->State.Life==EVeilLife::Alive)GetCharacterMovement()->MaxWalkSpeed=300;}
bool ASquadEnemy::ActiveEncounter()const{const auto* S=GetWorld()->GetGameState<ACoopState>();return S&&(Encounter=="relay"?S->Record.NextObjective>=2:Encounter=="convoy"?S->Record.NextObjective>=5:false);}
void ASquadEnemy::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ASquadEnemy,TacticalStatus);}
void ASquadEnemy::Tick(float D){Super::Tick(D);Label->SetWorldRotation((GetWorld()->GetFirstPlayerController()?GetWorld()->GetFirstPlayerController()->GetControlRotation()+FRotator(0,180,0):GetActorRotation()));const FString Name=SquadRole==ESquadRole::Rifleman?"Rifleman":SquadRole==ESquadRole::Support?"Support gunner":"Marksman";Label->SetText(FText::FromString("Continuity | "+Name+" | "+(Health->State.Life==EVeilLife::Dead?FString("Dead"):TacticalStatus)));if(HasAuthority()&&Health->State.Life==EVeilLife::Dead&&GetWorld()->GetTimeSeconds()>=NextCasualtyRetry){NextCasualtyRetry=GetWorld()->GetTimeSeconds()+1;if(auto* S=GetWorld()->GetGameState<ACoopState>())S->RegisterCasualty(StableId);}}

USquadTacticsTask::USquadTacticsTask(){NodeName="VeilFall squad tactics";bNotifyTick=true;bCreateNodeInstance=true;}
EBTNodeResult::Type USquadTacticsTask::ExecuteTask(UBehaviorTreeComponent& C,uint8* M){return Cast<ASquadController>(C.GetAIOwner())?EBTNodeResult::InProgress:EBTNodeResult::Failed;}
void USquadTacticsTask::TickTask(UBehaviorTreeComponent& C,uint8* M,float D){if(auto* AI=Cast<ASquadController>(C.GetAIOwner()))AI->StepTactics();}
USquadCoverGenerator::USquadCoverGenerator(){ItemType=UEnvQueryItemType_Point::StaticClass();}
void USquadCoverGenerator::GenerateItems(FEnvQueryInstance& Q)const{const auto* AI=QueryController(Q);if(!AI)return;for(TActorIterator<ASquadCover> It(AI->GetWorld());It;++It){FVector Location;if(AI->CoverCandidate(*It,Location))Q.AddItemData<UEnvQueryItemType_Point>(Location);}}
USquadCoverScore::USquadCoverScore(){ValidItemType=UEnvQueryItemType_Point::StaticClass();TestPurpose=EEnvTestPurpose::Score;ScoringEquation=EEnvTestScoreEquation::Linear;SetWorkOnFloatValues(true);}
void USquadCoverScore::RunTest(FEnvQueryInstance& Q)const{const auto* AI=QueryController(Q);if(!AI)return;for(FEnvQueryInstance::ItemIterator It(this,Q);It;++It)It.SetScore(TestPurpose,EEnvTestFilterType::Range,AI->CoverScore(GetItemLocation(Q,It.GetIndex())),0,1);}

ASquadController::ASquadController(){Perception=CreateDefaultSubobject<UAIPerceptionComponent>("Perception");SetPerceptionComponent(*Perception);Sight=CreateDefaultSubobject<UAISenseConfig_Sight>("Sight");Sight->SightRadius=2200;Sight->LoseSightRadius=2600;Sight->PeripheralVisionAngleDegrees=75;Sight->SetMaxAge(8);Sight->DetectionByAffiliation.bDetectEnemies=true;Sight->DetectionByAffiliation.bDetectFriendlies=true;Sight->DetectionByAffiliation.bDetectNeutrals=true;Perception->ConfigureSense(*Sight);Perception->SetDominantSense(UAISense_Sight::StaticClass());Perception->OnTargetPerceptionUpdated.AddDynamic(this,&ASquadController::SightUpdated);PrimaryActorTick.bCanEverTick=true;}
void ASquadController::OnPossess(APawn* InPawn){Super::OnPossess(InPawn);if(!HasAuthority())return;Home=InPawn->GetActorLocation();RuntimeTree=NewObject<UBehaviorTree>(this);auto* Root=NewObject<UBTComposite_Sequence>(RuntimeTree);auto* Task=NewObject<USquadTacticsTask>(RuntimeTree);FBTCompositeChild Child;Child.ChildTask=Task;Root->Children.Add(Child);RuntimeTree->RootNode=Root;bBehaviorTreeRunning=RunBehaviorTree(RuntimeTree);UE_LOG(LogTemp,Log,TEXT("G3_BT_STARTED pawn=%s success=%d"),*InPawn->GetName(),bBehaviorTreeRunning);
 CoverQuery=NewObject<UEnvQuery>(this);auto* Option=NewObject<UEnvQueryOption>(CoverQuery);Option->Generator=NewObject<USquadCoverGenerator>(Option);Option->Tests.Add(NewObject<USquadCoverScore>(Option));CoverQuery->GetOptionsMutable().Add(Option);
}
void ASquadController::OnUnPossess(){if(auto* C=Coordinator())C->Release(this);Super::OnUnPossess();}
void ASquadController::Tick(float D){Super::Tick(D);if(HasAuthority()&&!bBehaviorTreeRunning)StepTactics();}
ASquadCoordinator* ASquadController::Coordinator()const{for(TActorIterator<ASquadCoordinator> It(GetWorld());It;++It)return *It;return nullptr;}
void ASquadController::SightUpdated(AActor* Actor,FAIStimulus Stimulus){if(!HasAuthority())return;auto* Player=Cast<ACoopCharacter>(Actor);auto* Enemy=Cast<ASquadEnemy>(GetPawn());if(!Player||!Enemy)return;if(Stimulus.WasSuccessfullySensed()&&Player->Health->State.Life==EVeilLife::Alive&&LineOfSightTo(Player)){VisibleTarget=Player;Memory.Target=Player;Memory.Location=Player->GetActorLocation();Memory.Time=GetWorld()->GetTimeSeconds();if(auto* C=Coordinator())C->Report(Enemy->Encounter,Player,Memory.Location,Memory.Time);}else if(VisibleTarget.Get()==Player)VisibleTarget.Reset();}
bool ASquadController::HasVisibleTarget()const{const auto* P=Cast<ACoopCharacter>(VisibleTarget.Get());return P&&GetPawn()&&P->Health->State.Life==EVeilLife::Alive&&FVector::DistSquared(P->GetActorLocation(),GetPawn()->GetActorLocation())<=FMath::Square(Sight->LoseSightRadius)&&FVector::DotProduct(GetControlRotation().Vector(),(P->GetActorLocation()-GetPawn()->GetActorLocation()).GetSafeNormal())>=FMath::Cos(FMath::DegreesToRadians(Sight->PeripheralVisionAngleDegrees))&&LineOfSightTo(P);}
bool ASquadController::CoverCandidate(const ASquadCover* Cover,FVector& Out)const{const auto* Enemy=Cast<ASquadEnemy>(GetPawn());auto* C=Coordinator();if(!Enemy||!Cover||!Memory.Fresh(GetWorld()->GetTimeSeconds())||Cover->Encounter!=Enemy->Encounter||(C&&!C->Available(Cover->StableId,this))||FVector::DistSquared(Cover->GetActorLocation(),Home)>FMath::Square(3500.f))return false;auto* Nav=FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());FNavLocation Location;if(!Nav||!Nav->ProjectPointToNavigation(Cover->GetActorLocation(),Location,FVector(100,100,200)))return false;
 FCollisionQueryParams Q(SCENE_QUERY_STAT(G3Cover),false,GetPawn());FHitResult Hit;if(!GetWorld()->LineTraceSingleByChannel(Hit,Location.Location+FVector(0,0,80),Memory.Location+FVector(0,0,64),ECC_Visibility,Q)||Hit.GetActor()==Memory.Target.Get())return false;Out=Location.Location;return true;}
float ASquadController::CoverScore(const FVector& L)const{const auto* E=Cast<ASquadEnemy>(GetPawn());if(!E)return 0;const float ThreatDistance=FVector::Dist2D(L,Memory.Location);const float HomeDistance=FVector::Dist2D(L,Home);if(E->SquadRole==ESquadRole::Support)return 1.f-FMath::Clamp(HomeDistance/3500.f,0.f,1.f);if(E->SquadRole==ESquadRole::Marksman)return FMath::Clamp(ThreatDistance/3500.f,0.f,1.f);const FVector Forward=(Memory.Location-Home).GetSafeNormal2D();const FVector Offset=(L-Home).GetSafeNormal2D();return FMath::Clamp(FMath::Abs(FVector::CrossProduct(Forward,Offset).Z)*.7f+(1-FMath::Clamp(ThreatDistance/3500.f,0.f,1.f))*.3f,0.f,1.f);}
void ASquadController::StartCoverQuery(){if(PendingQuery)return;LastQueryStart=GetWorld()->GetTimeSeconds();PendingQuery=UEnvQueryManager::RunEQSQuery(GetWorld(),CoverQuery,this,EEnvQueryRunMode::AllMatching,nullptr);if(!PendingQuery)CoverFallback();}
void ASquadController::SelectCover(const FVector& L){auto* C=Coordinator();ASquadCover* Closest=nullptr;double Best=150;for(TActorIterator<ASquadCover> It(GetWorld());It;++It){FVector P;if(CoverCandidate(*It,P)){const double D=FVector::Dist(P,L);if(D<Best){Best=D;Closest=*It;}}}if(!Closest||!C||!C->Reserve(Closest->StableId,this))return;ReservedCover=Closest->StableId;const auto Result=MoveToLocation(L,60,true,true,false,false,nullptr,false);if(auto* E=Cast<ASquadEnemy>(GetPawn()))E->TacticalStatus=Result==EPathFollowingRequestResult::Failed?"Holding (path unavailable)":"Taking reserved cover";UE_LOG(LogTemp,Log,TEXT("G3_COVER_RESERVED pawn=%s cover=%s path=%d"),*GetNameSafe(GetPawn()),*ReservedCover.ToString(),int32(Result));}
void ASquadController::CoverFallback(){if(!bFallbackLogged){UE_LOG(LogTemp,Warning,TEXT("G3_AI_FALLBACK pawn=%s: no usable EQS/path cover; hold or search last observed point"),*GetNameSafe(GetPawn()));bFallbackLogged=true;}ASquadCover* Best=nullptr;FVector BestPoint;float Score=-1;for(TActorIterator<ASquadCover> It(GetWorld());It;++It){FVector L;if(CoverCandidate(*It,L)&&CoverScore(L)>Score){Score=CoverScore(L);Best=*It;BestPoint=L;}}if(Best)SelectCover(BestPoint);else if(Memory.Fresh(GetWorld()->GetTimeSeconds()))MoveToLocation(Memory.Location,150,true,true,false,false,nullptr,false);}
void ASquadController::StepTactics(){
 if(!HasAuthority())return;auto* E=Cast<ASquadEnemy>(GetPawn());if(!E)return;const double Now=GetWorld()->GetTimeSeconds();if(Now<NextThink)return;NextThink=Now+.15;
 if(E->Health->State.Life!=EVeilLife::Alive){StopMovement();if(auto* C=Coordinator())C->Release(this);if(BrainComponent)BrainComponent->StopLogic("Enemy dead");return;}
 if(!E->ActiveEncounter()){StopMovement();E->TacticalStatus="Guarding";return;}
 const bool Peeking=FMath::Fmod(Now+double(int32(E->SquadRole)),3.0)<1.2;if(Peeking)E->UnCrouch();
 // Poll actual Perception as well as callbacks, since an encounter can activate after sight acquisition.
 if(!HasVisibleTarget()){TArray<AActor*> Seen;Perception->GetCurrentlyPerceivedActors(UAISense_Sight::StaticClass(),Seen);for(auto* A:Seen)if(auto* P=Cast<ACoopCharacter>(A))if(P->Health->State.Life==EVeilLife::Alive&&LineOfSightTo(P)){VisibleTarget=P;break;}}
 if(HasVisibleTarget()){Memory.Target=VisibleTarget;Memory.Location=VisibleTarget->GetActorLocation();Memory.Time=Now;if(auto* C=Coordinator())C->Report(E->Encounter,VisibleTarget.Get(),Memory.Location,Now);}
 else {VisibleTarget.Reset();FSquadSighting Shared;if(auto* C=Coordinator())if(C->Recall(E->Encounter,Shared)&&Shared.Time>=Memory.Time)Memory=Shared;}
 if(!Memory.Fresh(Now)){StopMovement();if(!Peeking)E->Crouch();E->TacticalStatus="Guarding (sighting expired)";return;}
 if(PendingQuery){const auto* R=PendingQuery->GetQueryResult();if(R){if(R->IsSuccessful()&&!R->Items.IsEmpty()){++SuccessfulQueries;UE_LOG(LogTemp,Log,TEXT("G3_EQS_COMPLETE pawn=%s candidates=%d"),*GetNameSafe(GetPawn()),R->Items.Num());SelectCover(R->GetItemAsLocation(0));}else CoverFallback();PendingQuery=nullptr;}else if(Now-LastQueryStart>1){PendingQuery=nullptr;CoverFallback();}}
 if(Now>=NextCover){NextCover=Now+4;StartCoverQuery();}
 if(HasVisibleTarget()){
  E->UnCrouch();FVector Eye;FRotator R;E->GetActorEyesViewPoint(Eye,R);const FVector AimPoint=VisibleTarget->GetActorLocation()+FVector(0,0,20);SetControlRotation((AimPoint-Eye).Rotation());E->SetActorRotation(FRotator(0,GetControlRotation().Yaw,0));
  if(Now>=NextFire){const auto* Ammo=E->Combat->CurrentAmmo();if(Ammo&&Ammo->Magazine==0)E->Combat->ServerReload_Implementation();else if(Ammo){const auto W=E->Combat->Spec(E->Combat->EquippedId);const float Spread=E->SquadRole==ESquadRole::Marksman?.7f:E->SquadRole==ESquadRole::Support?3.f:2.f;const FVector Direction=FMath::VRandCone(GetControlRotation().Vector(),FMath::DegreesToRadians(Spread));E->Combat->ServerFire_Implementation(Direction,++ShotSequence);NextFire=Now+W.ShotInterval+(E->SquadRole==ESquadRole::Support&&ShotSequence%4==0?1.2:0);E->TacticalStatus=E->SquadRole==ESquadRole::Support?"Suppressing visible target":"Engaging visible target";}
  }
 }else{if(!Peeking)E->Crouch();E->TacticalStatus=Peeking?"Checking last sighting":"Searching last reported position";}
 if(FParse::Param(FCommandLine::Get(),TEXT("G3Debug"))){DrawDebugSphere(GetWorld(),Memory.Location,35,8,FColor::Orange,false,.2f);DrawDebugLine(GetWorld(),E->GetActorLocation(),Memory.Location,HasVisibleTarget()?FColor::Red:FColor::Yellow,false,.2f,0,2);}
}







