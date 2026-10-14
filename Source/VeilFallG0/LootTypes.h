#pragma once
#include "CoreMinimal.h"
#include "LootTypes.generated.h"

USTRUCT(BlueprintType)
struct FFieldItem {
 GENERATED_BODY()
 UPROPERTY(SaveGame,BlueprintReadOnly) FGuid InstanceId;
 UPROPERTY(SaveGame,BlueprintReadOnly) FName Type;
 bool Validate() const {return InstanceId.IsValid()&&(Type=="loot.medical"||Type=="loot.ammunition"||Type=="loot.parts");}
};
USTRUCT(BlueprintType)
struct FRecoveryRecord {
 GENERATED_BODY()
 UPROPERTY(SaveGame,BlueprintReadOnly) FGuid PackId;
 UPROPERTY(SaveGame,BlueprintReadOnly) FVector Location=FVector::ZeroVector;
 UPROPERTY(SaveGame,BlueprintReadOnly) TArray<FFieldItem> Items;
};
