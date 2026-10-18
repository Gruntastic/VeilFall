#include "VFTestRange.h"
#include "Combat.h"
#include "VFIntroShared.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

AVFTestRange::AVFTestRange(){PrimaryActorTick.bCanEverTick=false;}
void AVFTestRange::BeginPlay(){
 Super::BeginPlay();
 const bool bCoverTest=FParse::Param(FCommandLine::Get(),TEXT("VFCoverTest"));
 const bool bArmoryTest=FParse::Param(FCommandLine::Get(),TEXT("VFArmoryTest"));
 if(bCoverTest||bArmoryTest)bEnemiesShoot=false;
 for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It){FPost P;P.NPC=*It;P.Home=It->GetActorTransform();Posts.Add(P);}
 GetWorldTimerManager().SetTimer(Timer,this,&AVFTestRange::Step,.2f,true,.5f);
 if(bCoverTest)GetWorldTimerManager().SetTimer(TestTimer,this,&AVFTestRange::CoverTestStep,.1f,true,2.f);
 if(bArmoryTest)GetWorldTimerManager().SetTimer(TestTimer,this,&AVFTestRange::ArmoryTestStep,1.5f,true,3.f);
}
// -VFArmoryTest: takes each weapon from its rack, fires it, throws a grenade, and saves screenshots to Saved/TestRange/.
void AVFTestRange::ArmoryTestStep(){
 auto* P=Cast<AVFIntroCharacter>(UGameplayStatics::GetPlayerPawn(this,0));if(!P||!P->bArmed)return;
 TArray<AVFWeaponRack*> Racks;for(TActorIterator<AVFWeaponRack> It(GetWorld());It;++It)Racks.Add(*It);Racks.Sort([](const AVFWeaponRack& A,const AVFWeaponRack& B){return A.GetActorLocation().Y<B.GetActorLocation().Y;});
 // Screenshot a moment later so the held model has updated and the teleport blur has settled.
 auto Shot=[this](const FString& Name){const FString Path=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("TestRange"),Name+TEXT(".png"));FTimerHandle H;GetWorldTimerManager().SetTimer(H,[Path]{FScreenshotRequest::RequestScreenshot(Path,true,false);},.8f,false);};
 auto Face=[P](FVector At,FRotator R){P->SetActorLocation(At+FVector(0,0,P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2),false,nullptr,ETeleportType::TeleportPhysics);P->SetActorRotation(FRotator(0,R.Yaw,0));P->GetController()->SetControlRotation(R);};
 const int32 Phase=TestPhase++;
 if(Phase==0){if(auto* PC=Cast<APlayerController>(P->GetController()))PC->ConsoleCommand(TEXT("r.MotionBlurQuality 0"));Face(FVector(-2800,-1600,0),FRotator(-12,25,0));Shot(TEXT("Armory"));return;}
 if(Phase>=1&&Phase<=Racks.Num()){
  // Odd half-steps: take + fire; the screenshot of the held weapon lands on the next timer tick.
  AVFWeaponRack* R=Racks[Phase-1];IFoundationInteractable::Execute_Activate(R,P);
  Check(P->Combat->EquippedId==R->WeaponId,*FString::Printf(TEXT("rack equips %s"),*R->WeaponId.ToString()));
  Face(FVector(-2000,-1400,0),FRotator(0,0,0));const int32 Before=P->Combat->CurrentAmmo()?P->Combat->CurrentAmmo()->Magazine:-1;
  P->Combat->StartFire();P->Combat->StopFire();const int32 After=P->Combat->CurrentAmmo()?P->Combat->CurrentAmmo()->Magazine:-1;
  const auto* D=UVFWeaponSounds::Get();const FVFWeaponSoundSet* W=D?D->Weapons.Find(R->WeaponId):nullptr;
  Check(After==Before-1,*FString::Printf(TEXT("%s fires (mag %d -> %d, fire sound %s)"),*R->WeaponId.ToString(),Before,After,W?*UVFWeaponSounds::Label(W->Fire):TEXT("(no entry)")));
  Shot(R->WeaponId.ToString().Replace(TEXT("weapon."),TEXT("Held_")));return;
 }
 if(Phase==Racks.Num()+1){for(TActorIterator<AVFGrenadeCrate> It(GetWorld());It;++It){Face(It->GetActorLocation()-FVector(150,0,It->GetActorLocation().Z),FRotator(-5,0,0));Check(IFoundationInteractable::Execute_Activate(*It,P),TEXT("grenade crate throws"));break;}return;}
 if(Phase==Racks.Num()+2){bool bFlying=false;for(TActorIterator<AVFGrenade> It(GetWorld());It;++It)bFlying=true;Check(bFlying,TEXT("grenade in flight"));Shot(TEXT("Grenade_Flight"));return;}
 if(Phase==Racks.Num()+3)return; // fuse is 3.2 s
 if(Phase==Racks.Num()+4){bool bBlast=false;for(TActorIterator<AVFBlastEffect> It(GetWorld());It;++It)bBlast=true;Check(bBlast,TEXT("grenade exploded"));Shot(TEXT("Grenade_Blast"));return;}
 UE_LOG(LogTemp,Display,TEXT("%s checks=%d"),bTestFailed?TEXT("VF_ARMORY_TEST_FAIL"):TEXT("VF_ARMORY_TEST_PASS"),Checks);
 GetWorldTimerManager().ClearTimer(TestTimer);FPlatformMisc::RequestExit(false);
}
void AVFTestRange::Post(AVFIntroNPC* NPC){
 auto* AI=NPC?Cast<AVFIntroAIController>(NPC->GetController()):nullptr;if(!AI)return;
 const FPost* P=Posts.FindByPredicate([NPC](const FPost& E){return E.NPC.Get()==NPC;});if(!P)return;
 if(bEnemiesShoot)AI->HoldAt(P->Home.GetLocation(),P->Home.Rotator());else AI->Idle(P->Home.Rotator());
}
void AVFTestRange::Step(){
 const double Now=GetWorld()->GetTimeSeconds();
 auto* Player=Cast<AVFIntroCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
 if(Player){
  if(!Player->bArmed)Player->SetArmed(true);
  if(bPlayerInvulnerable){
   if(Player->Health->State.Life!=EVeilLife::Alive){Player->Health->ResetHealth();Player->RefreshPresentation();}
   else if(Player->Health->State.Health<100)Player->Health->Recover(100);
  }
 }
 // Enemies: hold the post they were placed at; come back RespawnSeconds after dying (the AI stops thinking on death, so re-possess).
 for(FPost& P:Posts){
  AVFIntroNPC* N=P.NPC.Get();if(!N)continue;auto* AI=Cast<AVFIntroAIController>(N->GetController());
  if(!N->IsAlive()){if(P.DiedAt<=0)P.DiedAt=Now;
   if(Now-P.DiedAt>=RespawnSeconds){P.DiedAt=0;N->Health->ResetHealth();N->SetActorTransform(P.Home,false,nullptr,ETeleportType::TeleportPhysics);N->RefreshPresentation();if(AI){AI->UnPossess();AI->Possess(N);}Post(N);}
   continue;}
  if(AI&&((bEnemiesShoot&&AI->Mode!=EVFAIMode::Hold)||(!bEnemiesShoot&&AI->Mode!=EVFAIMode::Idle)))Post(N);
 }
 for(TActorIterator<ACombatTarget> It(GetWorld());It;++It){
  if(It->Health->State.Life!=EVeilLife::Dead){TargetDiedAt.Remove(*It);continue;}
  double& At=TargetDiedAt.FindOrAdd(*It,Now);if(Now-At>=RespawnSeconds*.5f){It->Health->ResetHealth();TargetDiedAt.Remove(*It);}
 }
 if(bShowCoverReadout&&Player)if(auto* PC=Cast<AVFIntroPlayerController>(Player->GetController()))if(PC->HUD){
  const bool Near=Player->NearCover(),Crouched=Player->bIsCrouched,Pop=Player->PoppingUp,ADS=Player->Combat->IsADS();
  const FString Stance=Pop?TEXT("POPPED UP"):Crouched?TEXT("crouched"):TEXT("standing");
  const FString Over=Near&&Player->Tactics()->bWantsToCrouch?(Player->CanAimOverCover()?TEXT("  |  hold RMB to pop up"):TEXT("  |  too tall to pop over: Q/E to lean")):FString();
  // Sound test bed: which sounds the held weapon and the grenade crate are using right now.
  FString SoundLine;const FName Id=Player->Combat->EquippedId;
  if(const auto* D=UVFWeaponSounds::Get()){const FVFWeaponSoundSet* W=D->Weapons.Find(Id);const FVFWeaponSoundSet None;if(!W)W=&None;
   SoundLine=FString::Printf(TEXT("\nSounds  %s: fire %s   reload %s   equip %s   |   grenade: throw %s   bounce %s   blast %s"),*Player->Combat->Spec(Id).DisplayName,
    *UVFWeaponSounds::Label(W->Fire),*UVFWeaponSounds::Label(W->Reload),*UVFWeaponSounds::Label(W->Equip),*UVFWeaponSounds::Label(D->GrenadeThrow),*UVFWeaponSounds::Label(D->GrenadeBounce),*UVFWeaponSounds::Label(D->GrenadeExplosion));}
  else SoundLine=TEXT("\nSounds: DA_VF_WeaponSounds missing (Scripts/VFTestRange.ps1 Weapons)");
  PC->HUD->ShowHint(FString::Printf(TEXT("TEST RANGE   Ctrl/C crouch   RMB aim (pops up at cover)   Q/E peek   Shift sprint   R reload   1/2/3 weapons   F take weapon / throw grenade\nCover: %s   %s%s%s%s"),
   Near?TEXT("at cover"):TEXT("open ground"),*Stance,ADS?TEXT("   ADS"):TEXT(""),*Over,*SoundLine),1.f);
 }
}

// ---------------------------------------------------------------- -VFCoverTest
bool AVFTestRange::Check(bool bOk,const TCHAR* What){
 ++Checks;if(!bOk)bTestFailed=true;
 auto* P=Cast<ATacticalCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
 UE_LOG(LogTemp,Display,TEXT("VF_COVER_CHECK %s: %s (crouched=%d intent=%d pop=%d ads=%d near=%d)"),bOk?TEXT("PASS"):TEXT("FAIL"),What,
  P&&P->bIsCrouched,P&&P->Tactics()->bWantsToCrouch,P&&P->PoppingUp,P&&P->Combat->IsADS(),P&&P->NearCover());
 return bOk;
}
void AVFTestRange::PlaceAt(FName CoverTag){
 auto* P=Cast<ACharacter>(UGameplayStatics::GetPlayerPawn(this,0));if(!P)return;
 FVector Spot=FVector::ZeroVector;
 if(CoverTag.IsNone()){for(TActorIterator<APlayerStart> It(GetWorld());It;++It){Spot=It->GetActorLocation();break;}Spot.Z=0;}
 else for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(CoverTag)){FVector Origin,Extent;It->GetActorBounds(true,Origin,Extent);Spot=FVector(Origin.X-Extent.X-P->GetCapsuleComponent()->GetScaledCapsuleRadius()-16,Origin.Y,0);break;}
 // Cover faces -X: stand on its near side looking at it.
 P->SetActorLocation(Spot+FVector(0,0,P->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+2),false,nullptr,ETeleportType::TeleportPhysics);
 P->SetActorRotation(FRotator::ZeroRotator);if(P->GetController())P->GetController()->SetControlRotation(FRotator::ZeroRotator);
}
void AVFTestRange::CoverTestStep(){
 auto* P=Cast<ATacticalCharacter>(UGameplayStatics::GetPlayerPawn(this,0));if(!P)return;
 const double Now=GetWorld()->GetTimeSeconds();if(Now-PhaseAt<.6)return;PhaseAt=Now;
 auto* M=P->Tactics();auto* C=P->Combat.Get();
 switch(TestPhase++){
 case 0:C->SetADS(false);P->UnCrouch();PlaceAt("VF_CoverTest_110");break;
 case 1:Check(P->NearCover()&&!P->bIsCrouched,TEXT("standing at 110 cm cover"));P->ToggleCrouch();break;
 case 2:Check(P->bIsCrouched&&!P->PoppingUp,TEXT("crouch takes cover behind 110 cm"));C->SetADS(true);break;
 case 3:Check(P->PoppingUp&&!P->bIsCrouched&&M->bWantsToCrouch,TEXT("hold ADS pops up over 110 cm"));C->SetADS(false);break;
 case 4:Check(P->bIsCrouched&&!P->PoppingUp,TEXT("release ADS drops back behind cover"));C->SetADS(true);break;
 case 5:Check(P->PoppingUp&&!P->bIsCrouched,TEXT("second ADS pops up again"));C->SetADS(false);PlaceAt("VF_CoverTest_120");break;
 case 6:C->SetADS(true);break;
 case 7:Check(P->PoppingUp&&!P->bIsCrouched,TEXT("hold ADS pops up over 120 cm (hay bale height)"));C->SetADS(false);PlaceAt("VF_CoverTest_220");break;
 case 8:C->SetADS(true);break;
 case 9:Check(P->bIsCrouched&&!P->PoppingUp,TEXT("ADS at 220 cm wall stays crouched"));C->SetADS(false);PlaceAt("VF_CoverTest_90");break;
 case 10:M->LeanRight=true;break;
 case 11:Check(P->PoppingUp&&!P->bIsCrouched,TEXT("E peek still pops over 90 cm"));M->LeanRight=false;PlaceAt(NAME_None);break;
 case 12:C->SetADS(true);break;
 case 13:Check(P->bIsCrouched&&!P->PoppingUp,TEXT("ADS in the open stays crouched"));C->SetADS(false);P->ToggleCrouch();break;
 case 14:Check(!P->bIsCrouched,TEXT("toggle crouch stands back up"));
  UE_LOG(LogTemp,Display,TEXT("%s checks=%d"),bTestFailed?TEXT("VF_COVER_TEST_FAIL"):TEXT("VF_COVER_TEST_PASS"),Checks);
  GetWorldTimerManager().ClearTimer(TestTimer);FPlatformMisc::RequestExit(false);break;
 default:break;
 }
}

// ---------------------------------------------------------------- weapon rack / grenade crate
AVFWeaponRack::AVFWeaponRack(){
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 Table=CreateDefaultSubobject<UStaticMeshComponent>("Table");SetRootComponent(Table);Table->SetStaticMesh(Cube.Object);Table->SetRelativeScale3D(FVector(.7f,1.5f,.9f));Table->SetCollisionProfileName("BlockAll");
 Display=CreateDefaultSubobject<UStaticMeshComponent>("Display");Display->SetupAttachment(Table);Display->SetUsingAbsoluteScale(true);Display->SetRelativeLocation(FVector(0,0,55));Display->SetRelativeRotation(FRotator(0,90,0));Display->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Label=CreateDefaultSubobject<UTextRenderComponent>("Label");Label->SetupAttachment(Table);Label->SetUsingAbsoluteScale(true);Label->SetRelativeLocation(FVector(-40,0,90));Label->SetRelativeRotation(FRotator(0,180,0));Label->SetHorizontalAlignment(EHTA_Center);Label->SetWorldSize(18);Label->SetTextRenderColor(FColor(255,230,150));
}
void AVFWeaponRack::OnConstruction(const FTransform& T){
 Super::OnConstruction(T);VF::Paint(Table,FLinearColor(.3f,.2f,.12f));
 if(UStaticMesh* M=Model.LoadSynchronous())Display->SetStaticMesh(M);
 Label->SetText(FText::FromString(UCombatDataLibrary::DefaultWeaponSpec(WeaponId).DisplayName));
}
FText AVFWeaponRack::GetPrompt(APawn* User)const{return FText::FromString(TEXT("Take the ")+UCombatDataLibrary::DefaultWeaponSpec(WeaponId).DisplayName.ToLower());}
bool AVFWeaponRack::Activate_Implementation(APawn* User){
 auto* P=Cast<ACombatCharacter>(User);if(!HasAuthority()||!P||P->Health->State.Life!=EVeilLife::Alive)return false;auto* C=P->Combat.Get();
 if(!C->Spec(WeaponId).Validate()||C->State.Carried.Num()!=3)return false;
 // Pistol lives in slot 3; everything else replaces the primary.
 const int32 Slot=WeaponId=="weapon.pistol"?2:0;C->CancelActions();C->State.Carried[Slot]=WeaponId;
 if(!C->State.Ammo.ContainsByPredicate([this](const FVeilAmmo& A){return A.WeaponId==WeaponId;})){FVeilAmmo A;A.WeaponId=WeaponId;C->State.Ammo.Add(A);}
 C->Restock();C->ServerEquip_Implementation(Slot);return true;
}
AVFGrenadeCrate::AVFGrenadeCrate(){
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 Crate=CreateDefaultSubobject<UStaticMeshComponent>("Crate");SetRootComponent(Crate);Crate->SetStaticMesh(Cube.Object);Crate->SetRelativeScale3D(FVector(.6f,.9f,.6f));Crate->SetCollisionProfileName("BlockAll");
 Sample=CreateDefaultSubobject<UStaticMeshComponent>("Sample");Sample->SetupAttachment(Crate);Sample->SetUsingAbsoluteScale(true);Sample->SetRelativeLocation(FVector(0,0,55));Sample->SetCollisionEnabled(ECollisionEnabled::NoCollision);
 Label=CreateDefaultSubobject<UTextRenderComponent>("Label");Label->SetupAttachment(Crate);Label->SetUsingAbsoluteScale(true);Label->SetRelativeLocation(FVector(-35,0,75));Label->SetRelativeRotation(FRotator(0,180,0));Label->SetHorizontalAlignment(EHTA_Center);Label->SetWorldSize(18);Label->SetTextRenderColor(FColor(255,230,150));Label->SetText(FText::FromString(TEXT("Grenades")));
}
void AVFGrenadeCrate::BeginPlay(){
 Super::BeginPlay();VF::Paint(Crate,FLinearColor(.11f,.13f,.07f));
 if(auto* M=LoadObject<UStaticMesh>(nullptr,TEXT("/Game/VeilFall/Weapons/TEMP_Grenade.TEMP_Grenade"),nullptr,LOAD_NoWarn|LOAD_Quiet))Sample->SetStaticMesh(M);
}
bool AVFGrenadeCrate::Activate_Implementation(APawn* User){
 const double Now=GetWorld()->GetTimeSeconds();if(!User||Now-LastThrow<.8)return false;LastThrow=Now;
 FVector Eye;FRotator R;User->GetActorEyesViewPoint(Eye,R);const FVector Dir=(R.Vector()+FVector(0,0,.25f)).GetSafeNormal();
 auto* G=GetWorld()->SpawnActor<AVFGrenade>(AVFGrenade::StaticClass(),Eye+R.Vector()*60,FRotator::ZeroRotator);if(!G)return false;G->Launch(Dir*ThrowSpeed,User);
 if(const auto* D=UVFWeaponSounds::Get())if(USoundBase* S=D->GrenadeThrow.LoadSynchronous())UGameplayStatics::PlaySound2D(this,S,.7f);
 return true;
}

AVFTestRangeGameMode::AVFTestRangeGameMode(){DefaultPawnClass=AVFIntroCharacter::StaticClass();PlayerControllerClass=AVFIntroPlayerController::StaticClass();}
