#include "MissionWorkshop.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Paths.h"
AG5Character::AG5Character(){GetCharacterMovement()->GravityScale=JumpGravityScale;GetCharacterMovement()->JumpZVelocity=JumpSpeed;JumpMaxHoldTime=0;}
void AG5Character::RefreshPresentation(){Super::RefreshPresentation();GetCharacterMovement()->GravityScale=JumpGravityScale;GetCharacterMovement()->JumpZVelocity=Health->State.Life==EVeilLife::Alive?JumpSpeed:0;}
float AG5Character::PopClearanceHeight()const{return WaistCoverHeight+10;}
UClass* AG5Character::DiagnosticCoverClass()const{return AG5ProbeCover::StaticClass();}
AG5ProbeCover::AG5ProbeCover(){CastChecked<UBoxComponent>(GetRootComponent())->SetBoxExtent(FVector(5,80,45));}
AG5GameMode::AG5GameMode(){DefaultPawnClass=AG5Character::StaticClass();GameStateClass=AG5State::StaticClass();}
FString AG5State::SavePath()const{
 if(FParse::Param(FCommandLine::Get(),TEXT("G3Smoke"))||FParse::Param(FCommandLine::Get(),TEXT("G4Smoke"))||FParse::Param(FCommandLine::Get(),TEXT("G4Capture")))return ADeadSignalState::SavePath();
 if(FParse::Param(FCommandLine::Get(),TEXT("G5Preview"))){if(PreviewSavePath.IsEmpty())PreviewSavePath=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("G5WorkshopPreview"),FGuid::NewGuid().ToString(),TEXT("DeadSignal.vfs"));return PreviewSavePath;}
 return FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("CampaignG5/DeadSignal.vfs"));
}
void AG5Character::BeginPlay(){Super::BeginPlay();RefreshPresentation();}
