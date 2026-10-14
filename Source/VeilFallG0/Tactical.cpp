#include "Tactical.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameStateBase.h"
#include "Sound/SoundWave.h"
#include "UnrealClient.h"
#include "Net/UnrealNetwork.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

void FSprintClock::Advance(double Now,bool Held,bool Eligible,bool Moving){
 if(!Held)RequireRelease=false;
 if(!Active&&Now-LastQualified>8)Consecutive=0;
 if(Active){
  if(!Eligible||!Held||Now-Started>=4){Active=false;ReadyAt=Now+1;RequireRelease=Held;}
  else if(!Qualified&&Moving&&Now-Started>=.5){Qualified=true;LastQualified=Now;Consecutive=FMath::Min(3,Consecutive+1);if(Consecutive==3)BreathingUntil=Now+8;}
 }else if(Held&&Eligible&&!RequireRelease&&Now>=ReadyAt){Active=true;Qualified=false;Started=Now;}
}
namespace {
class FTacticalMove : public FSavedMove_Character {
public:
 using Super=FSavedMove_Character;
 uint8 Tactics=0;
 virtual void Clear()override{Super::Clear();Tactics=0;}
 virtual uint8 GetCompressedFlags()const override{return Super::GetCompressedFlags()|Tactics;}
 virtual bool CanCombineWith(const FSavedMovePtr& NewMove,ACharacter* C,float Delta)const override{return Tactics==static_cast<const FTacticalMove*>(NewMove.Get())->Tactics&&Super::CanCombineWith(NewMove,C,Delta);}
 virtual void SetMoveFor(ACharacter* C,float Delta,FVector const& MoveAcceleration,FNetworkPredictionData_Client_Character& Data)override{
  Super::SetMoveFor(C,Delta,MoveAcceleration,Data);auto* M=CastChecked<UTacticalMovement>(C->GetCharacterMovement());auto* P=CastChecked<ATacticalCharacter>(C);M->Aiming=P->Combat->IsADS();Tactics=(M->WantsSprint?FLAG_Custom_0:0)|(M->LeanLeft?FLAG_Custom_1:0)|(M->LeanRight?FLAG_Custom_2:0)|(M->Aiming?FLAG_Custom_3:0);
 }
 virtual void PrepMoveFor(ACharacter* C)override{Super::PrepMoveFor(C);CastChecked<UTacticalMovement>(C->GetCharacterMovement())->UpdateFromCompressedFlags(GetCompressedFlags());}
};
class FTacticalPrediction : public FNetworkPredictionData_Client_Character {
public:
 FTacticalPrediction(const UCharacterMovementComponent& M):FNetworkPredictionData_Client_Character(M){}
 virtual FSavedMovePtr AllocateNewMove()override{return FSavedMovePtr(new FTacticalMove());}
};
}
FNetworkPredictionData_Client* UTacticalMovement::GetPredictionData_Client()const{if(!ClientPredictionData)const_cast<UTacticalMovement*>(this)->ClientPredictionData=new FTacticalPrediction(*this);return ClientPredictionData;}
void UTacticalMovement::UpdateFromCompressedFlags(uint8 Flags){Super::UpdateFromCompressedFlags(Flags);WantsSprint=(Flags&FSavedMove_Character::FLAG_Custom_0)!=0;LeanLeft=(Flags&FSavedMove_Character::FLAG_Custom_1)!=0;LeanRight=(Flags&FSavedMove_Character::FLAG_Custom_2)!=0;Aiming=(Flags&FSavedMove_Character::FLAG_Custom_3)!=0;}
float UTacticalMovement::GetMaxSpeed()const{
 const auto* P=Cast<ATacticalCharacter>(CharacterOwner);if(!P)return Super::GetMaxSpeed();if(P->Health->State.Life==EVeilLife::Dead)return 0;if(P->Health->State.Life==EVeilLife::Downed)return 80;
 const bool Active=P->HasAuthority()?P->Sprint.Active:Predicted.Active;const float Base=UVeilHealthComponent::MovementSpeed(P->Health->State.Armor);return IsCrouching()?Base*.45f:Active?Base*1.45f:Base;
}
void UTacticalMovement::UpdateCharacterStateBeforeMovement(float Delta){
 auto* P=Cast<ATacticalCharacter>(CharacterOwner);if(!P||P->GetLocalRole()==ROLE_SimulatedProxy){Super::UpdateCharacterStateBeforeMovement(Delta);return;}
 const bool CrouchIntent=bWantsToCrouch;const bool Top=P->Health->State.Life==EVeilLife::Alive&&P->ChoosePeek(float(LeanRight)-float(LeanLeft),CrouchIntent)==EVeilPeek::Top;
 bWantsToCrouch=CrouchIntent&&!Top;Super::UpdateCharacterStateBeforeMovement(Delta);bWantsToCrouch=CrouchIntent;P->PoppingUp=Top&&!IsCrouching();
 const bool Forward=FVector::DotProduct(GetCurrentAcceleration().GetSafeNormal2D(),P->GetActorForwardVector())>.4f;
 if(P->IsLocallyControlled())Aiming=P->Combat->IsADS();
 const bool Eligible=P->Health->State.Life==EVeilLife::Alive&&IsMovingOnGround()&&Forward&&!IsCrouching()&&!Aiming&&!LeanLeft&&!LeanRight&&P->Combat->State.Use==EVeilUse::None;
 auto& Clock=P->HasAuthority()?P->Sprint:Predicted;Clock.Advance(P->ServerTime(),WantsSprint,Eligible,Velocity.SizeSquared2D()>FMath::Square(100.f));
}
ATacticalCharacter::ATacticalCharacter(const FObjectInitializer& Initializer):Super(Initializer.SetDefaultSubobjectClass<UTacticalMovement>(ACharacter::CharacterMovementComponentName)){
 GetCharacterMovement()->NavAgentProps.bCanCrouch=true;BreathAudio=CreateDefaultSubobject<UAudioComponent>("Breathing");BreathAudio->SetupAttachment(Camera);BreathAudio->bAutoActivate=false;BreathAudio->bOverrideAttenuation=true;BreathAudio->AttenuationOverrides.bAttenuate=true;BreathAudio->AttenuationOverrides.AttenuationShapeExtents=FVector(80);BreathAudio->AttenuationOverrides.FalloffDistance=320;BreathAudio->SetVolumeMultiplier(.25f);
}
UTacticalMovement* ATacticalCharacter::Tactics()const{return CastChecked<UTacticalMovement>(GetCharacterMovement());}
double ATacticalCharacter::ServerTime()const{return GetWorld()->GetGameState()?GetWorld()->GetGameState()->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();}
void ATacticalCharacter::BeginPlay(){Super::BeginPlay();CarbineMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/G4/Meshes/SM_Carbine.SM_Carbine"));CubeMesh=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));BreathAudio->SetSound(LoadObject<USoundWave>(nullptr,TEXT("/Game/G4/Audio/SW_HeavyBreathing.SW_HeavyBreathing")));}
void ATacticalCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(ATacticalCharacter,Sprint);DOREPLIFETIME(ATacticalCharacter,Lean);DOREPLIFETIME(ATacticalCharacter,PoppingUp);DOREPLIFETIME(ATacticalCharacter,ProbeStart);}
void ATacticalCharacter::OnRep_Sprint(){Tactics()->Predicted=Sprint;}
bool ATacticalCharacter::NearCover()const{
 FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_WorldDynamic);FCollisionQueryParams Q(SCENE_QUERY_STAT(G4Cover),false,this);const FVector Start=GetActorLocation()+FVector(0,0,65-GetCapsuleComponent()->GetScaledCapsuleHalfHeight());
 for(FVector Direction:{GetActorForwardVector(),GetActorRightVector(),-GetActorForwardVector(),-GetActorRightVector()}){FHitResult Hit;if(GetWorld()->SweepSingleByObjectType(Hit,Start,Start+Direction*90,FQuat::Identity,Objects,FCollisionShape::MakeSphere(12),Q)&&FMath::Abs(Hit.ImpactNormal.Z)<.5f)return true;}return false;
}
EVeilPeek ATacticalCharacter::ChoosePeek(float Direction,bool CrouchIntent)const{
 if(FMath::IsNearlyZero(Direction)||!NearCover())return EVeilPeek::None;
 if(!CrouchIntent)return EVeilPeek::Side;
 FCollisionObjectQueryParams Objects;Objects.AddObjectTypesToQuery(ECC_WorldStatic);Objects.AddObjectTypesToQuery(ECC_WorldDynamic);FCollisionQueryParams Q(SCENE_QUERY_STAT(G4Peek),false,this);
 const FVector Feet=GetActorLocation()-FVector(0,0,GetCapsuleComponent()->GetScaledCapsuleHalfHeight());const FVector Low=Feet+FVector(0,0,65);FVector Face=FVector::ZeroVector;float Distance=110;
 for(const FVector D:{GetActorForwardVector(),GetActorRightVector(),-GetActorForwardVector(),-GetActorRightVector()}){FHitResult Hit;if(GetWorld()->SweepSingleByObjectType(Hit,Low,Low+D*90,FQuat::Identity,Objects,FCollisionShape::MakeSphere(9),Q)&&FMath::Abs(Hit.ImpactNormal.Z)<.5f&&Hit.Distance<Distance){Face=D;Distance=Hit.Distance;}}
 if(Face.IsNearlyZero())return EVeilPeek::Side;
 FHitResult Hit;const FVector High=Feet+FVector(0,0,PopClearanceHeight());
 if(GetWorld()->SweepSingleByObjectType(Hit,High,High+Face*90,FQuat::Identity,Objects,FCollisionShape::MakeSphere(9),Q))return EVeilPeek::Side;
 const FVector Side=Low+GetActorRightVector()*FMath::Clamp(Direction,-1.f,1.f)*28;
 // Test the chosen edge at crouched exposure height; an obstructed side stays a top peek.
 return GetWorld()->SweepSingleByObjectType(Hit,Side,Side+Face*90,FQuat::Identity,Objects,FCollisionShape::MakeSphere(9),Q)?EVeilPeek::Top:EVeilPeek::Side;
}
FVector ATacticalCharacter::SafeEyeOffset(float Amount)const{
 if(!NearCover())return FVector::ZeroVector;FVector Origin;FRotator Rotation;Super::GetActorEyesViewPoint(Origin,Rotation);const FVector Desired=FRotationMatrix(FRotator(0,Rotation.Yaw,0)).GetUnitAxis(EAxis::Y)*FMath::Clamp(Amount,-1.f,1.f)*28;
 FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(G4Eye),false,this);const bool Blocked=GetWorld()->SweepSingleByChannel(Hit,Origin,Origin+Desired,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(9),Q);return Blocked?Desired*FMath::Max(0.f,Hit.Time-.02f):Desired;
}
void ATacticalCharacter::GetActorEyesViewPoint(FVector& Location,FRotator& Rotation)const{Super::GetActorEyesViewPoint(Location,Rotation);Location+=SafeEyeOffset(Lean);}
void ATacticalCharacter::Tick(float Delta){Super::Tick(Delta);DiagnosticTick();auto* M=Tactics();if(GetLocalRole()!=ROLE_SimulatedProxy){const float Direction=float(M->LeanRight)-float(M->LeanLeft);Lean=Health->State.Life==EVeilLife::Alive&&ChoosePeek(Direction,M->bWantsToCrouch)==EVeilPeek::Side?Direction:0;}if(IsLocallyControlled()){FVector Eye;FRotator R;GetActorEyesViewPoint(Eye,R);Camera->SetWorldLocation(Eye);Camera->SetRelativeRotation(FRotator(0,0,Lean*7));}
 if(CarbineMesh&&CubeMesh){const bool Carbine=Combat->EquippedId==FName("weapon.carbine");Weapon->SetStaticMesh(Carbine?CarbineMesh:CubeMesh);Weapon->SetRelativeScale3D(Carbine?FVector(.55):FVector(.5,.08,.08));WorldWeapon->SetStaticMesh(Carbine?CarbineMesh:CubeMesh);WorldWeapon->SetRelativeScale3D(Carbine?FVector(.65):FVector(.55,.08,.08));}
 Head->SetRelativeLocation(FVector(0,0,bIsCrouched?35:65));Body->SetRelativeScale3D(FVector(.4,.45,bIsCrouched?.55:.95));TArray<UBoxComponent*> Boxes;GetComponents(Boxes);for(auto* Box:Boxes){if(Box->ComponentHasTag("Head"))Box->SetRelativeLocation(FVector(0,0,bIsCrouched?35:65));if(Box->ComponentHasTag("Torso")){Box->SetBoxExtent(FVector(22,25,bIsCrouched?20:38));Box->SetRelativeLocation(FVector(0,0,bIsCrouched?0:10));}if(Box->ComponentHasTag("Limb"))Box->SetRelativeLocation(FVector(0,0,bIsCrouched?-20:-55));}
 const bool Breathing=Health->State.Life==EVeilLife::Alive&&Sprint.BreathingUntil>ServerTime();if(Breathing&&!BreathAudio->IsPlaying())BreathAudio->Play();else if(!Breathing&&BreathAudio->IsPlaying())BreathAudio->Stop();
}
void ATacticalCharacter::SetupPlayerInputComponent(UInputComponent* Input){Super::SetupPlayerInputComponent(Input);auto* E=CastChecked<UEnhancedInputComponent>(Input);TacticalContext=NewObject<UInputMappingContext>(this);auto Bind=[this,E](FKey Key,auto Start,auto Stop){auto* A=NewObject<UInputAction>(this);TacticalActions.Add(A);TacticalContext->MapKey(A,Key);E->BindAction(A,ETriggerEvent::Started,this,Start);E->BindAction(A,ETriggerEvent::Completed,this,Stop);E->BindAction(A,ETriggerEvent::Canceled,this,Stop);};Bind(EKeys::LeftShift,&ATacticalCharacter::SprintStart,&ATacticalCharacter::SprintStop);auto* CrouchAction=NewObject<UInputAction>(this);TacticalActions.Add(CrouchAction);TacticalContext->MapKey(CrouchAction,EKeys::LeftControl);TacticalContext->MapKey(CrouchAction,EKeys::C);E->BindAction(CrouchAction,ETriggerEvent::Started,this,&ATacticalCharacter::ToggleCrouch);Bind(EKeys::Q,&ATacticalCharacter::LeftStart,&ATacticalCharacter::LeftStop);Bind(EKeys::E,&ATacticalCharacter::RightStart,&ATacticalCharacter::RightStop);if(auto* PC=Cast<APlayerController>(GetController()))if(auto* LP=PC->GetLocalPlayer())ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP)->AddMappingContext(TacticalContext,2);}
void ATacticalCharacter::SprintStart(){Tactics()->WantsSprint=true;}void ATacticalCharacter::SprintStop(){Tactics()->WantsSprint=false;}void ATacticalCharacter::ToggleCrouch(){if(Health->State.Life==EVeilLife::Alive){if(Tactics()->bWantsToCrouch)UnCrouch();else Crouch();}}void ATacticalCharacter::LeftStart(){Tactics()->LeanLeft=true;}void ATacticalCharacter::LeftStop(){Tactics()->LeanLeft=false;}void ATacticalCharacter::RightStart(){Tactics()->LeanRight=true;}void ATacticalCharacter::RightStop(){Tactics()->LeanRight=false;}
ATacticalGameMode::ATacticalGameMode(){DefaultPawnClass=ATacticalCharacter::StaticClass();GameStateClass=ATacticalState::StaticClass();}


FString ATacticalState::SavePath()const{if(FParse::Param(FCommandLine::Get(),TEXT("G3Smoke"))||FParse::Param(FCommandLine::Get(),TEXT("G4Smoke"))||FParse::Param(FCommandLine::Get(),TEXT("G4Capture")))return ADeadSignalState::SavePath();return FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CampaignG4/DeadSignal.vfs"));}
void ATacticalCharacter::DiagnosticTick(){
 if(FParse::Param(FCommandLine::Get(),TEXT("G4Capture"))&&IsLocallyControlled()){if(!CapturePlaced){CapturePlaced=true;SetActorLocation(FVector(3300,4350,100));GetController()->SetControlRotation(FRotator(-4,12,0));}if(!CaptureTaken&&ServerTime()>5){CaptureTaken=true;FScreenshotRequest::RequestScreenshot(FPaths::Combine(FPaths::ProjectDir(),TEXT("Docs/Captures/G4-Benchmarks.png")),true,false);UE_LOG(LogTemp,Log,TEXT("G4_CAPTURE_REQUESTED"));}}
 if(!FParse::Param(FCommandLine::Get(),TEXT("G4Smoke")))return;
 if(HasAuthority()&&GetController()&&!ProbePlaced){ProbePlaced=true;ProbeStart=ServerTime()+2;const int32 N=FMath::Max(0,GetWorld()->GetGameState()->PlayerArray.IndexOfByKey(GetPlayerState()));SetActorLocation(FVector(6000,7000+N*1200,100));UE_LOG(LogTemp,Log,TEXT("COVER_PROBE_PLACEMENT slot=%d location=%s"),N,*GetActorLocation().ToString());GetController()->SetControlRotation(FRotator::ZeroRotator);}
 if(ProbeStart<=0)return;const double T=ServerTime()-ProbeStart;
 if(IsLocallyControlled()&&T>=0){GetController()->SetControlRotation(FRotator::ZeroRotator);SetActorRotation(FRotator::ZeroRotator);if(T<6){AddMovementInput(FVector::ForwardVector,1);Tactics()->WantsSprint=FMath::Fmod(T,2.)<.9;}else Tactics()->WantsSprint=false;if(T>8.5&&T<12)Tactics()->LeanRight=true;else Tactics()->LeanRight=false;if(T>9.5&&T<12)Crouch();else if(T>=12&&T<22)Crouch();else if(T>=22)UnCrouch();
  if(T>=12&&T<14)Tactics()->LeanRight=true;if(T>=14&&T<16)Tactics()->LeanRight=false;if(T>=16&&T<18)Tactics()->LeanRight=true;if(T>=18){Tactics()->LeanRight=false;Tactics()->LeanLeft=T<20;}
  if(T>13&&!ProbeClientTop&&PoppingUp&&!bIsCrouched){ProbeClientTop=true;UE_LOG(LogTemp,Log,TEXT("G4_CLIENT_TOP_PEEK_PASS exposed=1 crouchIntent=1"));}
  if(!ProbeClientLogged&&Sprint.Consecutive==3){ProbeClientLogged=true;UE_LOG(LogTemp,Log,TEXT("G4_CLIENT_FATIGUE_PASS count=3 remaining=%.2f"),Sprint.BreathingUntil-ServerTime());}}
 if(HasAuthority()){
  if(!ProbeSprintLogged&&Sprint.Consecutive==3&&Sprint.BreathingUntil>ServerTime()){ProbeSprintLogged=true;UE_LOG(LogTemp,Log,TEXT("G4_SERVER_SPRINT_PASS count=3 speed=%.1f max=%.1f"),GetVelocity().Size2D(),Tactics()->GetMaxSpeed());}
  if(T>=8&&!ProbeCover){ProbeCover=true;auto* Wall=GetWorld()->SpawnActor<AActor>();ProbeObstacle=Wall;auto* B=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(B);B->RegisterComponent();B->SetBoxExtent(FVector(5,80,100));B->SetCollisionProfileName("BlockAll");Wall->SetActorLocation(GetActorLocation()+FVector(70,0,0));}
  if(T>=12&&!ProbeLow){ProbeLow=true;if(ProbeObstacle)ProbeObstacle->Destroy();ProbeObstacle=GetWorld()->SpawnActor<AActor>(DiagnosticCoverClass(),FVector(GetActorLocation().X+70,GetActorLocation().Y,DiagnosticCoverHeight()/2),FRotator::ZeroRotator);}
  if(T>13&&T<14&&!ProbeTopLogged&&PoppingUp&&!bIsCrouched&&Tactics()->bWantsToCrouch){ProbeTopLogged=true;UE_LOG(LogTemp,Log,TEXT("G4_SERVER_TOP_PEEK_PASS exposed=1 crouchIntent=1"));}
  if(T>15&&T<16&&!ProbeDropLogged&&bIsCrouched&&!PoppingUp&&Tactics()->bWantsToCrouch){ProbeDropLogged=true;UE_LOG(LogTemp,Log,TEXT("G4_SERVER_PEEK_RELEASE_PASS crouched=1"));}
  if(T>=16&&!ProbeEdge){ProbeEdge=true;ProbeObstacle->SetActorLocation(ProbeObstacle->GetActorLocation()+FVector(0,-75,0));}
  if(T>17&&T<17.05)UE_LOG(LogTemp,Log,TEXT("COVER_PROBE_EDGE_STATE pawn=%s location=%s yaw=%.1f near=%d crouched=%d intent=%d lean=%.2f pop=%d choice=%d"),*GetName(),*GetActorLocation().ToString(),GetActorRotation().Yaw,NearCover(),bIsCrouched,Tactics()->bWantsToCrouch,Lean,PoppingUp,int32(ChoosePeek(1,true)));
  if(T>17&&T<18&&!ProbeEdgeLogged&&bIsCrouched&&Lean>.9&&!PoppingUp){ProbeEdgeLogged=true;UE_LOG(LogTemp,Log,TEXT("G4_SERVER_OPEN_EDGE_PASS side=1 crouched=1"));}
  if(T>10&&T<12&&!ProbeCoverLogged&&Lean>.9&&bIsCrouched&&NearCover()){ProbeCoverLogged=true;UE_LOG(LogTemp,Log,TEXT("G4_SERVER_COVER_PASS crouch=1 lean=%.1f eyeOffset=%.1f"),Lean,SafeEyeOffset(Lean).Size());}
 }
}


ATacticalProbeCover::ATacticalProbeCover(){bReplicates=true;bAlwaysRelevant=true;SetReplicateMovement(true);auto* Box=CreateDefaultSubobject<UBoxComponent>("CoverCollision");SetRootComponent(Box);Box->SetBoxExtent(FVector(5,80,55));Box->SetCollisionProfileName("BlockAll");}


float ATacticalCharacter::PopClearanceHeight()const{const auto* Defaults=GetClass()->GetDefaultObject<ACharacter>();return Defaults->GetCapsuleComponent()->GetUnscaledCapsuleHalfHeight()+Defaults->BaseEyeHeight;}
UClass* ATacticalCharacter::DiagnosticCoverClass()const{return ATacticalProbeCover::StaticClass();}

