#pragma once
// Developer test range (/Game/VeilFall/Dev/L_VF_TestRange): a small flat map for trying mechanics without playing the intro.
// Cover pieces of labelled heights, a training-target lane and an enemy yard. Launch with PlayTestRange.cmd.
#include "CoreMinimal.h"
#include "VFIntro.h"
#include "VFTestRange.generated.h"

class ACombatTarget;
class UTextRenderComponent;

// One per test map. Arms the player, keeps them alive, holds placed enemies at their posts and brings them (and
// training targets) back after they die, and shows a live cover readout. -VFCoverTest runs the cover self-test and exits.
UCLASS()
class AVFTestRange : public AActor { GENERATED_BODY()
public:
 AVFTestRange();
 virtual void BeginPlay() override;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall|Dev") bool bEnemiesShoot=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall|Dev") bool bPlayerInvulnerable=true;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall|Dev",meta=(ClampMin="1")) float RespawnSeconds=6;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall|Dev") bool bShowCoverReadout=true;
private:
 struct FPost { TWeakObjectPtr<AVFIntroNPC> NPC; FTransform Home; double DiedAt=0; };
 TArray<FPost> Posts;
 TMap<TWeakObjectPtr<ACombatTarget>,double> TargetDiedAt;
 FTimerHandle Timer;
 void Step();
 void Post(AVFIntroNPC* NPC);
 // -VFCoverTest: scripted approach/crouch/ADS checks against the tagged cover pieces.
 void CoverTestStep();
 void ArmoryTestStep();
 bool Check(bool bOk,const TCHAR* What);
 void PlaceAt(FName CoverTag);
 FTimerHandle TestTimer; int32 TestPhase=0; double PhaseAt=0; bool bTestFailed=false; int32 Checks=0;
};

// Test range table: shows a weapon model; [F] swaps it into the primary slot and equips it (sound/feel testing).
UCLASS()
class AVFWeaponRack : public AActor, public IFoundationInteractable, public IVFPromptSource { GENERATED_BODY()
public:
 AVFWeaponRack();
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual bool Activate_Implementation(APawn* User) override;
 virtual FText GetPrompt(APawn* User) const override;
 // weapon.carbine, weapon.shotgun, weapon.mg, weapon.marksman (sniper rifle), weapon.pistol.
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") FName WeaponId="weapon.carbine";
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") TSoftObjectPtr<UStaticMesh> Model;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Table;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Display;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
};

// Test range crate: [F] throws a grenade along the view (throw, bounce and explosion sounds from DA_VF_WeaponSounds).
UCLASS()
class AVFGrenadeCrate : public AActor, public IFoundationInteractable, public IVFPromptSource { GENERATED_BODY()
public:
 AVFGrenadeCrate();
 virtual void BeginPlay() override;
 virtual bool Activate_Implementation(APawn* User) override;
 virtual FText GetPrompt(APawn* User) const override {return FText::FromString(TEXT("Throw a grenade"));}
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") float ThrowSpeed=1500;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Crate;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Sample;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Label;
private: double LastThrow=-10;
};

UCLASS()
class AVFTestRangeGameMode : public AGameModeBase { GENERATED_BODY()
public: AVFTestRangeGameMode();
};
