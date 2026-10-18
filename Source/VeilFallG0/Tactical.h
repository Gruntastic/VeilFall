#pragma once
#include "CoreMinimal.h"
#include "Coop.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Tactical.generated.h"

USTRUCT(BlueprintType)
struct FSprintClock {
 GENERATED_BODY()
 UPROPERTY() bool Active=false;
 UPROPERTY() bool RequireRelease=false;
 UPROPERTY() bool Qualified=false;
 UPROPERTY() int32 Consecutive=0;
 UPROPERTY() double Started=0;
 UPROPERTY() double ReadyAt=0;
 UPROPERTY() double LastQualified=-100;
 UPROPERTY() double BreathingUntil=0;
 void Advance(double Now,bool Held,bool Eligible,bool Moving);
};
enum class EVeilPeek : uint8 { None, Side, Top };
UCLASS()
class UTacticalMovement : public UCharacterMovementComponent {
 GENERATED_BODY()
public:
 bool WantsSprint=false,LeanLeft=false,LeanRight=false,Aiming=false;
 FSprintClock Predicted;
 virtual float GetMaxSpeed()const override;
 virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds)override;
 virtual void UpdateFromCompressedFlags(uint8 Flags)override;
 virtual FNetworkPredictionData_Client* GetPredictionData_Client()const override;
};
UCLASS()
class ATacticalCharacter : public ACoopCharacter {
 GENERATED_BODY()
public:
 ATacticalCharacter(const FObjectInitializer& Initializer=FObjectInitializer::Get());
 virtual void BeginPlay()override;
 virtual void Tick(float DeltaSeconds)override;
 virtual void SetupPlayerInputComponent(UInputComponent* Input)override;
 virtual void GetActorEyesViewPoint(FVector& Location,FRotator& Rotation)const override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out)const override;
 UPROPERTY(ReplicatedUsing=OnRep_Sprint,BlueprintReadOnly) FSprintClock Sprint;
 UPROPERTY(Replicated,BlueprintReadOnly) float Lean=0;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UAudioComponent> BreathAudio;
 UFUNCTION() void OnRep_Sprint();
 bool NearCover()const;
 virtual float PopClearanceHeight()const;
 virtual UClass* DiagnosticCoverClass()const;
 virtual float DiagnosticCoverHeight()const{return 110;}
 EVeilPeek ChoosePeek(float Direction,bool CrouchIntent)const;
 // Crouched at cover with ADS held: stand up over it while aiming, drop back on release.
 // Allowed when the cover top is below AimPopClearanceHeight (foot-relative), i.e. the standing sight line clears it.
 bool CanAimOverCover()const;
 virtual float AimPopClearanceHeight()const;
 void ToggleCrouch();
 UPROPERTY(Replicated,BlueprintReadOnly) bool PoppingUp=false;
 FVector SafeEyeOffset(float Amount)const;
 double ServerTime()const;
 UTacticalMovement* Tactics()const;
private:
 UPROPERTY(Replicated) double ProbeStart=0;
 UPROPERTY() TObjectPtr<UStaticMesh> CarbineMesh;
 UPROPERTY() TObjectPtr<UStaticMesh> CubeMesh;
 bool CapturePlaced=false,CaptureTaken=false;
 UPROPERTY() TObjectPtr<AActor> ProbeObstacle;
 bool ProbeLow=false,ProbeEdge=false,ProbeTopLogged=false,ProbeDropLogged=false,ProbeEdgeLogged=false,ProbeClientTop=false;
 bool ProbePlaced=false,ProbeCover=false,ProbeSprintLogged=false,ProbeCoverLogged=false,ProbeClientLogged=false;
 void DiagnosticTick();
 // Nearest blocking face at crouched cover height (foot-relative), or zero when nothing is in reach.
 FVector LowCoverFace()const;
 bool FaceBlockedAt(const FVector& Face,float Height)const;
 UPROPERTY() TObjectPtr<UInputMappingContext> TacticalContext;
 UPROPERTY() TArray<TObjectPtr<UInputAction>> TacticalActions;
 void SprintStart();void SprintStop();
 void LeftStart();void LeftStop();void RightStart();void RightStop();
};
UCLASS()
class ATacticalProbeCover : public AActor {
 GENERATED_BODY()
public: ATacticalProbeCover();
};
UCLASS()
class ATacticalState : public ACoopState {
 GENERATED_BODY()
public: virtual FString SavePath()const override;
};
UCLASS()
class ATacticalGameMode : public ACoopGameMode {
 GENERATED_BODY()
public: ATacticalGameMode();
};


