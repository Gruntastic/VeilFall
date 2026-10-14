#if WITH_DEV_AUTOMATION_TESTS
#include "Campaign.h"
#include "Misc/AutomationTest.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/Crc.h"
#include "Serialization/MemoryWriter.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/PlatformFileManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG1StateTest,"VeilFallG0.Campaign.OrderedState",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG1StateTest::RunTest(const FString&) {
 FDeadSignalRecord R;
 TestTrue(TEXT("New campaign consistent"),R.Validate());
 TestFalse(TEXT("Cannot softlock by skipping to chamber"),R.Apply("chamber.reveal"));
 TestFalse(TEXT("Optional clue locked until briefing"),R.Apply("clue.utility"));
 TestTrue(TEXT("Briefing accepted"),R.Apply("hq.brief"));
 TestTrue(TEXT("Optional clue accepted"),R.Apply("clue.utility"));
 TestFalse(TEXT("Repeated clue has no duplicated reward"),R.Apply("clue.utility"));
 TestEqual(TEXT("Quest clue persisted once"),R.ClueMask,1);
 const FName Rest[]={"edge.arrive","relay.clear","relay.power","relay.establish","convoy.recover","facility.enter","chamber.reveal","hq.debrief"};
 for(FName Action:Rest){
  TestTrue(*FString::Printf(TEXT("Ordered action %s"),*Action.ToString()),R.Apply(Action));
  TestTrue(TEXT("State invariant after action"),R.Validate());
  TestFalse(TEXT("Concurrent/repeated action rejected"),R.Apply(Action));
 }
 TestEqual(TEXT("Full route completes"),R.NextObjective,9);
 TestEqual(TEXT("Outpost established"),int32(R.Outpost),3);
 TestEqual(TEXT("Debrief banks evidence once"),R.BankedEvidence,1);
 TestFalse(TEXT("No post-completion reward duplication"),R.Apply("hq.debrief"));
 TestFalse(TEXT("Unknown action rejected"),R.Apply("invented.action"));
 FDeadSignalRecord Broken=R;Broken.Outpost=ERelayState::Hostile;
 TestFalse(TEXT("Inconsistent outpost record rejected"),Broken.Validate());
 Broken=R;Broken.MissionId="unknown";TestFalse(TEXT("Unknown stable mission ID rejected"),Broken.Validate());
 Broken=R;Broken.ClueMask=4;TestFalse(TEXT("Unknown clue bits rejected"),Broken.Validate());
 Broken=R;Broken.SchemaVersion=0;TestTrue(TEXT("Schema zero migration supported"),Broken.Migrate());TestEqual(TEXT("Migration initializes optional clues"),Broken.ClueMask,0);TestTrue(TEXT("Migration preserves completed campaign"),Broken.Validate());
 Broken=R;Broken.SchemaVersion=99;TestFalse(TEXT("Future schema protected"),Broken.Migrate());
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG1SaveTest,"VeilFallG0.Campaign.AtomicSaveRecovery",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG1SaveTest::RunTest(const FString&) {
 const FString Root=FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectSavedDir(),"G1Tests",FGuid::NewGuid().ToString()));
 const FString Path=FPaths::Combine(Root,"Test.vfs");FString Error;
 FDeadSignalRecord R,Loaded;R.Apply("hq.brief");
 TestTrue(TEXT("Write first campaign"),FDeadSignalStore::Write(Path,R,Error));
 TestTrue(TEXT("Read first campaign"),FDeadSignalStore::Read(Path,Loaded,Error));
 TestEqual(TEXT("First revision retained"),Loaded.Revision,R.Revision);
 R.Apply("edge.arrive");R.Apply("clue.archive");
 TestTrue(TEXT("Atomic replace creates backup"),FDeadSignalStore::Write(Path,R,Error));
 TestTrue(TEXT("Primary full roundtrip"),FDeadSignalStore::Read(Path,Loaded,Error));
 TestEqual(TEXT("Clues retained"),Loaded.ClueMask,2);
 TestEqual(TEXT("Mission retained"),Loaded.NextObjective,2);
 TestTrue(TEXT("Prior primary backup exists"),FPaths::FileExists(Path+".bak"));
 FFileHelper::SaveStringToFile(TEXT("interrupted writer"),*(Path+".tmp"));
 TestTrue(TEXT("Uncommitted temp cannot replace campaign"),FDeadSignalStore::Read(Path,Loaded,Error));
 TestEqual(TEXT("Temp ignored"),Loaded.NextObjective,2);
 TArray<uint8> Bytes;FFileHelper::LoadFileToArray(Bytes,*Path);Bytes.Last()^=0xFF;FFileHelper::SaveArrayToFile(Bytes,*Path);
 TestTrue(TEXT("Checksum corruption falls back to backup"),FDeadSignalStore::Read(Path,Loaded,Error));
 TestEqual(TEXT("Backup is prior valid revision"),Loaded.NextObjective,1);
 TestTrue(TEXT("Backup recovery visible"),Error.Contains("Recovered backup"));
 TestTrue(TEXT("Recovered record recommits safely"),FDeadSignalStore::Write(Path,Loaded,Error));
 FDeadSignalRecord Invalid=Loaded;Invalid.SchemaVersion=42;
 TestFalse(TEXT("Unsupported record never overwrites primary"),FDeadSignalStore::Write(Path,Invalid,Error));
 TestTrue(TEXT("Primary survives invalid write"),FDeadSignalStore::Read(Path,Loaded,Error));
 auto* FutureSave=NewObject<UDeadSignalSave>();FutureSave->Record=Loaded;FutureSave->Record.SchemaVersion=99;
 TArray<uint8> Payload,Envelope;UGameplayStatics::SaveGameToMemory(FutureSave,Payload);
 FMemoryWriter Writer(Envelope);uint32 Magic=0x56465331,Format=1,Length=Payload.Num(),CRC=FCrc::MemCrc32(Payload.GetData(),Payload.Num());
 Writer<<Magic<<Format<<Length<<CRC;Writer.Serialize(Payload.GetData(),Payload.Num());Writer.Close();FFileHelper::SaveArrayToFile(Envelope,*Path);
 TestFalse(TEXT("Future primary never rolls back to old backup"),FDeadSignalStore::Read(Path,Loaded,Error));
 TestTrue(TEXT("Future schema explicitly protected"),Error=="Unsupported schema");
 TestFalse(TEXT("Valid old writer cannot overwrite future primary"),FDeadSignalStore::Write(Path,R,Error));
 FFileHelper::SaveStringToFile(TEXT("bad"),*Path);FFileHelper::SaveStringToFile(TEXT("bad"),*(Path+".bak"));
 TestFalse(TEXT("Dual corruption reported"),FDeadSignalStore::Read(Path,Loaded,Error));
 UE_LOG(LogTemp,Log,TEXT("G1_SAVE_TEST_DIRECTORY %s"),*Root);
 return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FG1AuthorityTest,"VeilFallG0.Campaign.AuthorityAndOutpost",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FG1AuthorityTest::RunTest(const FString&) {
 UWorld* W=UWorld::CreateWorld(EWorldType::Game,false);GEngine->CreateNewWorldContext(EWorldType::Game).SetCurrentWorld(W);
 W->InitializeActorsForPlay(FURL());W->BeginPlay();W->GetWorldSettings()->NotifyBeginPlay();
 auto* State=W->SpawnActor<ADeadSignalState>();W->SetGameState(State);State->bSaveBlocked=true; // No player save writes from this test.
 auto* Controller=W->SpawnActor<APlayerController>();auto* Pawn=W->SpawnActor<AFoundationCharacter>();
 TestFalse(TEXT("Null actor cannot advance mission"),State->Apply("hq.brief",nullptr));
 TestFalse(TEXT("Unpossessed pawn cannot advance mission"),State->Apply("hq.brief",Pawn));
 Controller->Possess(Pawn);
 TestFalse(TEXT("Hostile relay cannot resupply"),State->Apply("relay.resupply",Pawn));
 for(FName Action:{FName("hq.brief"),FName("edge.arrive"),FName("relay.clear"),FName("relay.power"),FName("relay.establish")})TestTrue(TEXT("Authoritative outpost action"),State->Apply(Action,Pawn));
 TestTrue(TEXT("Established relay enables resupply marker"),State->Apply("relay.resupply",Pawn));
 auto* Resident=W->SpawnActor<ADeadSignalNode>();Resident->Action="relay.resident";Resident->Tick(0.2f);
 TestFalse(TEXT("Established resident visible"),Resident->IsHidden());
 State->Record=FDeadSignalRecord();State->Record.Revision=100;Resident->Tick(0.2f);
 TestTrue(TEXT("Restored hostile state hides resident"),Resident->IsHidden());
 W->EndPlay(EEndPlayReason::Quit);GEngine->DestroyWorldContext(W);W->DestroyWorld(false);return true;
}
#endif

