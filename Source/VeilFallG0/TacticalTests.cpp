#if WITH_DEV_AUTOMATION_TESTS
#include "Tactical.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "Components/BoxComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG4SprintTest,"VeilFallG0.Tactical.SprintFatigue",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG4SprintTest::RunTest(const FString&){
 FSprintClock C;for(int i=0;i<3;i++){const double T=i*2.;C.Advance(T,true,true,false);C.Advance(T+.6,true,true,true);C.Advance(T+.8,false,true,true);}
 TestEqual(TEXT("Three distinct moving bursts qualify"),C.Consecutive,3);TestEqual(TEXT("Third burst starts eight seconds of breathing"),C.BreathingUntil,12.6);C.Advance(13,false,true,false);TestEqual(TEXT("Rest resets the streak"),C.Consecutive,0);
 C=FSprintClock();C.Advance(0,true,true,false);C.Advance(.6,true,true,true);C.Advance(4,true,true,true);TestFalse(TEXT("Four second maximum enforced"),C.Active);C.Advance(6,true,true,true);TestFalse(TEXT("Held key cannot auto restart"),C.Active);C.Advance(6.1,false,true,false);C.Advance(6.2,true,true,true);TestTrue(TEXT("Release permits another burst after cooldown"),C.Active);C.Advance(6.3,true,false,true);TestFalse(TEXT("Ineligible movement cancels sprint"),C.Active);
 C=FSprintClock();C.Advance(0,true,true,false);C.Advance(.8,true,true,false);C.Advance(.9,false,true,false);TestEqual(TEXT("Stationary tapping does not add fatigue"),C.Consecutive,0);C.Advance(1.2,true,true,true);TestFalse(TEXT("Cooldown rejects early restart"),C.Active);return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG4CoverTest,"VeilFallG0.Tactical.CoverEyeCollision",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG4CoverTest::RunTest(const FString&){
 UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());W->BeginPlay();W->GetWorldSettings()->NotifyBeginPlay();auto* P=W->SpawnActor<ATacticalCharacter>();
 TestFalse(TEXT("Open ground is not cover"),P->NearCover());TestTrue(TEXT("Open ground cannot lean"),P->SafeEyeOffset(1).IsNearlyZero());
 auto* A=W->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(A);A->SetRootComponent(B);B->RegisterComponent();B->SetBoxExtent(FVector(5,80,100));B->SetCollisionProfileName("BlockAll");A->SetActorLocation(FVector(70,0,40));
 TestTrue(TEXT("Nearby blocking wall is cover"),P->NearCover());TestTrue(TEXT("Unblocked eye lean is bounded to 28cm"),FMath::IsNearlyEqual(P->SafeEyeOffset(1).Size(),28.f));
 auto* A2=W->SpawnActor<AActor>();auto* B2=NewObject<UBoxComponent>(A2);A2->SetRootComponent(B2);B2->RegisterComponent();B2->SetBoxExtent(FVector(40,5,100));B2->SetCollisionProfileName("BlockAll");A2->SetActorLocation(FVector(0,20,40));
 TestTrue(TEXT("Eye sweep shortens lean against side obstruction"),P->SafeEyeOffset(1).Size()<10);TestTrue(TEXT("Excess lean input remains clamped"),P->SafeEyeOffset(100).Size()<=28);
 W->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(W);W->DestroyWorld(false);return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG4PopTest,"VeilFallG0.Tactical.ToggleAndLowCoverPop",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG4PopTest::RunTest(const FString&){
 UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());W->BeginPlay();W->GetWorldSettings()->NotifyBeginPlay();
 auto Box=[W](FVector At,FVector Extent){auto* A=W->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(A);A->SetRootComponent(B);B->RegisterComponent();B->SetBoxExtent(Extent);B->SetCollisionProfileName("BlockAll");A->SetActorLocation(At);return A;};
 Box(FVector(0,0,-10),FVector(1000,1000,10));auto* P=W->SpawnActor<ATacticalCharacter>(FVector(0,0,88),FRotator::ZeroRotator);auto* M=P->Tactics();M->SetMovementMode(MOVE_Walking);auto* Wall=Box(FVector(70,0,55),FVector(5,80,55));
 P->ToggleCrouch();M->UpdateCharacterStateBeforeMovement(.016);TestTrue(TEXT("Toggle enters crouch without held key"),P->bIsCrouched);TestTrue(TEXT("Crouch intent remains latched"),bool(M->bWantsToCrouch));
 TestTrue(TEXT("Center of low wall chooses top"),P->ChoosePeek(1,true)==EVeilPeek::Top);M->LeanRight=true;M->UpdateCharacterStateBeforeMovement(.016);TestFalse(TEXT("Hold raises actual capsule"),P->bIsCrouched);TestTrue(TEXT("Pop preserves crouch intent"),bool(M->bWantsToCrouch));TestTrue(TEXT("Pop exposes player"),P->PoppingUp);
 CastChecked<UBoxComponent>(Wall->GetRootComponent())->SetBoxExtent(FVector(5,80,40));Wall->SetActorLocation(FVector(70,0,40));M->UpdateCharacterStateBeforeMovement(.016);M->UpdateCharacterStateBeforeMovement(.016);TestFalse(TEXT("Holding pop at 80cm cover remains standing without oscillation"),P->bIsCrouched);CastChecked<UBoxComponent>(Wall->GetRootComponent())->SetBoxExtent(FVector(5,80,55));Wall->SetActorLocation(FVector(70,0,55));
 M->LeanRight=false;M->UpdateCharacterStateBeforeMovement(.016);TestTrue(TEXT("Release returns crouch"),P->bIsCrouched);TestFalse(TEXT("Release clears pop"),P->PoppingUp);
 Wall->SetActorLocation(FVector(70,-75,55));TestTrue(TEXT("Open right edge chooses side"),P->ChoosePeek(1,true)==EVeilPeek::Side);TestTrue(TEXT("Closed left edge chooses top"),P->ChoosePeek(-1,true)==EVeilPeek::Top);M->LeanRight=true;M->UpdateCharacterStateBeforeMovement(.016);TestTrue(TEXT("Side stays crouched"),P->bIsCrouched);
 Wall->SetActorLocation(FVector(70,0,55));auto* Ceiling=Box(FVector(0,0,120),FVector(40,40,5));M->UpdateCharacterStateBeforeMovement(.016);TestTrue(TEXT("Ceiling prevents standing"),P->bIsCrouched);TestFalse(TEXT("Blocked pop not exposed"),P->PoppingUp);Ceiling->Destroy();M->LeanRight=false;
 P->ToggleCrouch();M->UpdateCharacterStateBeforeMovement(.016);TestFalse(TEXT("Second toggle stands"),P->bIsCrouched);TestFalse(TEXT("Second toggle unlatches"),bool(M->bWantsToCrouch));
 W->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(W);W->DestroyWorld(false);return true;
}
#endif

