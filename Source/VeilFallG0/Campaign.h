#pragma once
#include "CoreMinimal.h"
#include "Foundation.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/SaveGame.h"
#include "GameFramework/PlayerController.h"
#include "Engine/DataAsset.h"
#include "Blueprint/UserWidget.h"
#include "LootTypes.h"
#include "Campaign.generated.h"

UENUM(BlueprintType)
enum class ERelayState : uint8 { Hostile, Cleared, PowerRestored, Established };

USTRUCT(BlueprintType)
struct FDeadSignalRecord {
 GENERATED_BODY()
 UPROPERTY(SaveGame, BlueprintReadOnly) int32 SchemaVersion=2;
 UPROPERTY(SaveGame, BlueprintReadOnly) FName MissionId="operation.dead_signal";
 UPROPERTY(SaveGame, BlueprintReadOnly) int32 NextObjective=0;
 UPROPERTY(SaveGame, BlueprintReadOnly) ERelayState Outpost=ERelayState::Hostile;
 UPROPERTY(SaveGame, BlueprintReadOnly) int32 ClueMask=0;
 UPROPERTY(SaveGame, BlueprintReadOnly) int32 BankedEvidence=0;
 UPROPERTY(SaveGame, BlueprintReadOnly) int32 Revision=0;
 UPROPERTY(SaveGame,BlueprintReadOnly) TArray<FName> ConsumedPickups;
 UPROPERTY(SaveGame,BlueprintReadOnly) TArray<FName> KilledEnemies;
 UPROPERTY(SaveGame,BlueprintReadOnly) TArray<FFieldItem> HostField;
 UPROPERTY(SaveGame,BlueprintReadOnly) TArray<FFieldItem> SupplyStash;
 UPROPERTY(SaveGame,BlueprintReadOnly) TArray<FRecoveryRecord> RecoveryPacks;
 bool Validate() const;
 bool Apply(FName Action);
 bool Migrate();
 FString Summary() const;
};

UCLASS()
class UDeadSignalSave : public USaveGame {
 GENERATED_BODY()
public:
 UPROPERTY(SaveGame) FDeadSignalRecord Record;
};

// No editor dependency. Atomic replacement on Windows, rename on POSIX.
class FDeadSignalStore {
public:
 static bool Write(const FString& Path,const FDeadSignalRecord& Record,FString& Error);
 static bool Read(const FString& Path,FDeadSignalRecord& Record,FString& Error);
};

UCLASS(BlueprintType)
class UG1RouteDefinition : public UPrimaryDataAsset {
 GENERATED_BODY()
public:
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName StableId="operation.dead_signal";
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TArray<FString> Objectives;
 virtual FPrimaryAssetId GetPrimaryAssetId() const override { return FPrimaryAssetId("VeilFallMission",StableId); }
};

UCLASS()
class ADeadSignalState : public AGameStateBase {
 GENERATED_BODY()
public:
 ADeadSignalState();
 virtual void BeginPlay() override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
 UPROPERTY(ReplicatedUsing=OnRep_Record,BlueprintReadOnly) FDeadSignalRecord Record;
 UPROPERTY(Replicated,BlueprintReadOnly) FString Status;
 bool Apply(FName Action,APawn* User);
 bool SaveCampaign();
 virtual bool RestoreCampaign();
 virtual FString SavePath() const;
 UFUNCTION() void OnRep_Record();
 bool bSaveBlocked=false;
};

UCLASS()
class ADeadSignalNode : public AActor,public IFoundationInteractable {
 GENERATED_BODY()
public:
 ADeadSignalNode();
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual bool Activate_Implementation(APawn* User) override;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName StableId;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Action;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FString Prompt;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FString EvidenceStatement;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) TObjectPtr<UG1RouteDefinition> Definition;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> Label;
private:
 int32 SeenRevision=-1;
};

UCLASS()
class UDeadSignalOverlay : public UUserWidget {
 GENERATED_BODY()
protected:
 virtual void NativeOnInitialized() override;
 virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
private:
 UPROPERTY() TObjectPtr<class UTextBlock> Readout;
};

UCLASS()
class ADeadSignalController : public APlayerController {
 GENERATED_BODY()
public:
 virtual void Tick(float DeltaSeconds) override;
 UFUNCTION(Exec) void VFSave();
 UFUNCTION(Exec) void VFLoad();
private:
 UPROPERTY() TObjectPtr<UDeadSignalOverlay> Overlay;
 double SmokeStart=-1;
 bool bCapturePositioned=false;
 bool bCaptureTaken=false;
};

UCLASS()
class ADeadSignalGameMode : public AFoundationGameMode {
 GENERATED_BODY()
public:
 ADeadSignalGameMode();
 virtual void InitGameState() override;
 virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
};

