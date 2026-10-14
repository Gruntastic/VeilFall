#if WITH_DEV_AUTOMATION_TESTS
#include "MissionWorkshop.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WorldSettings.h"
#include "Components/BoxComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG5MovementTest,"VeilFallG0.Workshop.WaistCoverAndJumpFlight",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG5MovementTest::RunTest(const FString&){
 UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());W->BeginPlay();W->GetWorldSettings()->NotifyBeginPlay();
 auto Box=[W](FVector At,FVector Extent){auto* A=W->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(A);A->SetRootComponent(B);A->AddInstanceComponent(B);B->SetBoxExtent(Extent);B->SetCollisionProfileName("BlockAll");B->RegisterComponent();A->SetActorLocation(At);return B;};
 Box(FVector(0,0,-50),FVector(2000,2000,50));auto* P=W->SpawnActor<AG5Character>(FVector(0,0,100),FRotator::ZeroRotator);auto* PC=W->SpawnActor<APlayerController>();PC->SetAsLocalPlayerController();PC->SetActorTickEnabled(false);PC->Possess(P);auto* M=P->Tactics();const float DT=1.f/120;
 for(int i=0;i<60;++i)M->TickComponent(DT,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Grounded before jump"),M->IsMovingOnGround());const float Ground=P->GetActorLocation().Z;P->Jump();M->TickComponent(DT,LEVELTICK_All,nullptr);P->StopJumping();TestTrue(TEXT("Jump is physical falling state"),M->IsFalling());float Peak=P->GetActorLocation().Z;int Frames=1;
 while(M->IsFalling()&&Frames<240){M->TickComponent(DT,LEVELTICK_All,nullptr);Peak=FMath::Max(Peak,P->GetActorLocation().Z);++Frames;}
 const float AirTime=Frames*DT;const float Rise=Peak-Ground;TestTrue(TEXT("Heavier jump lands in less than 0.75 seconds"),AirTime>.45f&&AirTime<.75f);TestTrue(TEXT("Jump retains useful height without float"),Rise>60&&Rise<90);TestTrue(TEXT("Jump lands on real floor"),M->IsMovingOnGround());
 P->Health->SetArmor(EVeilArmor::Heavy);TestEqual(TEXT("Health presentation refresh retains jump tuning"),M->JumpZVelocity,520.f);TestEqual(TEXT("Gravity survives equipment update"),M->GravityScale,1.75f);
 auto* Wall=Box(FVector(70,0,45),FVector(5,80,45));P->ToggleCrouch();M->UpdateCharacterStateBeforeMovement(DT);TestTrue(TEXT("90 cm waist cover chooses top"),P->ChoosePeek(1,true)==EVeilPeek::Top);M->LeanRight=true;M->UpdateCharacterStateBeforeMovement(DT);TestTrue(TEXT("Waist peek exposes standing player"),P->PoppingUp&&!P->bIsCrouched);M->LeanRight=false;M->UpdateCharacterStateBeforeMovement(DT);TestTrue(TEXT("Release restores crouch"),P->bIsCrouched);
 Wall->SetBoxExtent(FVector(5,80,55));Wall->GetOwner()->SetActorLocation(FVector(70,0,55));TestTrue(TEXT("110 cm cover uses side rather than top"),P->ChoosePeek(1,true)==EVeilPeek::Side);
 UE_LOG(LogTemp,Log,TEXT("G5_JUMP_FLIGHT_MEASURED airtime=%.3f riseCm=%.2f gravity=%.2f"),AirTime,Rise,M->GravityScale);
 W->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(W);W->DestroyWorld(false);return true;
}
#endif

