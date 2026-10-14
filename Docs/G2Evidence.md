# G2 evidence — VF-G2-20261003-A

Candidate prepared October 3, 2026. G1 was accepted through the user's manual route/pacing report. G2 human combat acceptance remains pending. Engine: UE 5.8.3 CL 58210709. Parent project and prior G0/G1 packages preserved.

| Operation actually run | Result | Evidence |
|---|---|---|
| Editor module compilation | PASS, incremental compilation | `Logs/G2-Build.log`, Result: Succeeded |
| Eight engine automation tests | PASS: five regressions + three combat tests | `Logs/G2-Tests.log`, eight Success markers, zero Fail markers |
| Editor creation of four weapon data assets, three targets, four benches and separate map | PASS | `Logs/G2-Map.log`, zero commandlet errors; upright rotations asserted, G2_MAP_SUCCESS |
| Windows Development build/cook/stage/archive | PASS | `Logs/G2-Package.log`, BUILD SUCCESSFUL, exit 0 |
| Packaged URL-free default launch | PASS, G2 map and pawn observed, bounded NullRHI exit 0 | `Logs/G2-DefaultLaunch.log` and result file |
| Actual packaged DX12 screenshot | PASS, reviewed training view with upright targets and readable HUD | `Captures/G2-Combat.png`, `Logs/G2-Capture.log` and result file |
| Four-process combat baseline | PASS | `Logs/G2-Network-20261003-215034/Result.txt` |
| Four-process combat, configured 100 ms nominal RTT / 2% directional loss | PASS | `Logs/G2-Network-20261003-215046/Result.txt` |
| Four-process combat, configured 200 ms nominal RTT / 5% directional loss | PASS | `Logs/G2-Network-20261003-215102/Result.txt` |
| Four-process combat DX12 baseline | PASS | `Logs/G2-Network-20261003-215120/Result.txt` |

The final four network runs used the final cooked map. Each has a listen-server observer and three guests. Each guest sends actual owned combat RPCs through its pawn; authority and client logs verify three cartridge-spending shots, carbine timed reload to 30/89, shotgun magazine 5, pistol magazine 14, ammunition kit consumption, target health reduction, training downing and timed self recovery to 50 HP with one medical charge spent. Actors are explicitly repositioned into isolated floating fixtures and aim is controlled. This proves those state paths across processes, not physical input, locomotion, AI or a real combat route. Packet settings are configured emulation, not measured ping. Diagnostics perform no mission actions/save writes; the host can read an existing package campaign.

Final executable SHA256: `5514CA5D9D96EE407C479065BB3C13559C8EF1CA3AC36D349DB86CC18E1574AE`. `Logs/G2-ArtifactHashes.txt` records hashes for executable, cooked containers, map and weapon assets. Source/text toolchain remains reproducible through `Scripts/G2.ps1 Bootstrap`; build caches/stages are ignored and existing user assets are preserved.

The initial network run at `Logs/G2-Network-20261003-213551` FAILED its client completion marker because its expected death ignored per-pellet torso armor. It was corrected and rerun. Initial captures caught a wall blocking the training view and ambiguous positional editor rotations; the saved map was corrected using named pitch/yaw/roll and assertions. Initial images remain under `Captures/G2-Combat-Initial.png` and `G2-Combat-RotationIssue.png` for traceability. The final screenshot is an engine render, not gameplay input evidence.

NOT RUN: human G2 firing/aiming/feel; physical G2 input; teammate revive in packaged separate processes; hostile cross-connection ownership attempts; head/limb aiming in packaged play; marksman firing in the network harness; combat late join and disconnect cleanup; recovery packs and persistent equipment (not implemented); combat-heavy performance budgets. Synthetic revive tests cover start/interruption/charge/state transitions, but do not establish networked teammate timing or concurrent reviver behavior. No internet matchmaking, polished art/animation/audio or production-readiness claim.

Next: human combat review of the headquarters benches/targets, all four weapons, reload interruption, gadgets and two-player E revive. Resolve findings before accepting G2. G3 then adds squad perception/navigation/roles and authored encounters, validates co-op lifecycle/late join and implements unique recovery packs with persistence.
