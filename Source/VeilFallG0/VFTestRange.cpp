#include "VFTestRange.h"
#include "Combat.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/PlatformMisc.h"

AVFTestRange::AVFTestRange(){PrimaryActorTick.bCanEverTick=false;}
void AVFTestRange::BeginPlay(){
 Super::BeginPlay();
 const bool bCoverTest=FParse::Param(FCommandLine::Get(),TEXT("VFCoverTest"));
 if(bCoverTest)bEnemiesShoot=false;
 for(TActorIterator<AVFIntroNPC> It(GetWorld());It;++It){FPost P;P.NPC=*It;P.Home=It->GetActorTransform();Posts.Add(P);}
 GetWorldTimerManager().SetTimer(Timer,this,&AVFTestRange::Step,.2f,true,.5f);
 if(bCoverTest)GetWorldTimerManager().SetTimer(TestTimer,this,&AVFTestRange::CoverTestStep,.1f,true,2.f);
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
  PC->HUD->ShowHint(FString::Printf(TEXT("TEST RANGE   Ctrl/C crouch   RMB aim (pops up at cover)   Q/E peek   Shift sprint   R reload   1/2/3 weapons\nCover: %s   %s%s%s"),
   Near?TEXT("at cover"):TEXT("open ground"),*Stance,ADS?TEXT("   ADS"):TEXT(""),*Over),1.f);
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

AVFTestRangeGameMode::AVFTestRangeGameMode(){DefaultPawnClass=AVFIntroCharacter::StaticClass();PlayerControllerClass=AVFIntroPlayerController::StaticClass();}
