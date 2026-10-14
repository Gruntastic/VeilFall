#include "VFIntro.h"
#include "VFIntroShared.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavigationSystem.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Engine/LocalPlayer.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

// ---------------------------------------------------------------- NPCs
AVFIntroNPC::AVFIntroNPC(const FObjectInitializer& Initializer):Super(Initializer){
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 AIControllerClass=AVFIntroAIController::StaticClass();AutoPossessAI=EAutoPossessAI::PlacedInWorldOrSpawned;
 bUseControllerRotationYaw=false;GetCharacterMovement()->bOrientRotationToMovement=true;GetCharacterMovement()->RotationRate=FRotator(0,540,0);
 GetCharacterMovement()->MaxWalkSpeed=300;GetCharacterMovement()->NavAgentProps.bCanCrouch=true;
 Health->bAllowDowned=false;
 // Clothing/vest block reads at distance in greybox (faction colour); the mannequin body comes from ACombatCharacter.
 Vest=CreateDefaultSubobject<UStaticMeshComponent>("Vest");Vest->SetupAttachment(GetCapsuleComponent());Vest->SetStaticMesh(Cube.Object);Vest->SetRelativeLocation(FVector(4,0,28));Vest->SetRelativeScale3D(FVector(.34f,.42f,.42f));Vest->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 HelmetMesh=CreateDefaultSubobject<UStaticMeshComponent>("Helmet");HelmetMesh->SetupAttachment(GetCapsuleComponent());HelmetMesh->SetStaticMesh(Sphere.Object);HelmetMesh->SetRelativeLocation(FVector(0,0,80));HelmetMesh->SetRelativeScale3D(FVector(.27f,.27f,.2f));HelmetMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
}
void AVFIntroNPC::BeginPlay(){
 Super::BeginPlay();
 if(!FMath::IsNearlyEqual(BodyScale,1.f))SetActorScale3D(FVector(BodyScale));
 Health->Team=Faction==EVFFaction::NCD?EVeilTeam::Continuity:EVeilTeam::FreeStates;
 if(HasAuthority())Health->SetArmor(Faction==EVFFaction::NCD||Faction==EVFFaction::Vanguard?EVeilArmor::Medium:EVeilArmor::Light);
 if(WeaponDamage>0){auto* D=NewObject<UVeilWeaponDefinition>(this);D->Spec=UCombatDataLibrary::DefaultWeaponSpec("weapon.carbine");D->Spec.Damage=FMath::Clamp(WeaponDamage,1.f,200.f);Combat->Definitions.Add(D);Combat->Restock();}
 VF::Paint(Vest,ClothingColor);VF::Paint(HelmetMesh,ClothingColor*.7f);HelmetMesh->SetVisibility(bHelmet);
 if(bEssential)GetWorldTimerManager().SetTimer(HealTimer,this,&AVFIntroNPC::EssentialHeal,.25f,true);
 RefreshPresentation();
}
void AVFIntroNPC::RefreshPresentation(){
 Super::RefreshPresentation();if(!Health||!WorldWeapon)return;
 WorldWeapon->SetVisibility(bArmed&&Health->State.Life==EVeilLife::Alive&&!IsHidden());
 ApplyAnimation();
 if(Health->State.Life==EVeilLife::Dead&&HasAuthority())if(auto* M=AVFMissionController::Get(this))M->OnPawnKilled(this);
}
// Armed humans use Epic's third-person rifle Anim Blueprint (UE 5.8 first-person template, Variant_Shooter);
// unarmed keep ABP_Unarmed. Deaths play one of the template death clips instead of tipping the mesh over.
void AVFIntroNPC::ApplyAnimation(){
 USkeletalMeshComponent* Body=GetMesh();if(!Body||!Body->GetSkeletalMeshAsset())return;
 const bool Dead=Health->State.Life==EVeilLife::Dead;
 Body->SetRelativeRotation(FRotator(0,-90,0));
 if(Dead){
  if(!bDeathPlayed){bDeathPlayed=true;static const TCHAR* Clips[]={TEXT("MM_Death_Back_01"),TEXT("MM_Death_Front_01"),TEXT("MM_Death_Front_02"),TEXT("MM_Death_Front_03"),TEXT("MM_Death_Left_01"),TEXT("MM_Death_Right_01")};
   const FString Name=Clips[FMath::RandRange(0,5)];
   if(auto* Clip=LoadObject<UAnimSequence>(nullptr,*FString::Printf(TEXT("/Game/Characters/Mannequins/Anims/Death/%s.%s"),*Name,*Name),nullptr,LOAD_NoWarn|LOAD_Quiet))Body->PlayAnimation(Clip,false);}
  return;
 }
 bDeathPlayed=false;
 const TCHAR* Path=bArmed?TEXT("/Game/Variant_Shooter/Anims/ABP_TP_Rifle.ABP_TP_Rifle_C"):TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed.ABP_Unarmed_C");
 UClass* Want=LoadClass<UAnimInstance>(nullptr,Path,nullptr,LOAD_NoWarn|LOAD_Quiet);
 if(Want&&(Body->GetAnimationMode()!=EAnimationMode::AnimationBlueprint||Body->GetAnimClass()!=Want)){Body->SetAnimationMode(EAnimationMode::AnimationBlueprint);Body->SetAnimInstanceClass(Want);}
 // Carbine in the right hand (socket on the UE5 mannequin) rather than floating off the capsule.
 if(bArmed&&WorldWeapon->GetAttachParent()!=Body){
  if(auto* Carbine=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/G4/Meshes/SM_Carbine.SM_Carbine"),nullptr,LOAD_NoWarn|LOAD_Quiet)){WorldWeapon->SetStaticMesh(Carbine);WorldWeapon->SetRelativeScale3D(FVector(.65f));}
  WorldWeapon->AttachToComponent(Body,FAttachmentTransformRules::SnapToTargetNotIncludingScale,"HandGrip_R");
 }
}
void AVFIntroNPC::EssentialHeal(){
 if(IsHidden())return;
 if(Health->State.Life==EVeilLife::Dead){Health->ResetHealth();RefreshPresentation();if(auto* C=Cast<AVFIntroAIController>(GetController()))C->HoldAt(GetActorLocation(),GetActorRotation());return;}
 if(Health->State.Health<55)Health->Recover(85);
}
bool AVFIntroNPC::IsAlive()const{return Health&&Health->State.Life==EVeilLife::Alive;}
bool AVFIntroNPC::IsHostileTo(const ACombatCharacter* Other)const{
 if(!Other||Other==this||!Other->Health)return false;
 const bool OtherNCD=Other->Health->Team==EVeilTeam::Continuity;
 return (Faction==EVFFaction::NCD)!=OtherNCD;
}
void AVFIntroNPC::SetArmed(bool b){bArmed=b;RefreshPresentation();}
void AVFIntroNPC::SetPresent(bool bPresent){
 SetActorHiddenInGame(!bPresent);SetActorEnableCollision(bPresent);
 if(!bPresent)if(auto* C=Cast<AAIController>(GetController()))C->StopMovement();
 RefreshPresentation();
}
bool AVFIntroNPC::Activate_Implementation(APawn* User){
 auto* M=AVFMissionController::Get(this);if(!M||!M->Talkable(this))return false;
 M->OnInteract(this,FName(*(FString("talk.")+RoleId.ToString())),User);return true;
}
FText AVFIntroNPC::GetPrompt(APawn* User)const{const auto* M=AVFMissionController::Get(this);return M&&M->Talkable(this)?M->TalkPromptFor(this):FText();}

// ---------------------------------------------------------------- AI
AVFIntroAIController::AVFIntroAIController(){PrimaryActorTick.bCanEverTick=false;}
AVFMissionController* AVFIntroAIController::Mission()const{return AVFMissionController::Get(this);}
void AVFIntroAIController::OnPossess(APawn* P){
 Super::OnPossess(P);HoldLocation=P->GetActorLocation();HoldFacing=P->GetActorRotation();
 GetWorldTimerManager().SetTimer(ThinkTimer,this,&AVFIntroAIController::Think,.2f,true,FMath::FRandRange(.05f,.25f));
}
void AVFIntroAIController::OnUnPossess(){GetWorldTimerManager().ClearTimer(ThinkTimer);Super::OnUnPossess();}
void AVFIntroAIController::GoTo(const FVector& L,TFunction<void()> OnArrive,bool bRun){
 auto* Me=Cast<AVFIntroNPC>(GetPawn());if(!Me)return;
 // Drop any in-flight move first so its completion can't consume the new callback.
 Arrive=nullptr;Mode=EVFAIMode::Idle;StopMovement();
 Mode=EVFAIMode::Move;Arrive=MoveTemp(OnArrive);HoldLocation=L;
 Me->UnCrouch();Me->GetCharacterMovement()->MaxWalkSpeed=bRun?480:230;bMoving=true;
 const auto R=MoveToLocation(L,45,true,true,true,false,nullptr,false);ActiveMove=GetCurrentMoveRequestID();
 if(R==EPathFollowingRequestResult::AlreadyAtGoal){bMoving=false;auto Done=MoveTemp(Arrive);Arrive=nullptr;if(Done)Done();}
 else if(R==EPathFollowingRequestResult::Failed){
  // Greybox safety net: never strand a story beat on a missing nav tile.
  UE_LOG(LogTemp,Warning,TEXT("VF_AI_PATH_FAILED pawn=%s goal=%s; placing directly"),*Me->GetName(),*L.ToString());
  Me->SetActorLocation(L+FVector(0,0,Me->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()),false,nullptr,ETeleportType::TeleportPhysics);bMoving=false;auto Done=MoveTemp(Arrive);Arrive=nullptr;if(Done)Done();
 }
}
void AVFIntroAIController::OnMoveCompleted(FAIRequestID Id,const FPathFollowingResult& Result){
 Super::OnMoveCompleted(Id,Result);if(Mode==EVFAIMode::Move&&Id!=ActiveMove)return;bMoving=false;
 if(Mode==EVFAIMode::Move&&Arrive){
  if(!Result.IsSuccess()&&!Result.HasFlag(FPathFollowingResultFlags::NewRequest))if(APawn* Me=GetPawn()){
   if(FVector::Dist2D(Me->GetActorLocation(),HoldLocation)>150){UE_LOG(LogTemp,Warning,TEXT("VF_AI_MOVE_INCOMPLETE pawn=%s; placing at goal"),*Me->GetName());Me->SetActorLocation(HoldLocation+FVector(0,0,90),false,nullptr,ETeleportType::TeleportPhysics);}
  }
  if(Result.HasFlag(FPathFollowingResultFlags::NewRequest))return;
  Mode=EVFAIMode::Idle;HoldFacing=GetPawn()?GetPawn()->GetActorRotation():HoldFacing;auto Done=MoveTemp(Arrive);Arrive=nullptr;if(Done)Done();
 }
}
void AVFIntroAIController::HoldAt(const FVector& L,const FRotator& Facing){Mode=EVFAIMode::Hold;HoldLocation=L;HoldFacing=Facing;Arrive=nullptr;if(FVector::Dist2D(GetPawn()?GetPawn()->GetActorLocation():L,L)>80){bMoving=true;MoveToLocation(L,30,true,true,true,false,nullptr,false);}}
void AVFIntroAIController::Assault(FName Group,float Interval){Mode=EVFAIMode::Assault;CoverGroup=Group;AdvanceEvery=Interval;NextAdvance=GetWorld()->GetTimeSeconds()+FMath::FRandRange(2.f,Interval);Cover=nullptr;Arrive=nullptr;PickCover(false);}
void AVFIntroAIController::Retreat(const FVector& L){Mode=EVFAIMode::Retreat;Arrive=nullptr;if(auto* Me=Cast<ACharacter>(GetPawn())){Me->UnCrouch();Me->GetCharacterMovement()->MaxWalkSpeed=500;}MoveToLocation(L,100,true,true,true,false,nullptr,false);}
void AVFIntroAIController::Idle(const FRotator& Facing){Mode=EVFAIMode::Idle;Arrive=nullptr;StopMovement();HoldFacing=Facing;if(auto* Me=Cast<ACharacter>(GetPawn())){Me->UnCrouch();Me->SetActorRotation(FRotator(0,Facing.Yaw,0));}}

ACombatCharacter* AVFIntroAIController::FindTarget(bool& bVisible)const{
 bVisible=false;const auto* Me=Cast<AVFIntroNPC>(GetPawn());if(!Me)return nullptr;
 TArray<TPair<float,ACombatCharacter*>> Options;
 for(TActorIterator<ACombatCharacter> It(GetWorld());It;++It){
  ACombatCharacter* O=*It;if(O==Me||O->IsHidden()||!O->Health||O->Health->State.Life!=EVeilLife::Alive||!Me->IsHostileTo(O))continue;
  if(const auto* N=Cast<AVFIntroNPC>(O))if(!N->bArmed)continue; // professionals: unarmed civilians are not targets
  const float D=FVector::DistSquared(O->GetActorLocation(),Me->GetActorLocation());if(D>FMath::Square(9000.f))continue;
  Options.Emplace(O==Target.Get()?D*.5f:D,O);
 }
 Options.Sort([](const auto& A,const auto& B){return A.Key<B.Key;});
 for(int32 i=0;i<FMath::Min(4,Options.Num());++i)if(LineOfSightTo(Options[i].Value)){bVisible=true;return Options[i].Value;}
 return Options.IsEmpty()?nullptr:Options[0].Value;
}
void AVFIntroAIController::PickCover(bool bAdvance){
 APawn* Me=GetPawn();auto* M=Mission();if(!Me||!M)return;
 const FVector Goal=AdvanceToward.IsZero()?M->DefendPoint():AdvanceToward;const FVector Here=Me->GetActorLocation();const float HereGoal=FVector::Dist2D(Here,Goal);
 AVFCoverPoint* Best=nullptr;float BestScore=TNumericLimits<float>::Max();
 for(TActorIterator<AVFCoverPoint> It(GetWorld());It;++It){
  if(It->Group!=CoverGroup||!M->CoverFree(*It,this)||*It==Cover.Get())continue;
  const float ToGoal=FVector::Dist2D(It->GetActorLocation(),Goal),ToMe=FVector::Dist2D(It->GetActorLocation(),Here);
  if(bAdvance&&ToGoal>HereGoal-500)continue;
  const float Score=bAdvance?ToMe+ToGoal*.35f:ToMe+ToGoal*.15f;
  if(Score<BestScore){BestScore=Score;Best=*It;}
 }
 if(!Best)return;
 Cover=Best;M->ReserveCover(Best,this);bMoving=true;
 if(auto* C=Cast<ACharacter>(Me)){C->UnCrouch();C->GetCharacterMovement()->MaxWalkSpeed=470;}
 MoveToLocation(Best->GetActorLocation(),35,true,true,true,false,nullptr,false);
}
void AVFIntroAIController::Fire(ACombatCharacter* T){
 auto* Me=Cast<AVFIntroNPC>(GetPawn());auto* M=Mission();if(!Me||!T||!M)return;const double Now=GetWorld()->GetTimeSeconds();
 if(Now<NextFire||Now<BurstPause)return;
 auto* Ammo=Me->Combat->CurrentAmmo();if(!Ammo)return;
 if(Ammo->Magazine==0){if(Ammo->Reserve==0)Me->Combat->Restock();else Me->Combat->ServerReload_Implementation();return;}
 if(Me->Combat->State.bReloading)return;
 // Professional pressure: only a few NCD may engage the player directly; the rest suppress the position.
 const bool bPlayer=T->IsPlayerControlled();const bool bToken=!bPlayer||Me->Faction!=EVFFaction::NCD||M->RequestShotToken(Me,T);
 FVector Eye;FRotator ER;Me->GetActorEyesViewPoint(Eye,ER);
 const FVector Aim=T->GetActorLocation()+FVector(0,0,FMath::FRandRange(0.f,45.f));const float Dist=FVector::Dist(Eye,Aim);
 const float Cone=(bToken?Accuracy:Accuracy*3.2f)*FMath::Clamp(Dist/2000.f,.6f,2.2f);
 const FRotator AimRot=(Aim-Eye).Rotation();SetControlRotation(AimRot);Me->SetActorRotation(FRotator(0,AimRot.Yaw,0));
 const FVector Dir=FMath::VRandCone(AimRot.Vector(),FMath::DegreesToRadians(Cone));
 Me->Combat->ServerFire_Implementation(Dir,++Sequence);M->PlayShot(Me->GetActorLocation());
 const auto Spec=Me->Combat->Spec(Me->Combat->EquippedId);NextFire=Now+FMath::Max(Spec.ShotInterval,.12f);
 if(++Burst>=FMath::RandRange(3,5)){Burst=0;BurstPause=Now+FMath::FRandRange(1.1f,2.4f);}
}
void AVFIntroAIController::Think(){
 auto* Me=Cast<AVFIntroNPC>(GetPawn());if(!Me)return;
 if(!Me->IsAlive()){StopMovement();if(!Me->bEssential)GetWorldTimerManager().ClearTimer(ThinkTimer);return;}
 if(Me->IsHidden())return;
 const double Now=GetWorld()->GetTimeSeconds();
 const bool bHurt=Me->Health->State.DamageRevision!=SeenDamage;SeenDamage=Me->Health->State.DamageRevision;
 bool bVisible=false;ACombatCharacter* T=nullptr;
 if(Me->bArmed&&(Mode==EVFAIMode::Hold||Mode==EVFAIMode::Assault)){T=FindTarget(bVisible);if(T!=Target.Get()){Target=T;TargetSince=Now;}}
 // Peek rhythm: exposed ~45% of the time, crouched behind cover otherwise. Hurt units drop immediately.
 const bool bPeek=!bHurt&&FMath::Fmod(Now+Me->GetUniqueID()%7,3.2)<1.45;
 switch(Mode){
 case EVFAIMode::Hold:{
  if(!bMoving&&FVector::Dist2D(Me->GetActorLocation(),HoldLocation)>120){bMoving=true;MoveToLocation(HoldLocation,30,true,true,true,false,nullptr,false);}
  if(bMoving)break;
  if(T&&bVisible&&bPeek){Me->UnCrouch();if(Now-TargetSince>.5)Fire(T);}
  else{if(T&&!bPeek)Me->Crouch();else Me->UnCrouch();if(!T)Me->SetActorRotation(FMath::RInterpTo(Me->GetActorRotation(),FRotator(0,HoldFacing.Yaw,0),.2f,6));}
  break;}
 case EVFAIMode::Assault:{
  if(bMoving){if(T&&bVisible&&FMath::FRand()<.12f&&Now-TargetSince>.8)Fire(T);break;}
  if(!Cover.IsValid()||(bHurt&&FMath::FRand()<.6f)){PickCover(false);break;}
  if(Now>=NextAdvance){NextAdvance=Now+AdvanceEvery*FMath::FRandRange(.8f,1.3f);PickCover(true);if(bMoving)break;}
  if(T&&bVisible&&bPeek){Me->UnCrouch();if(Now-TargetSince>.7)Fire(T);}else Me->Crouch();
  if(bCanThrowGrenades&&T&&T->IsPlayerControlled()&&Now>=NextGrenade&&FVector::Dist2D(T->GetActorLocation(),Me->GetActorLocation())<3200){
   NextGrenade=Now+FMath::FRandRange(14.f,22.f);
   if(auto* M=Mission()){FVector Start=Me->GetActorLocation()+FVector(0,0,90);FVector Goal=T->GetActorLocation()+FMath::VRand()*FVector(260,260,0);FVector Vel;
    if(UGameplayStatics::SuggestProjectileVelocity_CustomArc(this,Vel,Start,Goal,0,.55f)){auto* G=GetWorld()->SpawnActor<AVFGrenade>(AVFGrenade::StaticClass(),Start,FRotator::ZeroRotator);if(G){G->Launch(Vel,Me);M->Grenade(Goal,Me);}}}
  }
  break;}
 case EVFAIMode::Retreat:case EVFAIMode::Move:case EVFAIMode::Idle:default:break;
 }
}

// ---------------------------------------------------------------- grenade
AVFGrenade::AVFGrenade(){
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Sphere(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
 Mesh=CreateDefaultSubobject<UStaticMeshComponent>("Mesh");SetRootComponent(Mesh);Mesh->SetStaticMesh(Sphere.Object);Mesh->SetRelativeScale3D(FVector(.12f));
 Mesh->SetCollisionProfileName("BlockAllDynamic");Mesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);Mesh->SetCollisionResponseToChannel(ECC_Visibility,ECR_Ignore);
 Movement=CreateDefaultSubobject<UProjectileMovementComponent>("Movement");Movement->bShouldBounce=true;Movement->Bounciness=.25f;Movement->Friction=.5f;Movement->bAutoActivate=false;
}
void AVFGrenade::Launch(const FVector& V,AActor* Thrower){
 Source=Thrower;VF::Paint(Mesh,FLinearColor(.05f,.07f,.04f));Movement->Velocity=V;Movement->Activate();
 GetWorldTimerManager().SetTimer(Fuse,this,&AVFGrenade::Explode,3.2f,false);
}
void AVFGrenade::Explode(){
 const FVector P=GetActorLocation();if(auto* M=AVFMissionController::Get(this))M->SpawnBlast(P,.7f,false,true);
 for(TActorIterator<ACombatCharacter> It(GetWorld());It;++It){
  const float D=FVector::Dist(It->GetActorLocation(),P);if(D>520||!It->Health)continue;
  FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(VFGrenade),false,this);Q.AddIgnoredActor(*It);
  if(GetWorld()->LineTraceSingleByChannel(Hit,P+FVector(0,0,30),It->GetActorLocation(),ECC_WorldStatic,Q))continue; // hard cover protects
  It->Health->ApplyShot(VF::BlastSpec(85*(1-D/520)+5),EVeilRegion::Torso,this);
 }
 Destroy();
}

// ---------------------------------------------------------------- player
AVFIntroCharacter::AVFIntroCharacter(){}
void AVFIntroCharacter::BeginPlay(){
 Super::BeginPlay();Health->bAllowDowned=false;
 if(HasAuthority())Health->SetArmor(EVeilArmor::Light);
 RefreshPresentation();
}
void AVFIntroCharacter::SetupPlayerInputComponent(UInputComponent* Input){
 Super::SetupPlayerInputComponent(Input);
 // Higher-priority contexts swallow weapon keys while the protagonist is unarmed (and the unused shotgun slot always).
 auto Block=[this](TArray<FKey> Keys){auto* C=NewObject<UInputMappingContext>(this);for(const FKey& K:Keys){auto* A=NewObject<UInputAction>(this);UnarmedActions.Add(A);C->MapKey(A,K);}return C;};
 UnarmedContext=Block({EKeys::LeftMouseButton,EKeys::RightMouseButton,EKeys::R,EKeys::One,EKeys::Three,EKeys::G,EKeys::H});
 BlockedContext=Block({EKeys::Two,EKeys::Enter});
 if(auto* PC=Cast<APlayerController>(GetController()))if(auto* LP=PC->GetLocalPlayer())if(auto* Sub=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP))Sub->AddMappingContext(BlockedContext,100);
 ApplyUnarmedContext();
}
void AVFIntroCharacter::ApplyUnarmedContext(){
 auto* PC=Cast<APlayerController>(GetController());auto* LP=PC?PC->GetLocalPlayer():nullptr;auto* Sub=LP?ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP):nullptr;
 if(!Sub||!UnarmedContext)return;if(bArmed)Sub->RemoveMappingContext(UnarmedContext);else Sub->AddMappingContext(UnarmedContext,100);
}
void AVFIntroCharacter::SetArmed(bool b){bArmed=b;if(b&&HasAuthority())Combat->ServerEquip_Implementation(0);ApplyUnarmedContext();RefreshPresentation();}
void AVFIntroCharacter::RefreshPresentation(){
 Super::RefreshPresentation();if(!Health||!Weapon)return;const bool Alive=Health->State.Life==EVeilLife::Alive;
 Weapon->SetVisibility(Alive&&bArmed);WorldWeapon->SetVisibility(Alive&&bArmed);
 if(bRiding)GetCharacterMovement()->DisableMovement();
}
void AVFIntroCharacter::SetRiding(USceneComponent* Seat){
 if(Seat){bRiding=true;GetCharacterMovement()->DisableMovement();SetActorEnableCollision(false);AttachToComponent(Seat,FAttachmentTransformRules::SnapToTargetNotIncludingScale);}
 else{bRiding=false;DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);SetActorEnableCollision(true);GetCharacterMovement()->SetMovementMode(MOVE_Walking);}
}
