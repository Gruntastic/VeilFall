#pragma once
#include "CoreMinimal.h"
#include "Tactical.h"
#include "MissionWorkshop.generated.h"
UCLASS()
class AG5Character : public ATacticalCharacter {
 GENERATED_BODY()
public:
 AG5Character();
 virtual void BeginPlay()override;
 virtual void RefreshPresentation()override;
 virtual float PopClearanceHeight()const override;
 virtual UClass* DiagnosticCoverClass()const override;
 virtual float DiagnosticCoverHeight()const override{return 90;}
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="VeilFall|Cover",meta=(ClampMin="75",ClampMax="120",Units="cm")) float WaistCoverHeight=90;
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="VeilFall|Jump",meta=(ClampMin="1",ClampMax="3")) float JumpGravityScale=1.75f;
 UPROPERTY(EditDefaultsOnly,BlueprintReadOnly,Category="VeilFall|Jump",meta=(ClampMin="300",ClampMax="700")) float JumpSpeed=520;
};
UCLASS()
class AG5ProbeCover : public ATacticalProbeCover {
 GENERATED_BODY()
public: AG5ProbeCover();
};
UCLASS()
class AG5State : public ATacticalState {
 GENERATED_BODY()
public: virtual FString SavePath()const override;
private: mutable FString PreviewSavePath;
};
UCLASS()
class AG5GameMode : public ATacticalGameMode {
 GENERATED_BODY()
public: AG5GameMode();
};

