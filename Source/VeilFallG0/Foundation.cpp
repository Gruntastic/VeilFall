#include "Foundation.h"
#include "Combat.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Modules/ModuleManager.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
IMPLEMENT_PRIMARY_GAME_MODULE(FDefaultGameModuleImpl, VeilFallG0, "VeilFallG0");

UFoundationInteraction::UFoundationInteraction() { SetIsReplicatedByDefault(true); }
void UFoundationInteraction::Request() { const double Now=GetWorld()->GetTimeSeconds(); if (Now-LastLocalRequest<0.2) return; if (const APawn* P=Cast<APawn>(GetOwner()); P && P->IsLocallyControlled()) { LastLocalRequest=Now; ServerRequest(); } }
void UFoundationInteraction::ServerRequest_Implementation() {
 APawn* P=Cast<APawn>(GetOwner());
 if (!P || !P->HasAuthority() || !P->GetController()) return;
 const double Now=GetWorld()->GetTimeSeconds();
 if (Now-LastRequest<0.2) return;
 LastRequest=Now;
 FVector Eye; FRotator Rotation; P->GetActorEyesViewPoint(Eye,Rotation);
 FHitResult Hit;
 FCollisionQueryParams Query(SCENE_QUERY_STAT(FoundationInteract),false,P);
 if (GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+Rotation.Vector()*300,ECC_Visibility,Query)) {
  AActor* Target=Hit.GetActor();
  if (IsValid(Target) && Target->GetIsReplicated() && Target->Implements<UFoundationInteractable>()) {
   if (IFoundationInteractable::Execute_Activate(Target,P))
    UE_LOG(LogTemp,Log,TEXT("G0_INTERACTION_ACCEPTED user=%s target=%s"),*P->GetName(),*Target->GetName());
  }
 }
}
AFoundationCharacter::AFoundationCharacter(const FObjectInitializer& Initializer):Super(Initializer) {
 bReplicates=true; bUseControllerRotationYaw=true;
 Camera=CreateDefaultSubobject<UCameraComponent>("Camera");
 Camera->SetupAttachment(GetCapsuleComponent()); Camera->SetRelativeLocation(FVector(0,0,64)); Camera->bUsePawnControlRotation=true;
 Interaction=CreateDefaultSubobject<UFoundationInteraction>("Interaction");
}
void AFoundationCharacter::BeginPlay() {
 Super::BeginPlay();
 UE_LOG(LogTemp,Log,TEXT("G0_PAWN_BEGIN authority=%d local=%d"),HasAuthority(),IsLocallyControlled());
}
void AFoundationCharacter::SetupPlayerInputComponent(UInputComponent* Input) {
 Super::SetupPlayerInputComponent(Input);
 auto* Enhanced=CastChecked<UEnhancedInputComponent>(Input);
 Context=NewObject<UInputMappingContext>(this);
 MoveAction=NewObject<UInputAction>(this); MoveAction->ValueType=EInputActionValueType::Axis2D; MoveAction->AccumulationBehavior=EInputActionAccumulationBehavior::Cumulative;
 LookAction=NewObject<UInputAction>(this); LookAction->ValueType=EInputActionValueType::Axis2D;
 JumpAction=NewObject<UInputAction>(this);
 InteractAction=NewObject<UInputAction>(this);
 auto MapMove=[this](FKey Key,bool Negate,bool Swizzle) {
  auto& M=Context->MapKey(MoveAction,Key);
  if (Negate) M.Modifiers.Add(NewObject<UInputModifierNegate>(Context));
  if (Swizzle) M.Modifiers.Add(NewObject<UInputModifierSwizzleAxis>(Context));
 };
 MapMove(EKeys::W,false,true); MapMove(EKeys::S,true,true); MapMove(EKeys::A,true,false); MapMove(EKeys::D,false,false);
 Context->MapKey(LookAction,EKeys::Mouse2D); Context->MapKey(JumpAction,EKeys::SpaceBar); Context->MapKey(InteractAction,EKeys::F);
 Enhanced->BindAction(MoveAction,ETriggerEvent::Triggered,this,&AFoundationCharacter::Move);
 Enhanced->BindAction(LookAction,ETriggerEvent::Triggered,this,&AFoundationCharacter::Look);
 Enhanced->BindAction(JumpAction,ETriggerEvent::Started,this,&ACharacter::Jump);
 Enhanced->BindAction(JumpAction,ETriggerEvent::Completed,this,&ACharacter::StopJumping);
 Enhanced->BindAction(InteractAction,ETriggerEvent::Started,this,&AFoundationCharacter::Interact);
 if (APlayerController* PC=Cast<APlayerController>(GetController())) {
  if (ULocalPlayer* LP=PC->GetLocalPlayer()) {
   auto* Sub=ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LP);
   Sub->AddMappingContext(Context,0);
  }
 }
}
void AFoundationCharacter::Move(const FInputActionValue& Value) {
 FVector2D V=Value.Get<FVector2D>();
 const FRotator Yaw(0,GetControlRotation().Yaw,0);
 AddMovementInput(Yaw.Vector(),V.Y); AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y),V.X);
}
void AFoundationCharacter::Look(const FInputActionValue& Value) { FVector2D V=Value.Get<FVector2D>(); AddControllerYawInput(V.X*MouseSensitivity); AddControllerPitchInput(V.Y*MouseSensitivity); }
void AFoundationCharacter::Interact() { Interaction->Request(); }
AFoundationSwitch::AFoundationSwitch() {
 bReplicates=true; bAlwaysRelevant=true;
 Mesh=CreateDefaultSubobject<UStaticMeshComponent>("Mesh"); SetRootComponent(Mesh);
 static ConstructorHelpers::FObjectFinder<UStaticMesh> Cube(TEXT("/Engine/BasicShapes/Cube.Cube"));
 Mesh->SetStaticMesh(Cube.Object); Mesh->SetCollisionProfileName("BlockAll");
 Label=CreateDefaultSubobject<UTextRenderComponent>("Label"); Label->SetupAttachment(Mesh);
 Label->SetRelativeLocation(FVector(-51,0,80)); Label->SetRelativeRotation(FRotator(0,180,0));
 Label->SetWorldSize(20); Label->SetText(FText::FromString("F: relay test (0)"));
}
bool AFoundationSwitch::Activate_Implementation(APawn* User) {
 if (!HasAuthority() || !IsValid(User) || !User->GetController()) return false;
 ++Count; OnRep_Count(); ForceNetUpdate(); return true;
}
void AFoundationSwitch::OnRep_Count() { Label->SetText(FText::FromString(FString::Printf(TEXT("F: relay test (%d)"),Count))); UE_LOG(LogTemp,Log,TEXT("G0_SWITCH_STATE count=%d authority=%d"),Count,HasAuthority()); }
void AFoundationSwitch::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const { Super::GetLifetimeReplicatedProps(OutLifetimeProps); DOREPLIFETIME(AFoundationSwitch,Count); }
AFoundationGameMode::AFoundationGameMode() { DefaultPawnClass=AFoundationCharacter::StaticClass(); }

void AFoundationCharacter::Tick(float DeltaSeconds) {
 Super::Tick(DeltaSeconds);
 // Explicit command-line diagnostic only. Same owned component RPC as E.
 if (!IsLocallyControlled() || !FParse::Param(FCommandLine::Get(),TEXT("G0Smoke"))) return;
 APlayerController* PC=Cast<APlayerController>(GetController());
 if (!PC) return;
 for (TActorIterator<AFoundationSwitch> It(GetWorld()); It; ++It) {
  FVector Eye; FRotator R; GetActorEyesViewPoint(Eye,R);
  PC->SetControlRotation((It->GetActorLocation()-Eye).Rotation());
  if (GetWorld()->GetTimeSeconds()>3 && GetWorld()->GetTimeSeconds()<8) Interaction->Request();
  break;
 }
}


