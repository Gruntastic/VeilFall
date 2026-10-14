#if WITH_DEV_AUTOMATION_TESTS
#include "Foundation.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/BoxComponent.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG0InteractionTest,"VeilFallG0.Foundation.AuthoritativeInteraction",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG0InteractionTest::RunTest(const FString&) {
 UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);
 W->InitializeActorsForPlay(FURL()); W->BeginPlay(); W->GetWorldSettings()->NotifyBeginPlay();
 auto* PC=W->SpawnActor<APlayerController>();
 auto* P=W->SpawnActor<AFoundationCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
 P->GetCharacterMovement()->GravityScale=0;
 PC->Possess(P);
 auto* T=W->SpawnActor<AFoundationSwitch>(FVector(200,0,100),FRotator::ZeroRotator);
 auto Aim=[&](){ FVector Eye; FRotator R; P->GetActorEyesViewPoint(Eye,R); PC->SetControlRotation((T->GetActorLocation()-Eye).Rotation()); };
 Aim(); P->Interaction->ServerRequest();
 TestEqual(TEXT("Valid authoritative trace increments"),T->Count,1);
 P->Interaction->ServerRequest();
 TestEqual(TEXT("Rapid repeat rejected"),T->Count,1);
 W->Tick(LEVELTICK_All,0.3f); T->SetActorLocation(FVector(1000,0,100)); Aim(); P->Interaction->ServerRequest();
 TestEqual(TEXT("Out of range rejected"),T->Count,1);
 W->Tick(LEVELTICK_All,0.3f); T->SetActorLocation(FVector(200,0,100)); PC->SetControlRotation(FRotator(0,180,0)); P->Interaction->ServerRequest();
 TestEqual(TEXT("Wrong aim rejected"),T->Count,1);
 W->Tick(LEVELTICK_All,0.3f);
 auto* B=W->SpawnActor<AActor>(); auto* Box=NewObject<UBoxComponent>(B); B->SetRootComponent(Box); B->AddInstanceComponent(Box);
 Box->SetBoxExtent(FVector(20,100,200)); Box->SetCollisionProfileName("BlockAll"); Box->RegisterComponent(); B->SetActorLocation(FVector(100,0,100));
 Aim(); P->Interaction->ServerRequest(); TestEqual(TEXT("Visibility blocker rejects"),T->Count,1);
 B->Destroy(); W->Tick(LEVELTICK_All,0.3f); Aim(); P->Interaction->ServerRequest(); TestEqual(TEXT("Clear trace accepted again"),T->Count,2);
 PC->UnPossess(); W->Tick(LEVELTICK_All,0.3f); P->Interaction->ServerRequest(); TestEqual(TEXT("Unpossessed requester rejected"),T->Count,2);
 TestFalse(TEXT("Null user cannot mutate switch"),T->Activate_Implementation(nullptr)); TestEqual(TEXT("Rejected activation preserves state"),T->Count,2);
 W->EndPlay(EEndPlayReason::Quit); GEngine->DestroyWorldContext(W); W->DestroyWorld(false);
 return true;
}
#endif

#if WITH_DEV_AUTOMATION_TESTS
#include "EnhancedInputComponent.h"
#include "EnhancedPlayerInput.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG0InputTest,"VeilFallG0.Foundation.InputMovement",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG0InputTest::RunTest(const FString&) {
 UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);
 GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);
 W->InitializeActorsForPlay(FURL()); W->BeginPlay(); W->GetWorldSettings()->NotifyBeginPlay();
 auto* Floor=W->SpawnActor<AActor>();
 auto* Box=NewObject<UBoxComponent>(Floor); Floor->SetRootComponent(Box); Floor->AddInstanceComponent(Box);
 Box->SetBoxExtent(FVector(2000,2000,50)); Box->SetCollisionProfileName("BlockAll"); Box->RegisterComponent(); Floor->SetActorLocation(FVector(0,0,-50));
 auto* PC=W->SpawnActor<APlayerController>();
 auto* PlayerInputTest=NewObject<UEnhancedPlayerInput>(PC); PC->PlayerInput=PlayerInputTest; PC->SetAsLocalPlayerController(); PC->SetActorTickEnabled(false); // Test owns input processing; no viewport/local player in this fixture.
 auto* P=W->SpawnActor<AFoundationCharacter>(FVector(0,0,100),FRotator::ZeroRotator);
 PC->Possess(P);
 auto* Input=NewObject<UEnhancedInputComponent>(P); Input->RegisterComponent(); P->SetupPlayerInputComponent(Input);
 const TArray<UInputComponent*> Stack{Input};
 const float DT=1.f/60;
 auto Process=[&](){PlayerInputTest->ProcessInputStack(Stack,DT,false);};
 const FKey Keys[]{EKeys::W,EKeys::S,EKeys::A,EKeys::D};
 const FVector Expected[]{FVector(1,0,0),FVector(-1,0,0),FVector(0,-1,0),FVector(0,1,0)};
 for(int32 i=0;i<4;++i){
  P->ConsumeMovementInputVector();
  for(const auto& M:P->Context->GetMappings()) if(M.Key==Keys[i]){
   PlayerInputTest->InjectInputForAction(M.Action,FInputActionValue(FVector2D(1,0)),M.Modifiers);
  }
  Process();
  TestTrue(*FString::Printf(TEXT("Key %s modifiers and bound movement callback"),*Keys[i].ToString()),P->ConsumeMovementInputVector().Equals(Expected[i],0.001f));
 }
 PlayerInputTest->InjectInputForAction(P->MoveAction,FInputActionValue(FVector2D(1,1))); Process();
 TestTrue(TEXT("Diagonal callback retains both axes"),P->ConsumeMovementInputVector().Equals(FVector(1,1,0),0.001f));
 PC->SetControlRotation(FRotator(0,90,0));
 PlayerInputTest->InjectInputForAction(P->MoveAction,FInputActionValue(FVector2D(0,1))); Process();
 TestTrue(TEXT("Forward movement follows yaw"),P->ConsumeMovementInputVector().Equals(FVector(0,1,0),0.001f));
 PC->SetControlRotation(FRotator::ZeroRotator); PC->RotationInput=FRotator::ZeroRotator;
 PlayerInputTest->InjectInputForAction(P->LookAction,FInputActionValue(FVector2D(1,1))); Process();
 TestTrue(TEXT("Mouse right adds positive yaw"),PC->RotationInput.Yaw>0);
 TestTrue(TEXT("Mouse up adds positive pitch"),PC->RotationInput.Pitch>0);
 TestTrue(TEXT("Explicit mouse sensitivity"),FMath::IsNearlyEqual(PC->RotationInput.Pitch,P->MouseSensitivity));
 PC->RotationInput=FRotator::ZeroRotator;
 for(int32 i=0;i<60;++i) P->GetCharacterMovement()->TickComponent(DT,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Pawn settles on collision floor"),P->GetCharacterMovement()->IsMovingOnGround());
 const FVector Start=P->GetActorLocation();
 for(int32 i=0;i<30;++i){ PlayerInputTest->InjectInputForAction(P->MoveAction,FInputActionValue(FVector2D(0,1))); Process(); P->GetCharacterMovement()->TickComponent(DT,LEVELTICK_All,nullptr); }
 TestTrue(TEXT("Injected forward input moves physical pawn"),P->GetActorLocation().X>Start.X+20);
 PlayerInputTest->InjectInputForAction(P->MoveAction,FInputActionValue(FVector2D::ZeroVector)); Process();
 const float GroundZ=P->GetActorLocation().Z;
 PlayerInputTest->InjectInputForAction(P->JumpAction,FInputActionValue(true)); Process();
 for(int32 i=0;i<10;++i) P->GetCharacterMovement()->TickComponent(DT,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Jump action lifts pawn"),P->GetActorLocation().Z>GroundZ+10);
 PlayerInputTest->InjectInputForAction(P->JumpAction,FInputActionValue(false)); Process();
 TestFalse(TEXT("Jump release clears held state"),P->bPressedJump);
 for(int32 i=0;i<120;++i) P->GetCharacterMovement()->TickComponent(DT,LEVELTICK_All,nullptr);
 TestTrue(TEXT("Pawn lands on floor after jump"),P->GetCharacterMovement()->IsMovingOnGround());
 W->EndPlay(EEndPlayReason::Quit); GEngine->DestroyWorldContext(W); W->DestroyWorld(false);
 return true;
}
#endif

