#if WITH_DEV_AUTOMATION_TESTS
#include "Coop.h"
#include <limits>
#include "Squad.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
namespace {
struct FCoopWorld {
 UWorld* W;ACoopState* State;ACoopCharacter* P1;ACoopCharacter* P2;APlayerController* PC1;APlayerController* PC2;
 FCoopWorld(){W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);W->InitializeActorsForPlay(FURL());W->BeginPlay();W->GetWorldSettings()->NotifyBeginPlay();State=W->SpawnActor<ACoopState>();W->SetGameState(State);P1=W->SpawnActor<ACoopCharacter>();P2=W->SpawnActor<ACoopCharacter>(FVector(500,0,0),FRotator::ZeroRotator);PC1=W->SpawnActor<APlayerController>();PC2=W->SpawnActor<APlayerController>();PC1->SetActorTickEnabled(false);PC2->SetActorTickEnabled(false);PC1->Possess(P1);PC2->Possess(P2);}
 ~FCoopWorld(){P1->FieldItems.Reset();P2->FieldItems.Reset();W->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(W);W->DestroyWorld(false);}
};
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG3LootTest,"VeilFallG0.Coop.ClaimsRecoveryAndBank",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG3LootTest::RunTest(const FString&){
 FCoopWorld F;const FString Root=FPaths::Combine(FPaths::ProjectSavedDir(),"G3Tests",FGuid::NewGuid().ToString());F.State->AutomationSavePath=FPaths::Combine(Root,"Ledger.vfs");
 TestFalse(TEXT("Unknown loot cannot enter inventory"),F.State->ClaimPickup("pickup.bad","loot.invalid",F.P1));
 TestFalse(TEXT("No owner cannot claim"),F.State->ClaimPickup("pickup.a","loot.medical",nullptr));
 TestTrue(TEXT("First simultaneous pickup claim wins"),F.State->ClaimPickup("pickup.a","loot.medical",F.P1));
 TestFalse(TEXT("Second contender cannot duplicate claimed pickup"),F.State->ClaimPickup("pickup.a","loot.medical",F.P2));
 TestEqual(TEXT("Single item in winning inventory"),F.P1->FieldItems.Num(),1);TestEqual(TEXT("Losing inventory empty"),F.P2->FieldItems.Num(),0);
 F.P1->Health->TrainingDown();F.P1->Health->State.BleedEndTime=-1;F.P1->Health->TickComponent(.1f,LEVELTICK_All,nullptr);
 const auto Loadout=F.P1->Combat->State.Carried;const int32 Ammo=F.P1->Combat->CurrentAmmo()->Magazine;
 TestTrue(TEXT("Death drop commits a recovery pack"),F.P1->DropFieldLoot());TestEqual(TEXT("Field emptied once"),F.P1->FieldItems.Num(),0);TestEqual(TEXT("One pack in durable ledger"),F.State->Record.RecoveryPacks.Num(),1);const auto PackId=F.State->Record.RecoveryPacks[0].PackId;
 TestTrue(TEXT("Equipped loadout retained on death"),Loadout==F.P1->Combat->State.Carried);TestEqual(TEXT("Equipped ammo retained on death"),F.P1->Combat->CurrentAmmo()->Magazine,Ammo);
 TestFalse(TEXT("Dead owner cannot claim"),F.State->ClaimPack(PackId,F.P1));TestTrue(TEXT("Alive teammate recovers pack"),F.State->ClaimPack(PackId,F.P2));TestFalse(TEXT("Concurrent second claim rejected"),F.State->ClaimPack(PackId,F.P2));
 TestTrue(TEXT("Banking succeeds"),F.State->BankItems(F.P2));TestFalse(TEXT("Repeated banking cannot duplicate"),F.State->BankItems(F.P2));TestEqual(TEXT("Exactly one banked supply"),F.State->Record.SupplyStash.Num(),1);
 FDeadSignalRecord Loaded;FString Error;TestTrue(TEXT("Ledger reload is real filesystem read"),FDeadSignalStore::Read(F.State->SavePath(),Loaded,Error));TestEqual(TEXT("Banked item survives"),Loaded.SupplyStash.Num(),1);TestEqual(TEXT("Claimed pack absent after reload"),Loaded.RecoveryPacks.Num(),0);
 auto Invalid=Loaded;Invalid.HostField=Invalid.SupplyStash;TestFalse(TEXT("One instance cannot exist in stash and field"),Invalid.Validate());
 const FString FailedPath=FPaths::Combine(Root,"DirectoryIsNotAFile");FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*FailedPath);F.State->AutomationSavePath=FailedPath;AddExpectedError("G3_TRANSACTION_FAILED",EAutomationExpectedErrorFlags::Contains,1);
 TestFalse(TEXT("Failed durable write cannot claim loot"),F.State->ClaimPickup("pickup.failure","loot.parts",F.P2));TestFalse(TEXT("Failed claim does not consume pickup"),F.State->Record.ConsumedPickups.Contains("pickup.failure"));TestEqual(TEXT("Failed claim leaves field unchanged"),F.P2->FieldItems.Num(),0);
 F.State->AutomationSavePath=FPaths::Combine(Root,"Ledger.vfs");F.PC2->UnPossess();TestFalse(TEXT("Unpossessed pawn rejected"),F.State->ClaimPickup("pickup.noowner","loot.parts",F.P2));return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG3PersistenceTest,"VeilFallG0.Coop.MigrationAndRecoveryPersistence",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG3PersistenceTest::RunTest(const FString&){
 FDeadSignalRecord Old;Old.SchemaVersion=1;TestTrue(TEXT("Schema one migrates"),Old.Migrate());TestEqual(TEXT("Schema two current"),Old.SchemaVersion,2);TestTrue(TEXT("Migrated record valid"),Old.Validate());
 FDeadSignalRecord Legacy;FString Error;TestTrue(TEXT("Real archived G1 binary fixture loads"),FDeadSignalStore::Read(FPaths::Combine(FPaths::ProjectContentDir(),"Tests","LegacyG1.vfs"),Legacy,Error));TestEqual(TEXT("Legacy completed mission preserved"),Legacy.NextObjective,9);TestEqual(TEXT("Legacy clues preserved"),Legacy.ClueMask,3);TestEqual(TEXT("Legacy supply ledger starts empty"),Legacy.RecoveryPacks.Num(),0);
 FRecoveryRecord Pack;Pack.PackId=FGuid::NewGuid();Pack.Location=FVector(100,200,20);FFieldItem I;I.InstanceId=FGuid::NewGuid();I.Type="loot.parts";Pack.Items.Add(I);Legacy.RecoveryPacks.Add(Pack);Legacy.KilledEnemies.Add("relay.rifleman");Legacy.ConsumedPickups.Add("edge.parts");
 const FString Path=FPaths::Combine(FPaths::ProjectSavedDir(),"G3Tests",FGuid::NewGuid().ToString(),"Recovery.vfs");TestTrue(TEXT("Pack save succeeds"),FDeadSignalStore::Write(Path,Legacy,Error));FDeadSignalRecord Loaded;TestTrue(TEXT("Pack reload succeeds"),FDeadSignalStore::Read(Path,Loaded,Error));TestEqual(TEXT("Pack ID survives"),Loaded.RecoveryPacks[0].PackId,Pack.PackId);TestEqual(TEXT("Item ID survives"),Loaded.RecoveryPacks[0].Items[0].InstanceId,I.InstanceId);TestTrue(TEXT("Enemy casualty survives"),Loaded.KilledEnemies.Contains("relay.rifleman"));TestTrue(TEXT("Pickup claim survives"),Loaded.ConsumedPickups.Contains("edge.parts"));
 auto Bad=Loaded;Bad.RecoveryPacks.Add(Pack);TestFalse(TEXT("Duplicate pack/instance rejected"),Bad.Validate());Bad=Loaded;Bad.RecoveryPacks[0].Location.X=std::numeric_limits<double>::quiet_NaN();TestFalse(TEXT("Nonfinite pack position rejected"),Bad.Validate());return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG3SquadTest,"VeilFallG0.Coop.SightMemoryAndCoverReservations",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG3SquadTest::RunTest(const FString&){FCoopWorld F;auto* S=F.W->SpawnActor<ASquadCoordinator>();auto* C1=F.W->SpawnActor<ASquadController>();auto* C2=F.W->SpawnActor<ASquadController>();const double Now=F.W->GetTimeSeconds();const FVector Seen(100,200,0);
 TestTrue(TEXT("Timestamped observation shared"),S->Report("relay",F.P1,Seen,Now));TestFalse(TEXT("Older sighting cannot overwrite latest"),S->Report("relay",F.P1,FVector(999),Now-1));FSquadSighting Memory;F.P1->SetActorLocation(FVector(2000,0,0));TestTrue(TEXT("Fresh shared memory available"),S->Recall("relay",Memory));TestTrue(TEXT("Unseen movement does not change stored position"),Memory.Location.Equals(Seen));TestFalse(TEXT("Sighting expires after eight seconds"),Memory.Fresh(Now+9));TestFalse(TEXT("Other encounter has no magical sighting"),S->Recall("convoy",Memory));
 TestTrue(TEXT("First cover reservation succeeds"),S->Reserve("relay.cover.a",C1));TestFalse(TEXT("Squadmate cannot occupy same reserved cover"),S->Reserve("relay.cover.a",C2));S->Release(C1);TestTrue(TEXT("Released cover can be reserved"),S->Reserve("relay.cover.a",C2));TestFalse(TEXT("Future timestamp rejected"),S->Report("relay",F.P1,Seen,Now+100));return true;}
#endif

