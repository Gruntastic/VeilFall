#if WITH_DEV_AUTOMATION_TESTS
#include "Combat.h"
#include "Misc/AutomationTest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerController.h"
#include "Components/BoxComponent.h"
namespace {
struct FCombatWorld {
 UWorld* W;ACombatCharacter* P;APlayerController* PC;
 FCombatWorld(){W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());W->BeginPlay();W->GetWorldSettings()->NotifyBeginPlay();P=W->SpawnActor<ACombatCharacter>();PC=W->SpawnActor<APlayerController>();PC->SetActorTickEnabled(false);PC->Possess(P);}
 ~FCombatWorld(){W->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(W);W->DestroyWorld(false);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG2FireTest,"VeilFallG0.Combat.AuthorityAndHitRegions",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG2FireTest::RunTest(const FString&){FCombatWorld F;auto* C=F.P->Combat.Get();auto* T=F.W->SpawnActor<ACombatTarget>(FVector(500,0,64),FRotator::ZeroRotator);T->Health->SetArmor(EVeilArmor::Heavy);F.PC->SetControlRotation(FRotator::ZeroRotator);const int32 Initial=C->CurrentAmmo()->Magazine;
 C->ServerFire_Implementation(FVector(-1,0,0),1);TestEqual(TEXT("Implausible aim rejected"),C->CurrentAmmo()->Magazine,Initial);
 C->ServerFire_Implementation(FVector(1,0,0),1);TestEqual(TEXT("Authoritative shot spends one round"),C->CurrentAmmo()->Magazine,Initial-1);TestEqual(TEXT("Heavy torso stops six carbine damage"),T->Health->State.Health,56.f);
 C->ServerFire_Implementation(FVector(1,0,0),2);TestEqual(TEXT("Cadence rejects burst RPC"),C->State.AcceptedShots,1);
 C->LastServerShot=-100;C->ServerFire_Implementation(FVector(1,0,0),1);TestEqual(TEXT("Repeated sequence rejected"),C->State.AcceptedShots,1);
 auto* Wall=F.W->SpawnActor<AActor>();auto* B=NewObject<UBoxComponent>(Wall);Wall->SetRootComponent(B);B->RegisterComponent();B->SetBoxExtent(FVector(5,100,100));B->SetCollisionProfileName("BlockAll");Wall->SetActorLocation(FVector(40,0,64));C->ServerFire_Implementation(FVector(1,0,0),3);TestEqual(TEXT("Blocked barrel cannot damage target"),T->Health->State.Health,56.f);TestEqual(TEXT("Blocked shot still consumes cartridge"),C->CurrentAmmo()->Magazine,Initial-2);
 float Stopped=0;const auto S=C->Spec("weapon.pistol");TestEqual(TEXT("Head bypasses armor and triples damage"),UVeilHealthComponent::DamageAfterArmor(S,EVeilRegion::Head,EVeilArmor::Heavy,Stopped),114.f);TestEqual(TEXT("Limb bypasses armor with lower damage"),UVeilHealthComponent::DamageAfterArmor(S,EVeilRegion::Limb,EVeilArmor::Heavy,Stopped),38.f*.65f);
 Wall->Destroy();T->Destroy();T=F.W->SpawnActor<ACombatTarget>(FVector(500,123,25),FRotator::ZeroRotator);FVector Eye;FRotator View;F.P->GetActorEyesViewPoint(Eye,View);const FVector Angled=(T->GetActorLocation()-Eye).GetSafeNormal();F.PC->SetControlRotation(Angled.Rotation());C->LastServerShot=-100;C->ServerFire_Implementation(Angled,4);TestTrue(TEXT("Oblique muzzle ray reaches eye-hit surface"),T->Health->State.Health<100);
 F.PC->UnPossess();C->LastServerShot=-100;C->ServerFire_Implementation(FVector(1,0,0),5);TestEqual(TEXT("Unpossessed owner rejected"),C->State.AcceptedShots,3);return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG2ReloadTest,"VeilFallG0.Combat.ReloadAndLoadout",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG2ReloadTest::RunTest(const FString&){FCombatWorld F;auto* C=F.P->Combat.Get();TestTrue(TEXT("Initial inventory valid"),C->ValidateAmmo());for(FName Id:{FName("weapon.carbine"),FName("weapon.shotgun"),FName("weapon.marksman"),FName("weapon.pistol")})TestTrue(TEXT("Weapon archetype valid"),C->Spec(Id).Validate());TestFalse(TEXT("Unknown weapon rejected"),C->Spec("unknown").Validate());
 C->CurrentAmmo()->Magazine=7;C->CurrentAmmo()->Reserve=10;C->ServerReload_Implementation();TestTrue(TEXT("Reload channel starts"),C->State.bReloading);TestEqual(TEXT("No ammo transfer before completion"),C->CurrentAmmo()->Magazine,7);C->CompleteReload();TestEqual(TEXT("Limited reserve transferred"),C->CurrentAmmo()->Magazine,17);TestEqual(TEXT("Reserve exhausted"),C->CurrentAmmo()->Reserve,0);
 C->Restock();C->CurrentAmmo()->Magazine=3;C->ServerReload_Implementation();C->ServerEquip_Implementation(1);TestFalse(TEXT("Switch interrupts reload"),C->State.bReloading);C->ServerEquip_Implementation(0);TestEqual(TEXT("Interrupted reload does not invent ammo"),C->CurrentAmmo()->Magazine,3);C->ServerEquip_Implementation(99);TestEqual(TEXT("Invalid slot rejected"),C->State.EquippedSlot,0);C->SwapPrimary();TestEqual(TEXT("Locker equips marksman"),C->EquippedId,FName("weapon.marksman"));C->SwapPrimary();TestEqual(TEXT("Swap retains carbine magazine"),C->CurrentAmmo()->Magazine,3);
 C->CurrentAmmo()->Reserve=10000;TestFalse(TEXT("Reserve overflow caught"),C->ValidateAmmo());C->Restock();TestTrue(TEXT("Restock restores ammo limits"),C->ValidateAmmo());return true;}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG2ReviveTest,"VeilFallG0.Combat.ReviveAndCharges",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG2ReviveTest::RunTest(const FString&){FCombatWorld F;auto* C=F.P->Combat.Get();auto* Target=F.W->SpawnActor<ACombatCharacter>(FVector(200,0,0),FRotator::ZeroRotator);TestTrue(TEXT("Explicit training station downs actor"),Target->Health->TrainingDown());TestTrue(TEXT("Alive nearby teammate starts revive"),C->BeginRevive(Target));TestEqual(TEXT("Charge unspent during channel"),C->State.MedicalCharges,2);F.P->SetActorLocation(FVector(0,100,0));TestFalse(TEXT("Moving interrupts channel"),C->UseStillValid());C->CompleteUse();TestEqual(TEXT("Cancelled revive preserves charge"),C->State.MedicalCharges,2);F.P->SetActorLocation(FVector::ZeroVector);TestTrue(TEXT("Second revive starts"),C->BeginRevive(Target));C->CompleteUse();TestEqual(TEXT("Revived teammate alive"),Target->Health->State.Life,EVeilLife::Alive);TestEqual(TEXT("Revive spends exactly one charge"),C->State.MedicalCharges,1);TestFalse(TEXT("Cannot revive already recovered actor"),C->BeginRevive(Target));
 F.P->Health->TrainingDown();C->ServerUse_Implementation(EVeilUse::Medical);TestEqual(TEXT("Downed self medical can start"),C->State.Use,EVeilUse::Medical);C->CompleteUse();TestEqual(TEXT("Self medical recovers"),F.P->Health->State.Life,EVeilLife::Alive);TestEqual(TEXT("No medical charge duplication"),C->State.MedicalCharges,0);F.P->Health->TrainingDown();F.P->Health->State.BleedEndTime=-1;F.P->Health->TickComponent(.1f,LEVELTICK_All,nullptr);TestEqual(TEXT("Bleed timeout causes death"),F.P->Health->State.Life,EVeilLife::Dead);TestEqual(TEXT("Death retains carried loadout"),C->State.Carried.Num(),3);return true;}
#endif



