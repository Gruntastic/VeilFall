# G4 execution evidence

Final review candidate: **VF-G4-20261005-C**, Windows Development, UE **5.8.3 CL58210709**, Blender **5.2.2 LTS**. Native executable SHA256: **2FC1E68A054A6CB4C1A05E2A539130F7EF1CB6B1882BDE72F10F5E716FE2CC09**. A/B are preliminary and superseded. G3 native executable hash still matches **5971148081DFD4E70D0A6AA8BA8859EF266DD675F8A54C641268F688A26B084F**; its map/package/save folder were preserved.

| Operation actually run | Result | Evidence |
|---|---|---|
| Native editor compile | PASS | Logs/G4-Build.log; actual installed MSVC/SDK, nonpreferred toolchain warning retained |
| Original Blender generation/export | PASS | Logs/G4-Blender.log; four real .blend/FBX pairs and Manifest.json |
| Existing-source Blender export into new revision | PASS | Logs/G4-Existing-Blender-Export.log; ArtSource/G4/ValidationExport/Carbine.fbx |
| Real mesh/material/sound/map import | PASS after repair | Logs/G4-Import.log; real Content/G4 and L_DeadSignal_G4 assets |
| Fresh-process skeletal dependency reload | PASS | G4_RIG_IMPORTED skeleton=/Game/G4/Meshes/Enemy_Rig_Skeleton; initial persistence failure retained separately |
| Engine fixtures | PASS, 13/13 | Logs/G4-Tests.log: eleven previous tests plus SprintFatigue and CoverEyeCollision |
| Final Windows build/full cook/stage/archive | PASS | Logs/G4-Package.log: BUILD SUCCESSFUL, exit 0; five shader workers |
| URL-free Windows DX12 launch/default map | PASS | Logs/G4-Capture.log: L_DeadSignal_G4, TacticalGameMode; real capture reviewed |
| Two-process sprint/fatigue/crouch/lean, rendered host | PASS | Logs/G4-Network-20261005-083537/Result.txt |
| Two-process sprint/cover, 100 ms nominal RTT / 2% loss | PASS | Logs/G4-Network-20261005-083556/Result.txt |
| Two-process sprint/cover, 200 ms nominal RTT / 5% loss | PASS | Logs/G4-Network-20261005-083614/Result.txt |
| Four-player lifecycle regression, rendered host, 200 ms / 5% | PASS | Logs/G4-Lifecycle-20261005-083634/Result.txt; separate processes, guest late join and restored host |

The sprint fixture logs three qualifying bursts, server speed 739.5 cm/s for medium armor and replicated fatigue on the client. Cover logs server crouch/lean and eye offset 28 cm. Engine cover tests independently sweep eye collision against a side obstruction and reject lean away from nearby cover. Existing combat tests verify blocked muzzle damage, cadence, aim plausibility and ammo/ownership. These are actual runtime checks; not manual corner-shooting or animation evaluation.

Lifecycle regression retains G3-labelled native markers because it exercises the same claims/revive/persistence diagnostics in the final G4 package. It verifies real E interaction RPC claims, concurrent pickup exclusion, timed teammate revive, death pack claim/bank, duplicate exclusion, authoritative ammo ownership, late join recovery state, fifth-player capacity rejection, disconnect drop/reconnect and host restart restoration. Exactly two pack drops and one pack claim. Diagnostic movement/positions/aim are injected and bleed shortened; no physical input claim. G4 diagnostic/capture saves are isolated with G1SaveRoot. Ordinary G4 saves use CampaignG4. Reliable diagnostic acknowledgments now send once per stage, preventing the prior uncapped helper flood.

Impairment uses PktLag=50 ms at each endpoint for 100 ms nominal RTT, or 100 ms at each endpoint for 200 ms, with PktLoss=2/5 per outgoing direction. These are configured targets, not measured RTT distributions. Every helper process is capped at 60 FPS. All tests are development LAN/direct-IP; no internet matchmaking or dedicated-server deployment claim.

## Memory and presentation limits

Final cooker reports PeakPhysMemoryMB=2189, PeakVirtMemoryMB=2034 for the **cooker process**, not aggregate compiler/UAT/system memory. Five workers are evidenced after explicit thread reservation; percentage-only reservation was skipped by commandlets in the installed engine. Two-process rendered sprint test aggregate sampled peak working set 1321.91 MiB/private commitment 1836.58 MiB. Four-player lifecycle peak aggregate 2268.41 MiB/2405.04 MiB, including transient fifth-player capacity probe; rendered host with headless guests. NullRHI guest numbers cannot represent four rendered players or GPU memory.

The launch preset retains prior Medium/85% rendering and a 768 MiB texture-pool budget. It is a test preset, not a 1080p High performance measurement. No original memory-failure reproduction or GPU-byte measurement; no claim that the memory issue is solved. Prior before/after probes are documented in MemoryOptimization.md.

Captures/G4-Benchmarks-C.png is an actual packaged gameplay screenshot. Visual review confirms imported carbine, wall and block enemy plus HUD; art is sparse, proxy vehicle/tree geometry and older greybox structures remain. It is not evidence of gameplay feel, polished animation or AAA presentation. Initial rig import had an unsaved transient skeleton; fresh reload exposed it. The importer now saves the dependency explicitly and reload verified it. The enemy has a basic rigid-weight source rig, no animation clips, physics/ragdoll setup or deformation quality approval.

**NOT RUN:** human physical-input sprint/cover review; listening/mix/quality approval of the original synthetic breathing loop; full G4 sprint/lean late-join snapshot; corner-fire/manual low-cover scenarios; client correction smoothness measurements; full route with four rendered players; 1080p High p95/p99, VRAM, network bandwidth and warm-up hitch profiling; final UV/material/collision/art approval. Existing G3 quest/loot late-join tests do not imply these extra checks.

G4 is a review candidate, **not a closed art gate or production-ready slice**. Review the benchmark silhouettes, first-person carbine, sprint cadence, breathing and free crouch/lean cover before batching art. G5 integration has not started.

