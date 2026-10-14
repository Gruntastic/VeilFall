#pragma once
// VEILFALL intro mission ("Hale property"). Greybox systems for L_VF_IntroProperty.
// Every class here gets a BP_VF_* Blueprint child under /Game/VeilFall so tuning and art can be done in Blueprint.
#include "CoreMinimal.h"
#include "MissionWorkshop.h"
#include "AIController.h"
#include "Engine/DataTable.h"
#include "GameFramework/GameModeBase.h"
#include "VFIntro.generated.h"

class UStaticMeshComponent;
class UTextRenderComponent;
class UPointLightComponent;
class USoundBase;
class AVFMissionController;
class UTextBlock;
class UBorder;
class UCanvasPanel;

UENUM(BlueprintType)
enum class EVFIntroState : uint8 {
 NORMAL_HOME, EMERGENCY_ALERT, MOVE_TO_SHOP, OPEN_VAULT, EQUIP, LOCALS_ARRIVE, SECURE_CHILDREN, NCD_ARRIVAL, NCD_DEMAND,
 DEFENSE_ARMED, PROPERTY_ATTACK, HOLD_SHOP, VANGUARD_ARRIVAL, EXTRACT_FAMILY, EVACUATE, SCUTTLE_PROPERTY, MISSION_COMPLETE
};
UENUM(BlueprintType)
enum class EVFFaction : uint8 { Family, Local, NCD, Vanguard };
UENUM(BlueprintType)
enum class EVFDefenseState : uint8 { Safe, Armed, Triggered };

// One subtitle line. Rows live in DT_VF_IntroDialogue (CSV source: Content/VeilFall/Data/IntroDialogue.csv) and play in table order per Beat.
USTRUCT(BlueprintType)
struct FVFDialogueLine : public FTableRowBase {
 GENERATED_BODY()
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FName Beat;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FString Speaker;
 UPROPERTY(EditAnywhere,BlueprintReadOnly) FString Text;
 // 0 = derive from text length.
 UPROPERTY(EditAnywhere,BlueprintReadOnly) float Duration=0;
};

UINTERFACE(BlueprintType)
class UVFPromptSource : public UInterface { GENERATED_BODY() };
class IVFPromptSource { GENERATED_BODY()
public:
 // Empty text = not interactable right now.
 virtual FText GetPrompt(APawn* User) const {return FText();}
};

// Named location used by scripting (NPC stations, spawn points, player checkpoints, camera spots).
UCLASS()
class AVFMarker : public AActor { GENERATED_BODY()
public:
 AVFMarker();
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") FName MarkerId;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UArrowComponent> Arrow;
};

// Cover position for scripted squads. Group decides which wave/side may use it.
UCLASS()
class AVFCoverPoint : public AActor { GENERATED_BODY()
public:
 AVFCoverPoint();
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") FName Group="drive";
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UArrowComponent> Arrow;
};

// Doors, gate, vault door, fold-down defense wall. Hinge moves between closed and open relative transforms. Timer driven, no Tick.
UCLASS()
class AVFAnimatedProp : public AActor { GENERATED_BODY()
public:
 AVFAnimatedProp();
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") FName PropId;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") FVector OpenOffset=FVector::ZeroVector;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") FRotator OpenRotation=FRotator::ZeroRotator;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") float Duration=1.5f;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") FLinearColor Color=FLinearColor(.35f,.36f,.38f);
 UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Hinge;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
 UFUNCTION(BlueprintCallable,Category="VeilFall") void SetOpen(bool bOpen,bool bInstant=false);
 UFUNCTION(BlueprintPure,Category="VeilFall") bool IsOpen() const {return bOpenTarget;}
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void BeginPlay() override;
 UFUNCTION(CallInEditor,BlueprintCallable,Category="VeilFall") void Rebuild(){RerunConstructionScripts();}
private:
 void Step();
 FTimerHandle Timer; float Alpha=0; bool bOpenTarget=false;
};

// Generic usable thing: talk/inspect/equipment station/panel. Gameplay meaning is decided by AVFMissionController via InteractionId.
UCLASS()
class AVFInteractable : public AActor, public IFoundationInteractable, public IVFPromptSource { GENERATED_BODY()
public:
 AVFInteractable();
 virtual bool Activate_Implementation(APawn* User) override;
 virtual FText GetPrompt(APawn* User) const override;
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void BeginPlay() override;
 UFUNCTION(CallInEditor,BlueprintCallable,Category="VeilFall") void Rebuild(){RerunConstructionScripts();}
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") FName InteractionId;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") FText Prompt;
 // Empty = usable in every state.
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") TArray<EVFIntroState> ActiveStates;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") bool bOneShot=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") bool bHideWhenUsed=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") FLinearColor Color=FLinearColor(.5f,.5f,.5f);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") FString ScreenText;
 UPROPERTY(BlueprintReadOnly,Category="VeilFall") bool bUsed=false;
 UPROPERTY(BlueprintReadWrite,Category="VeilFall") bool bEnabled=true;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UTextRenderComponent> Screen;
 UFUNCTION(BlueprintCallable,Category="VeilFall") void ResetUse();
 UFUNCTION(BlueprintCallable,Category="VeilFall") void SetScreenText(const FString& Text,FLinearColor TextColor=FLinearColor::White);
 bool UsableNow() const;
};

// Fictional road-denial / shop-defense panel. Only SAFE, ARMED and TRIGGER exist; all effects are scripted VFX.
UCLASS()
class AVFDefensePanel : public AVFInteractable { GENERATED_BODY()
public:
 AVFDefensePanel();
 virtual FText GetPrompt(APawn* User) const override;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") EVFDefenseState DefenseState=EVFDefenseState::Safe;
 UPROPERTY(BlueprintReadWrite,Category="VeilFall") bool bTriggerReady=false;
 UFUNCTION(BlueprintCallable,Category="VeilFall") void SetDefenseState(EVFDefenseState NewState);
};

// Scripted damage zone along the approach. Kills NCD pawns and wrecks vehicles inside when triggered.
UCLASS()
class AVFScriptedDamageZone : public AActor { GENERATED_BODY()
public:
 AVFScriptedDamageZone();
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") FName ZoneGroup="driveway";
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") float Delay=0;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UBoxComponent> Box;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> ArmedLight;
 UFUNCTION(BlueprintCallable,Category="VeilFall") void SetArmed(bool bArmed);
 void Trigger(AVFMissionController* Mission);
};

// Short-lived greybox explosion: flash sphere, light, rising smoke puffs, sound. Purely visual.
UCLASS()
class AVFBlastEffect : public AActor { GENERATED_BODY()
public:
 AVFBlastEffect();
 void Init(float Scale,USoundBase* Sound,bool bLingeringFire);
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Flash;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Light;
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Smoke;
private:
 void Step(); FTimerHandle Timer; float Age=0,Size=1; bool bFire=false;
};

// Box-built placeholder vehicle that follows scripted routes (TEMP_SUV_NCD, TEMP_Truck_Sheriff, TEMP_Vanguard_Vehicle...).
UCLASS()
class AVFScriptedVehicle : public AActor { GENERATED_BODY()
public:
 AVFScriptedVehicle();
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void BeginPlay() override;
 UFUNCTION(CallInEditor,BlueprintCallable,Category="VeilFall") void Rebuild(){RerunConstructionScripts();}
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") FName VehicleId;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") FLinearColor Color=FLinearColor(.03f,.035f,.04f);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") bool bArmored=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") bool bBloodied=false;
 UPROPERTY(BlueprintReadOnly,Category="VeilFall") bool bWrecked=false;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Body;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Cabin;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Blood;
 UPROPERTY(VisibleAnywhere) TObjectPtr<USceneComponent> Seat;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UPointLightComponent> Headlights;
 UPROPERTY() TArray<TObjectPtr<UStaticMeshComponent>> Wheels;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UAudioComponent> Engine;
 void Drive(const TArray<FVector>& Points,float Speed,TFunction<void()> OnArrive,bool bSlideStop=false);
 void Stop();
 bool IsDriving() const {return Route.Num()>0;}
 UFUNCTION(BlueprintCallable,Category="VeilFall") void Wreck(AVFMissionController* Mission);
 UFUNCTION(BlueprintCallable,Category="VeilFall") void Restore(FTransform At);
 FVector Dismount(int32 Index) const;
private:
 void Step(); void Paint();
 FTimerHandle Timer; TArray<FVector> Route; float SpeedCm=1000; TFunction<void()> Arrive; bool bSlide=false; float SlideYaw=0;
};

// Greybox tree line / fence line built from instanced meshes along Start->End (cheap; one actor per line).
UCLASS()
class AVFGreyboxLine : public AActor { GENERATED_BODY()
public:
 AVFGreyboxLine();
 virtual void OnConstruction(const FTransform& Transform) override;
 virtual void BeginPlay() override;
 UFUNCTION(CallInEditor,BlueprintCallable,Category="VeilFall") void Rebuild(){RerunConstructionScripts();}
 UPROPERTY(EditAnywhere,Category="VeilFall") bool bTrees=true;
 UPROPERTY(EditAnywhere,Category="VeilFall",meta=(MakeEditWidget)) FVector End=FVector(5000,0,0);
 UPROPERTY(EditAnywhere,Category="VeilFall") float Spacing=600;
 UPROPERTY(EditAnywhere,Category="VeilFall") float Jitter=250;
 UPROPERTY(EditAnywhere,Category="VeilFall") int32 Rows=2;
 UPROPERTY(EditAnywhere,Category="VeilFall") int32 Seed=7;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UInstancedStaticMeshComponent> Primary;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UInstancedStaticMeshComponent> Secondary;
};

// Every human in the intro: family, locals, NCD, Vanguard. Reuses the project's combat character (health, armor, weapons, hitboxes).
UCLASS()
class AVFIntroNPC : public ACombatCharacter, public IVFPromptSource { GENERATED_BODY()
public:
 AVFIntroNPC(const FObjectInitializer& Initializer=FObjectInitializer::Get());
 virtual void BeginPlay() override;
 virtual void RefreshPresentation() override;
 virtual bool Activate_Implementation(APawn* User) override;
 virtual FText GetPrompt(APawn* User) const override;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") FName RoleId;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") FString DisplayName;
 UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="VeilFall") EVFFaction Faction=EVFFaction::Family;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") FLinearColor ClothingColor=FLinearColor(.25f,.3f,.4f);
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") bool bArmed=false;
 // Story-critical characters cannot die during the prototype.
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") bool bEssential=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") bool bHelmet=false;
 // Children: Game Animation Sample clips (retargeted to the UE5 mannequin) picked by speed,
 // with idle breaks, an alert variant after the emergency alert, and a crouched huddle in the vault.
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") bool bKidAnimation=false;
 UPROPERTY(BlueprintReadWrite,Category="VeilFall") bool bAlert=false;
 UPROPERTY(BlueprintReadWrite,Category="VeilFall") bool bHuddle=false;
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") float BodyScale=1;
 // Per-hit damage of this NPC's rifle (0 = project default carbine). Keeps the intro fight about pressure, not one-shot deaths.
 UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="VeilFall") float WeaponDamage=0;
 UPROPERTY(BlueprintReadWrite,Category="VeilFall") FText TalkPrompt;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Vest;
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> HelmetMesh;
 UFUNCTION(BlueprintCallable,Category="VeilFall") void SetArmed(bool bNewArmed);
 UFUNCTION(BlueprintCallable,Category="VeilFall") void SetPresent(bool bPresent);
 bool IsAlive() const;
 bool IsHostileTo(const ACombatCharacter* Other) const;
private:
 void ApplyAnimation(); bool bDeathPlayed=false;
 void KidAnimTick(); FTimerHandle KidTimer; double NextBreak=0,BreakUntil=0;
 void EssentialHeal(); FTimerHandle HealTimer;
};

UENUM()
enum class EVFAIMode : uint8 { Idle, Move, Hold, Assault, Retreat };

UCLASS()
class AVFIntroAIController : public AAIController { GENERATED_BODY()
public:
 AVFIntroAIController();
 virtual void OnPossess(APawn* InPawn) override;
 virtual void OnUnPossess() override;
 virtual void OnMoveCompleted(FAIRequestID RequestID,const FPathFollowingResult& Result) override;
 void GoTo(const FVector& Location,TFunction<void()> OnArrive,bool bRun=false);
 void HoldAt(const FVector& Location,const FRotator& Facing);
 void Assault(FName CoverGroup,float AdvanceInterval);
 void Retreat(const FVector& Location);
 void Idle(const FRotator& Facing);
 EVFAIMode Mode=EVFAIMode::Idle;
 FName CoverGroup;
 float Accuracy=3; // cone degrees at 20 m
 bool bCanThrowGrenades=false;
 FVector AdvanceToward=FVector::ZeroVector; // zero = mission defend point
private:
 void Think();
 ACombatCharacter* FindTarget(bool& bVisible) const;
 void Fire(ACombatCharacter* Target);
 void PickCover(bool bAdvance);
 AVFMissionController* Mission() const;
 FTimerHandle ThinkTimer; TFunction<void()> Arrive; FVector HoldLocation=FVector::ZeroVector; FRotator HoldFacing=FRotator::ZeroRotator;
 TWeakObjectPtr<AVFCoverPoint> Cover; TWeakObjectPtr<ACombatCharacter> Target;
 double NextFire=0,NextAdvance=0,TargetSince=0,NextGrenade=0,BurstPause=0; FAIRequestID ActiveMove; int32 Burst=0; uint32 Sequence=0; int32 SeenDamage=0; float AdvanceEvery=12; bool bMoving=false;
};

UCLASS()
class AVFGrenade : public AActor { GENERATED_BODY()
public:
 AVFGrenade();
 UPROPERTY(VisibleAnywhere) TObjectPtr<UStaticMeshComponent> Mesh;
 UPROPERTY(VisibleAnywhere) TObjectPtr<class UProjectileMovementComponent> Movement;
 void Launch(const FVector& Velocity,AActor* Thrower);
private:
 void Explode(); FTimerHandle Fuse; TWeakObjectPtr<AActor> Source;
};

// Player for the intro: the project's G5 character, unarmed until the vault.
UCLASS()
class AVFIntroCharacter : public AG5Character { GENERATED_BODY()
public:
 AVFIntroCharacter();
 virtual void BeginPlay() override;
 virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
 virtual void RefreshPresentation() override;
 UFUNCTION(BlueprintCallable,Category="VeilFall") void SetArmed(bool bNewArmed);
 UPROPERTY(BlueprintReadOnly,Category="VeilFall") bool bArmed=false;
 void SetRiding(USceneComponent* Seat);
 bool bRiding=false;
private:
 UPROPERTY() TObjectPtr<class UInputMappingContext> BlockedContext;
 UPROPERTY() TObjectPtr<class UInputMappingContext> UnarmedContext;
 UPROPERTY() TArray<TObjectPtr<class UInputAction>> UnarmedActions;
 void ApplyUnarmedContext();
};

// Runs the intro as explicit mission states. Actors react to OnStateChanged; jump/reset paths rebuild the world for any state.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FVFStateChanged,EVFIntroState,NewState);
UCLASS()
class AVFMissionController : public AActor { GENERATED_BODY()
public:
 AVFMissionController();
 virtual void BeginPlay() override;
 UPROPERTY(BlueprintReadOnly,Category="VeilFall") EVFIntroState State=EVFIntroState::NORMAL_HOME;
 UPROPERTY(BlueprintAssignable,Category="VeilFall") FVFStateChanged OnStateChanged;
 UFUNCTION(BlueprintImplementableEvent,Category="VeilFall") void ReceiveStateEntered(EVFIntroState NewState);
 UPROPERTY(EditAnywhere,Category="VeilFall") TSoftObjectPtr<UDataTable> Dialogue;
 UPROPERTY(EditAnywhere,Category="VeilFall") TSubclassOf<AVFIntroNPC> LocalClass;
 UPROPERTY(EditAnywhere,Category="VeilFall") TSubclassOf<AVFIntroNPC> NCDClass;
 UPROPERTY(EditAnywhere,Category="VeilFall") TSubclassOf<AVFIntroNPC> VanguardClass;
 UPROPERTY(EditAnywhere,Category="VeilFall") TSubclassOf<AVFScriptedVehicle> VehicleClass;
 UPROPERTY(EditAnywhere,Category="VeilFall") FString VaultCode="0614";
 UPROPERTY(EditAnywhere,Category="VeilFall") TSoftObjectPtr<USoundBase> AlertSound;
 UPROPERTY(EditAnywhere,Category="VeilFall") TSoftObjectPtr<USoundBase> ExplosionSound;
 UPROPERTY(EditAnywhere,Category="VeilFall") TSoftObjectPtr<USoundBase> GunshotSound;
 UPROPERTY(EditAnywhere,Category="VeilFall") TSoftObjectPtr<USoundBase> EngineSound;
 UPROPERTY(EditAnywhere,Category="VeilFall") TSoftObjectPtr<USoundBase> PASound;

 UFUNCTION(BlueprintCallable,Category="VeilFall|Debug") void JumpToState(EVFIntroState Target);
 UFUNCTION(BlueprintCallable,Category="VeilFall|Debug") void RestartCheckpoint();
 UFUNCTION(BlueprintCallable,Category="VeilFall|Debug") void ResetPropertyFight();
 UFUNCTION(BlueprintCallable,Category="VeilFall|Debug") void DebugVault(bool bOpen);
 static AVFMissionController* Get(const UObject* WorldContext);
 static FString StateName(EVFIntroState S);

 // Called by actors.
 bool CanUse(const AVFInteractable* Item) const;
 void OnInteract(AActor* Item,FName Id,APawn* User);
 void OnKeypadResult(bool bCorrect);
 void OnPawnKilled(AVFIntroNPC* Pawn);
 bool RequestShotToken(AActor* Shooter,AActor* Target);
 void Grenade(const FVector& At,AActor* Source);
 void SpawnBlast(const FVector& At,float Scale,bool bFire=false,bool bSound=true);
 void PlayShot(const FVector& At);
 void ReserveCover(AVFCoverPoint* Cover,AController* User);
 bool CoverFree(const AVFCoverPoint* Cover,const AController* User) const;
 FVector DefendPoint() const;
 bool Talkable(const AVFIntroNPC* NPC) const;
 FText TalkPromptFor(const AVFIntroNPC* NPC) const;
 USoundBase* Gunshot() const {return GunshotLoaded;}
 USoundBase* Engine() const {return EngineLoaded;}
 void OnPlayerUse();
 FString ForcedPrompt;
private:
 // Flow
 void Enter(EVFIntroState S);
 void Go(EVFIntroState Next);
 void SetStateValue(EVFIntroState S);
 void ArrangeWorld(EVFIntroState S);
 void CleanupSpawned();
 void Lines(FName Beat,TFunction<void()> Then=nullptr);
 void Bark(FName Beat);
 void After(float Seconds,TFunction<void()> Fn);
 void Objective(const FString& Text);
 void Hint(const FString& Text);
 void Checkpoint();
 void WatchPlayer();
 void TryAlert(bool bForce);
 void CheckEquip();
 void KeypadPrompt();
 void LookAtHouse();
 void Autoplay();
 FTimerHandle AutoTimer,NavTimer; int32 NavReports=0; int32 AutoKills=0; double AutoLastKill=0;
 void LocalsStandoff();
 void BeginWave(int32 Wave);
 void CheckWave();
 void StartColumn();
 void ColumnReady();
 void TriggerDriveway();
 void FoldWall();
 void VanguardEnters();
 void CommanderReveal();
 void Board();
 void Scuttle();
 void ThrowTrigger();
 void SpawnLocals(const FVector& Near);
 void SpawnGateTroops();
 void SpawnVanguard(bool bSettled);
 void SendNPC(FName Role,FName Station,TFunction<void()> Then=nullptr,bool bRun=false);
 void PlaceNPC(FName Role,FName Station);
 void HideNPC(FName Role,bool bHidden);
 AVFIntroNPC* SpawnNPC(TSubclassOf<AVFIntroNPC> Class,FName Role,const FString& Name,EVFFaction Faction,const FVector& At,const FRotator& Rot,TFunction<void(AVFIntroNPC*)> Configure=nullptr);
 AVFScriptedVehicle* SpawnVehicle(FName Id,const FVector& At,float Yaw,FLinearColor Color,bool bArmored);
 void PlacePlayer(FName Station);
 FTransform MarkerTransform(FName Id,bool* bFound=nullptr) const;
 TArray<AVFMarker*> MarkersWithPrefix(const FString& Prefix) const;
 AVFIntroNPC* NPC(FName Role) const;
 AVFInteractable* Item(FName Id) const;
 AVFAnimatedProp* Prop(FName Id) const;
 AVFScriptedVehicle* Vehicle(FName Id) const;
 class AVFIntroPlayerController* PC() const;
 class AVFIntroCharacter* Player() const;
 int32 LivingNCD() const;

 UPROPERTY() TArray<TObjectPtr<AActor>> Spawned;
 UPROPERTY() TMap<FName,TObjectPtr<AVFMarker>> Markers;
 UPROPERTY() TObjectPtr<USoundBase> AlertLoaded;
 UPROPERTY() TObjectPtr<USoundBase> ExplosionLoaded;
 UPROPERTY() TObjectPtr<USoundBase> GunshotLoaded;
 UPROPERTY() TObjectPtr<USoundBase> EngineLoaded;
 UPROPERTY() TObjectPtr<USoundBase> PALoaded;
 UPROPERTY() TObjectPtr<UDataTable> DialogueLoaded;
 UPROPERTY() TObjectPtr<class USoundAttenuation> ShotAttenuation;
 UPROPERTY() TObjectPtr<class USoundAttenuation> EngineAttenuation;
 TMap<TWeakObjectPtr<AVFCoverPoint>,TWeakObjectPtr<AController>> CoverUse;
 TMap<TWeakObjectPtr<AActor>,double> ShotTokens;
 TMap<TWeakObjectPtr<AActor>,FTransform> ScuttleOrigins;
 TMap<TWeakObjectPtr<AActor>,FTransform> CollapseTargets;
 TSet<TWeakObjectPtr<AVFIntroNPC>> Killed;
 TSet<FName> Talked;
 TSet<FName> Gear;
 EVFIntroState CheckpointState=EVFIntroState::NORMAL_HOME;
 int32 Epoch=0,Wave=0,LineSerial=0,CheckpointWave=0,ColumnArrived=0;
 double WaveStart=0,LastGrenade=-100,StateStart=0,LastShotSound=0;
 float CollapseAlpha=0;
 bool bRestarting=false,bScuttled=false,bThrown=false,bLocalsInside=false,bKidsInVault=false,bRadioGiven=false,bVanguardIn=false;
 bool bGrenadeHinted=false,bAlertQueued=false,bTruckStopped=false,bStandoff=false,bEastHinted=false,bHealHinted=false,bLocalsBriefed=false;
 bool bCamerasReady=false,bDemandDone=false,bRefused=false,bWallReady=false,bEvacReady=false,bVaultOpening=false,bGearDone=false,bColumnHalted=false;
 FTimerHandle WatchTimer,WaveTimer,CollapseTimer;
};

UCLASS()
class UVFIntroHUD : public UUserWidget { GENERATED_BODY()
public:
 void ShowSubtitle(const FString& Speaker,const FString& Text,float Seconds);
 void ShowObjective(const FString& Text);
 void ShowHint(const FString& Text,float Seconds=7);
 void ShowAlert(bool bShow);
 void FadeTo(float Opacity,float Seconds);
 void ShowTitle(bool bShow);
 void ShowKeypad(bool bShow);
 void PulseDamage(float Strength);
 void GrenadeWarning(float Seconds);
 void ShowDebug(bool bShow);
 bool IsKeypadOpen() const {return bKeypad;}
protected:
 virtual void NativeOnInitialized() override;
 virtual void NativeTick(const FGeometry& Geometry,float DeltaSeconds) override;
 virtual FReply NativeOnKeyDown(const FGeometry& Geometry,const FKeyEvent& Event) override;
private:
 UTextBlock* Text(UCanvasPanel* Canvas,int32 Size,FLinearColor Color,FVector2D Anchor,FVector2D Offset,FVector2D Align,bool bAutoSize=true,FVector2D Size2=FVector2D::ZeroVector);
 UBorder* Panel(UCanvasPanel* Canvas,FLinearColor Color,FAnchors Anchors,FMargin Offsets);
 UPROPERTY() TObjectPtr<UTextBlock> Subtitle; UPROPERTY() TObjectPtr<UTextBlock> ObjectiveText; UPROPERTY() TObjectPtr<UTextBlock> HintText;
 UPROPERTY() TObjectPtr<UTextBlock> PromptText; UPROPERTY() TObjectPtr<UTextBlock> Crosshair; UPROPERTY() TObjectPtr<UTextBlock> Status;
 UPROPERTY() TObjectPtr<UTextBlock> AlertText; UPROPERTY() TObjectPtr<UBorder> AlertPanel; UPROPERTY() TObjectPtr<UBorder> FadePanel;
 UPROPERTY() TObjectPtr<UTextBlock> Title; UPROPERTY() TObjectPtr<UBorder> KeypadPanel; UPROPERTY() TObjectPtr<UTextBlock> KeypadText;
 UPROPERTY() TObjectPtr<UBorder> DamagePanel; UPROPERTY() TObjectPtr<UTextBlock> GrenadeText; UPROPERTY() TObjectPtr<UTextBlock> DebugText;
 FString Code; bool bKeypad=false,bDebug=false; double SubtitleUntil=0,HintUntil=0,ObjectiveUntil=0,GrenadeUntil=0,LastPromptTrace=0;
 float Fade=0,FadeTarget=0,FadeSpeed=1,Damage=0,AlertFlash=0; int32 LastMagazine=-1; FName LastWeapon;
};

UCLASS()
class AVFIntroPlayerController : public APlayerController { GENERATED_BODY()
public:
 virtual void BeginPlay() override;
 virtual void SetupInputComponent() override;
 UPROPERTY() TObjectPtr<UVFIntroHUD> HUD;
 void OpenKeypad();
 void CloseKeypad(bool bSubmitted,const FString& Code);
 // Development commands (console): VFState NCD_ARRIVAL, VFSkip firefight, VFResetFight, VFOpenVault, VFResetVault, VFCheckpoint.
 UFUNCTION(Exec) void VFState(const FString& Name);
 UFUNCTION(Exec) void VFSkip(const FString& What);
 UFUNCTION(Exec) void VFResetFight();
 UFUNCTION(Exec) void VFOpenVault();
 UFUNCTION(Exec) void VFResetVault();
 UFUNCTION(Exec) void VFCheckpoint();
private:
 void UsePressed();
 void ToggleDebug(); void DebugNext(); void DebugPrev(); bool bDebug=false;
};

UCLASS()
class AVFIntroGameMode : public AGameModeBase { GENERATED_BODY()
public: AVFIntroGameMode();
};
