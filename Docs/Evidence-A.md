# G0 evidence — VF-G0-20261003-A
Status: compiled and packaged foundation delivered; G0 acceptance pending manual input/feel review. G1 NOT STARTED.
Engine 5.8.3 CL 58210709. Fresh project preserves the existing parent project.

| Check | Result | Evidence |
|---|---|---|
| Environment/toolchain audit | PASS with warnings | Environment.md; Logs/EditorBuild.log; Logs/Package.log |
| Editor native build | PASS | Editor DLL linked; successful UBT build; Logs/Package.log includes current editor target |
| Genuine map creation | PASS | Content/Foundation/L_G0.umap; Logs/Map.log G0_MAP_SUCCESS |
| Idempotent map rerun | PASS | Logs/Map-Rerun.log, success marker and exit 0 |
| In-engine interaction test | PASS, 1 test / 9 assertions | Logs/InteractionTests-backup-2026.10.03-11.25.49.log Result={Success}, process exit 0 |
| Windows compile/cook/stage/archive | PASS | Logs/Package.log BUILD SUCCESSFUL, exit 0 |
| Packaged separate processes, NullRHI | PASS | Logs/Network-20261003-063126/Result.txt; host and client interacted, all three processes count 39 |
| Packaged separate processes, DX12 | PASS | Logs/Network-20261003-063207/Result.txt; all three processes count 42 |
| DX12 screenshot and bounded launch | PASS | Captures/G0.png; Logs/Capture.log; game process exit 0 |
| Blender source/export benchmark | PASS | ScaleCube.blend + .fbx; 1 m assertion; Logs/BlenderExport.txt |
| PowerShell syntax | PASS | AST parse completed without errors |
| Combined Bootstrap.ps1 invocation | NOT RUN | Individual Build/Map/Test/Package actions executed |
| Manual WASD/mouse/jump/diagonal controls and feel | NOT RUN | Native input bindings exist; diagnostic RPC exercise does not prove keyboard input |
| Manual visual acceptance | NOT RUN | Capture inspected: placeholder floor, relay prompt, target, blocker and sky visible |
| Impaired-network, 4-player, disconnect/reconnect | NOT RUN | Plan in TestPlan.md |
| Cross-connection malicious RPC ownership test | NOT RUN | Conventional engine ownership used; unpossessed request tested |
| Performance target measurements | NOT RUN | No benchmark result inferred from capture -benchmark argument |
| Blender→Unreal scale/orientation import | NOT RUN | Exact steps in ImportAndExport.md |

Initial failures retained: replication macro compile parameter corrected; malformed source newline corrected; camera-cache aim caused valid interaction failure (Logs/InteractionTests-Failed.log), corrected to pawn eye/control rotation and rerun PASS. Map wrapper argument check fixed. Package staging failed deleting a OneDrive directory (Logs/Package-FailedStage.log); fresh local AppData stage succeeded. Launch paths corrected to actual archive layout.

The test asserts valid activation, immediate repeat rejection, range rejection, wrong aim rejection, occlusion rejection, acceptance after blocker removal, unpossessed rejection, null-user rejection and rejected-state preservation. It uses an in-engine game world; it is distinct from the packaged replication tests.

Packaged DX12 startup logs include PSO creation waits during startup; no warmup performance pass is claimed. Machine has 16 GB RAM and a nonpreferred compiler.

Review:
1. From G0/VeilFallG0 run ./Scripts/G0.ps1 Host. Expected: Windows test room.
2. Use WASD. Expected: walking relative to yaw, including diagonals.
3. Move mouse. Expected: horizontal look and upward motion raises view.
4. Press Space. Expected: jump and floor landing.
5. Aim center at relay cube within 300 cm.
6. Press E. Expected: visible relay counter increments.
7. Move beyond 300 cm.
8. Press E while aiming at relay. Expected: no increment.
9. Run ./Scripts/G0.ps1 Join in another terminal. Expected: client joins same map and receives current count.
10. Activate relay from either player. Expected: both counters agree.
11. Record feel/visual feedback before G0 signoff.

Next G1 work after acceptance: greybox only the HQ → residential edge → relay route segment, add stable-ID objective/outpost state, then host save/reload tests before extending the full 400×600 m authored route. No art batching or cinematics.
