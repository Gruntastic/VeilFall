#pragma once
#include "CoreMinimal.h"
#include "Combat.h"
#include "Coop.generated.h"

UCLASS(BlueprintType)
class UFieldLootDefinition : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName StableId="loot.parts";
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FString DisplayName="Equipment parts";
 virtual FPrimaryAssetId GetPrimaryAssetId()const override{return FPrimaryAssetId("VeilFallLoot",StableId);}
};

UCLASS()
class ACoopCharacter : public ACombatCharacter {
 GENERATED_BODY()
public:
 ACoopCharacter(const FObjectInitializer& Initializer=FObjectInitializer::Get());
 virtual void BeginPlay()override;
 virtual void Tick(float DeltaSeconds)override;
 virtual void EndPlay(EEndPlayReason::Type Reason)override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
 UPROPERTY(Replicated,BlueprintReadOnly) TArray<FFieldItem> FieldItems;
 bool IsHost()const;
 bool DropFieldLoot();
 UFUNCTION(Server,Reliable) void ServerDiagnosticAck(int32 Stage);
private:
 void DiagnosticTick();
 double NextProbeInput=0; bool bOwnershipProbeSent=false;
 int32 LoggedProbeStage=-100;
 double NextDropRetry=0; bool bHostFieldInitialized=false;
};

UCLASS()
class ACoopState : public ADeadSignalState {
 GENERATED_BODY()
public:
 ACoopState();
 virtual void BeginPlay()override;
 virtual void Tick(float DeltaSeconds)override;
 virtual bool RestoreCampaign()override;
 virtual FString SavePath()const override;
 bool Commit(FDeadSignalRecord Candidate);
 bool ClaimPickup(FName PickupId,FName Type,ACoopCharacter* User);
 bool ClaimPack(FGuid PackId,ACoopCharacter* User);
 bool DropItems(ACoopCharacter* User);
 bool BankItems(ACoopCharacter* User);
 bool RegisterCasualty(FName EnemyId);
 bool EncounterCleared(FName Encounter)const;
 void ReconcilePacks();
 void QueueDisconnectItems(const TArray<FFieldItem>& Items,const FVector& Location);
 void ProbeAck(ACoopCharacter* User,int32 Stage);
 UPROPERTY(Replicated) int32 ProbeStage=-1;
 UPROPERTY(Replicated) TObjectPtr<ACoopCharacter> ProbeWinner;
 UPROPERTY(Replicated) TObjectPtr<ACoopCharacter> ProbeReviver;
 UPROPERTY(Replicated) TObjectPtr<class AFieldPickup> ProbePickup;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
private:
 void DiagnosticTick();
 void AIDiagnosticTick();
 void PlaceProbePlayer(ACoopCharacter* User,const FVector& Position,const FVector& Target);
 double ProbeTime=0,NextAIReport=0;
 TWeakObjectPtr<ACoopCharacter> ProbeLateJoiner;
 bool bLateAck=false,bReconnectAck=false;
 TMap<FName,FVector> AIInitialPositions,AIMemorySnapshot;
 TMap<FName,int32> AIShotSnapshot;
 bool Eligible(ACoopCharacter* User)const;
 double NextSync=0;
 UPROPERTY() TArray<FRecoveryRecord> PendingDisconnectPacks;
 FString AutomationSavePath; friend class FG3LootTest; friend class FG3PersistenceTest;
};

UCLASS()
class AFieldPickup : public AActor,public IFoundationInteractable {
 GENERATED_BODY()
public:
 AFieldPickup();
 virtual void Tick(float DeltaSeconds)override;
 virtual bool Activate_Implementation(APawn* User)override;
 UPROPERTY(EditAnywhere) FName StableId;
 UPROPERTY(EditAnywhere) TObjectPtr<UFieldLootDefinition> Definition;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> Label;
};
UCLASS()
class ARecoveryPack : public AActor,public IFoundationInteractable {
 GENERATED_BODY()
public:
 ARecoveryPack();
 virtual void Tick(float DeltaSeconds)override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
 virtual bool Activate_Implementation(APawn* User)override;
 UPROPERTY(Replicated) FGuid PackId;
 UPROPERTY(Replicated) int32 ItemCount=0;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> Label;
};
UCLASS()
class AFieldBank : public AActor,public IFoundationInteractable {
 GENERATED_BODY()
public:
 AFieldBank();
 virtual void BeginPlay()override;
 virtual bool Activate_Implementation(APawn* User)override;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> Label;
};
UCLASS()
class ACoopGameMode : public ACombatGameMode {
 GENERATED_BODY()
public:
 ACoopGameMode();
 virtual void PreLogin(const FString& Options,const FString& Address,const FUniqueNetIdRepl& UniqueId,FString& ErrorMessage)override;
 virtual void Logout(AController* Exiting)override;
};







