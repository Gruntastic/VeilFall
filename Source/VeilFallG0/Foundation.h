#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "Components/ActorComponent.h"
#include "UObject/Interface.h"
#include "Foundation.generated.h"
class UCameraComponent;
class UInputAction;
class UInputMappingContext;
struct FInputActionValue;

UINTERFACE(BlueprintType)
class UFoundationInteractable : public UInterface { GENERATED_BODY() };
class IFoundationInteractable { GENERATED_BODY()
public:
 UFUNCTION(BlueprintNativeEvent, BlueprintCallable) bool Activate(APawn* User);
};
UCLASS(ClassGroup=(VeilFall), meta=(BlueprintSpawnableComponent))
class UFoundationInteraction : public UActorComponent { GENERATED_BODY()
public:
 UFoundationInteraction();
 void Request();
 UFUNCTION(Server,Reliable) void ServerRequest();
private: double LastRequest=-1; double LastLocalRequest=-1;
};
UCLASS()
class AFoundationCharacter : public ACharacter { GENERATED_BODY()
public:
 AFoundationCharacter(const FObjectInitializer& Initializer=FObjectInitializer::Get());
 virtual void BeginPlay() override;
 virtual void Tick(float DeltaSeconds) override;
 virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
 UPROPERTY(EditDefaultsOnly, Category="Input", meta=(ClampMin="0.01", ClampMax="2.0")) float MouseSensitivity=0.36f;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UFoundationInteraction> Interaction;
private:
 friend class FG0InputTest;
 UPROPERTY() TObjectPtr<UInputMappingContext> Context;
 UPROPERTY() TObjectPtr<UInputAction> MoveAction;
 UPROPERTY() TObjectPtr<UInputAction> LookAction;
 UPROPERTY() TObjectPtr<UInputAction> JumpAction;
 UPROPERTY() TObjectPtr<UInputAction> InteractAction;
 void Move(const FInputActionValue& Value);
 void Look(const FInputActionValue& Value);
 void Interact();
};
UCLASS()
class AFoundationSwitch : public AActor, public IFoundationInteractable { GENERATED_BODY()
public:
 AFoundationSwitch();
 virtual bool Activate_Implementation(APawn* User) override;
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& Out) const override;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> Label;
 UPROPERTY(ReplicatedUsing=OnRep_Count, VisibleAnywhere) int32 Count=0;
 UFUNCTION() void OnRep_Count();
};
UCLASS()
class AFoundationGameMode : public AGameModeBase { GENERATED_BODY()
public: AFoundationGameMode();
};

