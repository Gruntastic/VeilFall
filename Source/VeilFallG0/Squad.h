#pragma once
#include "CoreMinimal.h"
#include "Combat.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "BehaviorTree/BTTaskNode.h"
#include "EnvironmentQuery/EnvQueryGenerator.h"
#include "EnvironmentQuery/EnvQueryTest.h"
#include "Squad.generated.h"

UENUM(BlueprintType)
enum class ESquadRole : uint8 { Rifleman, Support, Marksman };
struct FSquadSighting {
 TWeakObjectPtr<APawn> Target;
 FVector Location=FVector::ZeroVector;
 double Time=-100;
 bool Fresh(double Now)const{return Target.IsValid()&&Now>=Time&&Now-Time<=8;}
};
UCLASS()
class ASquadCoordinator : public AActor {
 GENERATED_BODY()
public:
 ASquadCoordinator();
 bool Report(FName Encounter,APawn* Target,FVector Location,double ObservedTime);
 bool Recall(FName Encounter,FSquadSighting& Result)const;
 bool Reserve(FName CoverId,class ASquadController* User);
 bool Available(FName CoverId,const ASquadController* User)const;
 void Release(const ASquadController* User);
private:
 TMap<FName,FSquadSighting> Sightings;
 struct FReservation {TWeakObjectPtr<ASquadController> User;double Until=0;};
 TMap<FName,FReservation> Reservations;
};
UCLASS()
class ASquadCover : public AActor {
 GENERATED_BODY()
public:
 ASquadCover();
 UPROPERTY(EditAnywhere) FName StableId;
 UPROPERTY(EditAnywhere) FName Encounter="relay";
};
UCLASS()
class ASquadEnemy : public ACombatCharacter {
 GENERATED_BODY()
public:
 ASquadEnemy();
 virtual void BeginPlay()override;
 virtual void RefreshPresentation()override;
 virtual void Tick(float DeltaSeconds)override;
 UPROPERTY(EditAnywhere) FName StableId;
 UPROPERTY(EditAnywhere) FName Encounter="relay";
 UPROPERTY(EditAnywhere) ESquadRole SquadRole=ESquadRole::Rifleman;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UTextRenderComponent> Label;
 UPROPERTY(Replicated) FString TacticalStatus="Guarding";
 virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps)const override;
 bool ActiveEncounter()const;
private:
 double NextCasualtyRetry=0;
};
UCLASS()
class USquadTacticsTask : public UBTTaskNode {
 GENERATED_BODY()
public:
 USquadTacticsTask();
 virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory)override;
 virtual void TickTask(UBehaviorTreeComponent& OwnerComp,uint8* NodeMemory,float DeltaSeconds)override;
};
UCLASS()
class USquadCoverGenerator : public UEnvQueryGenerator {
 GENERATED_BODY()
public:
 USquadCoverGenerator();
 virtual void GenerateItems(FEnvQueryInstance& QueryInstance)const override;
};
UCLASS()
class USquadCoverScore : public UEnvQueryTest {
 GENERATED_BODY()
public:
 USquadCoverScore();
 virtual void RunTest(FEnvQueryInstance& QueryInstance)const override;
};
UCLASS()
class ASquadController : public AAIController {
 GENERATED_BODY()
public:
 ASquadController();
 virtual void OnPossess(APawn* InPawn)override;
 virtual void OnUnPossess()override;
 virtual void Tick(float DeltaSeconds)override;
 UFUNCTION() void SightUpdated(AActor* Actor,FAIStimulus Stimulus);
 void StepTactics();
 bool CoverCandidate(const ASquadCover* Cover,FVector& Projected)const;
 float CoverScore(const FVector& Location)const;
 ASquadCoordinator* Coordinator()const;
 FSquadSighting Memory;
 int32 SuccessfulQueries=0;
 bool HasVisibleTarget()const;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UAIPerceptionComponent> Perception;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UAISenseConfig_Sight> Sight;
private:
 UPROPERTY() TObjectPtr<class UBehaviorTree> RuntimeTree;
 UPROPERTY() TObjectPtr<class UEnvQuery> CoverQuery;
 UPROPERTY() TObjectPtr<class UEnvQueryInstanceBlueprintWrapper> PendingQuery;
 TWeakObjectPtr<APawn> VisibleTarget;
 FVector Home=FVector::ZeroVector;
 double NextThink=0,NextCover=0,LastQueryStart=0,NextFire=0;
 uint32 ShotSequence=0;
 FName ReservedCover;
 bool bBehaviorTreeRunning=false,bFallbackLogged=false;
 void StartCoverQuery();
 void SelectCover(const FVector& Location);
 void CoverFallback();
};


