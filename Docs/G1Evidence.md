# G1 review evidence — VF-G1-20261003-A
UE 5.8.3 CL 58210709. G0 advance authorized by user on October 3, 2026 following manual input pass; further sensitivity tuning deferred. G1 accepted October 3, 2026: user reported manual testing and pacing good, with no errors observed. This is user-reported acceptance, not an additional automated test. G2 NOT STARTED.

| Check | Result | Evidence |
|---|---|---|
| Final combined bootstrap | PASS, exit 0 | Logs/G1-Bootstrap-Result.txt; actual Build/Map/Test/Package execution |
| Editor and Windows native compilation | PASS | Final bootstrap; Logs/G1-Package.log, successful UBT/UAT results |
| Genuine G1 map and mission data asset | PASS, 80 actors / 14 unique nodes / 9 objectives | Logs/G1-Map.log; Content/DeadSignal/L_DeadSignal_G1.umap; DA_DeadSignal.uasset |
| Engine automation | PASS, five tests | Logs/G1-Tests.log, five Result={Success} |
| Windows compile/cook/stage/archive | PASS, exit 0 | Logs/G1-Package.log, BUILD SUCCESSFUL |
| Four-process mission, nominal 100 ms RTT / 2% outgoing loss per direction | PASS | Logs/G1-Network-20261003-193750/Result.txt and peer logs |
| Four-process mission, nominal 200 ms RTT / 5% outgoing loss per direction | PASS | Logs/G1-Network-20261003-193811/Result.txt and peer logs |
| Four-process DX12 baseline | PASS | Logs/G1-Network-20261003-193831/Result.txt and peer logs |
| URL-free default map launch | PASS, exit 0, NullRHI | Logs/G1-DefaultLaunch.log; G1-DefaultLaunch-Result.txt |
| Warmed-up DX12 UI captures | PASS, both process exits 0; inspected | Captures/G1-HQ.png; Captures/G1-Chamber.png; Logs/G1-Capture-*-Result.txt |
| Source/assets/executable/capture hashes | Recorded | Logs/G1-ArtifactHashes.txt |
| Manual testing and pacing | PASS, user-reported | October 3, 2026: "Manual testing and pacing are good. No errors observed" |
| Combat-heavy performance target measurements | NOT RUN | Combat/AI not implemented |
| Server timeout/logout cleanup of abruptly stopped guest | NOT ASSERTED | Replacement guest receives host state; full lifecycle tests remain G3 |
| Actual power-loss fault injection / POSIX save implementation | NOT RUN | Windows file operations executed; not hardware durability certification |

Tests executed: Foundation.AuthoritativeInteraction; Foundation.InputMovement; Campaign.OrderedState; Campaign.AtomicSaveRecovery; Campaign.AuthorityAndOutpost. Ordered state covers full route, skipping/unknown actions, repeated actions/clues, outpost invariants, banking once and schema-zero struct migration. Filesystem tests actually serialize/write/read versioned saves, replace the primary, preserve prior backup, ignore uncommitted temporary files, detect checksum corruption, recover backup, reject dual corruption and protect a valid future-schema primary from rollback/overwrite. No released legacy save corpus exists to validate migration against.

Network diagnostics: host deliberately never activates checkpoints; remote client(s) use the real owning interaction RPC. Exactly 11 actions are accepted: nine ordered mission actions and two optional clues. All peers receive stage 9, established relay, clue mask 3 and one banked evidence record. The fourth process late-joins; one guest is stopped and a replacement joins; the host is stopped/restarted against its isolated save; a new guest receives restored state. Restored host is verified to spawn at RelayEstablished. Server teleports participants to collision-free approaches and supplies diagnostic aiming. This does not prove normal traversal, OS input, malicious networking resistance or mission duration.

Emulation uses outgoing PktLag=50/100 ms on every endpoint for nominal added RTT of 100/200 ms over localhost baseline, and PktLoss=2/5 percent per outgoing direction. Settings are confirmed in the initial host/client/late/replacement logs. No measured ping guarantee, jitter, duplication or reordering profile is claimed.

Scope: full bounded greybox route; authored checkpoint/optional-clue interactions; persistent outpost residents/radio/spawn changes; availability-only resupply marker; temporary native UMG; host mission-evidence banking. Combat clearing and convoy encounter use explicit interaction stand-ins. Weapons, armor, AI, equipment inventory, down/revive and recovery packs are not implemented. Other player bodies are not yet visible. This is not the final polished 20–30 minute mission.

Corrected failures retained: initial WorldSettings discovery failed (Logs/G1-Map-FailedWorldSettings.log); initial network diagnostic stalled at archive because its fixed placement intersected a wall (Logs/G1-Network-20261003-191622). Collision-tested approach selection fixed it. First captures are retained as *-Initial.png; dynamic lighting removed the unbuilt-light warning, and the final capture tool waits two seconds for labels/rendering. Near-terminal world labels can clip; HUD remains the primary greybox objective guide. Layout tuning remains part of manual G1 review.

Installed compiler remains newer than Epic's preferred version; engine-header deprecation warnings remain. Machine RAM is below the provisional reference. No frame-time, VRAM, process-memory or bandwidth targets are claimed. No purchases, paid generation, publishing, external service setup, remote locks or pushes.

Launch from this project directory: ./Scripts/G1.ps1 Host; ./Scripts/G1.ps1 Join. Close previous host windows first. Build again: ./Scripts/G1.ps1 Bootstrap. Detailed play/save/route instructions: G1.md.
Next gate: G2 authoritative hitscan/reload/ammo, armor feedback and down/revive. Sensitivity remains deferred at 0.24 degrees/count.