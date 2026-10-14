#pragma once
#include "CoreMinimal.h"
#include "Foundation.h"
#include "Campaign.h"
#include "Engine/NetSerialization.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Combat.generated.h"

UENUM(BlueprintType)
enum class EVeilLife : uint8 { Alive, Downed, Dead };
UENUM(BlueprintType)
enum class EVeilArmor : uint8 { Light, Medium, Heavy };
UENUM(BlueprintType)
enum class EVeilRegion : uint8 { Torso, Head, Limb };
UENUM(BlueprintType)
enum class EVeilUse : uint8 { None, Medical, Revive, Ammunition };
UENUM(BlueprintType)
enum class EVeilTeam : uint8 { FreeStates, Continuity, Neutral };

USTRUCT(BlueprintType)
struct FVeilWeaponSpec {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName StableId="weapon.carbine";
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FString DisplayName="Carbine";
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 MagazineSize=30;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 ReserveLimit=90;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Damage=50;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Penetration=10;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float ShotInterval=0.12f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float ReloadSeconds=2;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float RangeCm=10000;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) int32 Pellets=1;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float SpreadDegrees=0;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float RecoilDegrees=0.7f;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bAutomatic=true;
 bool Validate() const;
};
UCLASS(BlueprintType)
class UVeilWeaponDefinition : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FVeilWeaponSpec Spec;
 virtual FPrimaryAssetId GetPrimaryAssetId() const override {return FPrimaryAssetId("VeilFallWeapon",Spec.StableId);}
};
UCLASS()
class UCombatDataLibrary : public UBlueprintFunctionLibrary {
 GENERATED_BODY()
public:
 UFUNCTION(BlueprintPure) static FVeilWeaponSpec DefaultWeaponSpec(FName StableId);
};

USTRUCT(BlueprintType)
struct FVeilHealthState {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) float Health=100;
 UPROPERTY(BlueprintReadOnly) EVeilLife Life=EVeilLife::Alive;
 UPROPERTY(BlueprintReadOnly) EVeilArmor Armor=EVeilArmor::Medium;
 UPROPERTY(BlueprintReadOnly) EVeilRegion LastRegion=EVeilRegion::Torso;
 UPROPERTY(BlueprintReadOnly) float LastDamage=0;
 UPROPERTY(BlueprintReadOnly) float LastArmorStopped=0;
 UPROPERTY(BlueprintReadOnly) float BleedEndTime=0;
 UPROPERTY(BlueprintReadOnly) int32 DamageRevision=0;
};
UCLASS(ClassGroup=(VeilFall),meta=(BlueprintSpawnableComponent))
class UVeilHealthComponent : public UActorComponent {
 GENERATED_BODY()
public:
 UVeilHealthComponent();
 virtual void TickComponent(float DeltaSeconds,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 UPROPERTY(ReplicatedUsing=OnRep_State,BlueprintReadOnly) FVeilHealthState State;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) bool bAllowDowned=true;
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly) EVeilTeam Team=EVeilTeam::FreeStates;
 static float ArmorProtection(EVeilArmor Armor);
 static float MovementSpeed(EVeilArmor Armor);
 static float DamageAfterArmor(const FVeilWeaponSpec& Spec,EVeilRegion Region,EVeilArmor Armor,float& Stopped);
 bool ApplyShot(const FVeilWeaponSpec& Spec,EVeilRegion Region,AActor* Source);
 bool TrainingDown();
 bool Recover(float Health);
 bool SetArmor(EVeilArmor Armor);
 void ResetHealth();
 UFUNCTION() void OnRep_State();
private:
 void Publish();
 double DeadAt=-1;
 double ProtectedUntil=0;
 bool bDiagnosticSingleHitShield=false;
 friend class ACoopState;
 friend class UVeilCombatComponent;
};

USTRUCT(BlueprintType)
struct FVeilAmmo {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) FName WeaponId;
 UPROPERTY(BlueprintReadOnly) int32 Magazine=0;
 UPROPERTY(BlueprintReadOnly) int32 Reserve=0;
};
USTRUCT(BlueprintType)
struct FVeilCombatState {
 GENERATED_BODY()
 UPROPERTY(BlueprintReadOnly) TArray<FName> Carried;
 UPROPERTY(BlueprintReadOnly) TArray<FVeilAmmo> Ammo;
 UPROPERTY(BlueprintReadOnly) int32 EquippedSlot=0;
 UPROPERTY(BlueprintReadOnly) bool bReloading=false;
 UPROPERTY(BlueprintReadOnly) float ReloadEndTime=0;
 UPROPERTY(BlueprintReadOnly) EVeilUse Use=EVeilUse::None;
 UPROPERTY(BlueprintReadOnly) float UseEndTime=0;
 UPROPERTY(BlueprintReadOnly) int32 MedicalCharges=2;
 UPROPERTY(BlueprintReadOnly) int32 AmmoCharges=1;
 UPROPERTY(BlueprintReadOnly) int32 AcceptedShots=0;
 UPROPERTY(BlueprintReadOnly) FString Feedback;
};
UCLASS(ClassGroup=(VeilFall),meta=(BlueprintSpawnableComponent))
class UVeilCombatComponent : public UActorComponent {
 GENERATED_BODY()
public:
 UVeilCombatComponent();
 virtual void BeginPlay() override;
 virtual void TickComponent(float DeltaSeconds,ELevelTick TickType,FActorComponentTickFunction* TickFunction) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 UPROPERTY(ReplicatedUsing=OnRep_State,BlueprintReadOnly) FVeilCombatState State;
 UPROPERTY(ReplicatedUsing=OnRep_Equipped,BlueprintReadOnly) FName EquippedId="weapon.carbine";
 UPROPERTY(EditAnywhere,Replicated,BlueprintReadOnly) TArray<TObjectPtr<UVeilWeaponDefinition>> Definitions;
 FVeilWeaponSpec Spec(FName Id) const;
 FVeilAmmo* CurrentAmmo();
 const FVeilAmmo* CurrentAmmo() const;
 bool ValidateAmmo() const;
 void StartFire();void StopFire();void RequestShot();void RequestReload();void RequestSlot(int32 Slot);
 void RequestMedical();void RequestAmmoKit();void RequestRespawn();
 void SetADS(bool Enabled){bADS=Enabled;}
 bool IsADS() const{return bADS;}
 bool Restock();bool SwapPrimary();bool CycleArmor();
 bool BeginRevive(APawn* Target);
 void CancelActions();
 UFUNCTION(Server,Reliable) void ServerFire(FVector_NetQuantizeNormal Aim,uint32 Sequence);
 UFUNCTION(Server,Reliable) void ServerReload();
 UFUNCTION(Server,Reliable) void ServerEquip(int32 Slot);
 UFUNCTION(Server,Reliable) void ServerUse(EVeilUse Kind);
 UFUNCTION(Server,Reliable) void ServerRespawn();
 UFUNCTION(Client,Reliable) void ClientShotResult(uint32 Sequence,bool Accepted);
 UFUNCTION(NetMulticast,Unreliable) void MulticastTrace(FVector_NetQuantize Start,FVector_NetQuantize End,bool Hit);
 UFUNCTION() void OnRep_State();
 UFUNCTION() void OnRep_Equipped();
 UFUNCTION(BlueprintImplementableEvent) void LocalShotPresentation();
 UFUNCTION(BlueprintImplementableEvent) void ConfirmedShotPresentation(bool Accepted);
private:
 bool ValidOwner() const;
 bool LocalOwner() const;
 void Publish();
 void CompleteReload();
 bool BeginUse(EVeilUse Kind,UVeilHealthComponent* Target);
 bool UseStillValid() const;
 void CompleteUse();
 void ResolvePellet(const FVeilWeaponSpec& Weapon,const FVector& Eye,const FVector& Direction,const FVector& Breech,const FVector& Muzzle,float& TotalDamage,FString& Region);
 UPROPERTY() TObjectPtr<UVeilHealthComponent> UseTarget;
 FVector UseStartLocation=FVector::ZeroVector;
 int32 UseDamageRevision=0;
 FName ReloadWeapon;
 double LastServerShot=-100;
 double LastLocalShot=-100;
 uint32 LastSequence=0,LocalSequence=0;
 bool bHeld=false,bADS=false;
 int32 ServerSeed=0;
 friend class FG2FireTest;
 friend class FG2ReloadTest;
 friend class FG2ReviveTest;
};

UCLASS()
class ACombatCharacter : public AFoundationCharacter,public IFoundationInteractable {
 GENERATED_BODY()
public:
 ACombatCharacter(const FObjectInitializer& Initializer=FObjectInitializer::Get());
 virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
 virtual bool Activate_Implementation(APawn* User) override;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UVeilHealthComponent> Health;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UVeilCombatComponent> Combat;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class USkeletalMeshComponent> FirstPersonMesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Weapon;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> WorldWeapon;
 virtual void RefreshPresentation();
 UPROPERTY(Replicated) TObjectPtr<class ACombatTarget> DiagnosticTarget;
private:
 void SmokeTick();
 int32 SmokePhase=0;
 double SmokeNext=0;
 UPROPERTY() TObjectPtr<UInputMappingContext> CombatContext;
 UPROPERTY() TArray<TObjectPtr<UInputAction>> CombatActions;
 void FireStart();void FireStop();void Reload();void SlotPrimary();void SlotSecondary();void SlotSidearm();
 void ADSStart();void ADSStop();void Medical();void AmmoKit();void Respawn();
};

UCLASS()
class ACombatTarget : public AActor,public IFoundationInteractable {
 GENERATED_BODY()
public:
 ACombatTarget();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual bool Activate_Implementation(APawn* User) override;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UVeilHealthComponent> Health;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> Label;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UBoxComponent> TorsoHit;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UBoxComponent> HeadHit;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UBoxComponent> LimbHit;
 UPROPERTY(EditAnywhere) EVeilArmor ArmorPreset=EVeilArmor::Medium;
 UPROPERTY(EditAnywhere) FName StableId;
};
UENUM()
enum class ECombatBenchAction : uint8 { PrimarySwap,ArmorCycle,Restock,DownSelf };
UCLASS()
class ACombatLocker : public AActor,public IFoundationInteractable {
 GENERATED_BODY()
public:
 ACombatLocker();
 virtual void BeginPlay() override;
 virtual bool Activate_Implementation(APawn* User) override;
 UPROPERTY(EditAnywhere) ECombatBenchAction Action=ECombatBenchAction::Restock;
 UPROPERTY(EditAnywhere) TArray<TObjectPtr<UVeilWeaponDefinition>> Catalog;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> Label;
};
UCLASS()
class ACombatGameMode : public ADeadSignalGameMode {
 GENERATED_BODY()
public: ACombatGameMode();
};



