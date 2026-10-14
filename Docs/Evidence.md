# Current G0 evidence — VF-G0-20261003-B
Engine: UE 5.8.3 CL 58210709. G0 technical verification delivered; manual device/feel/visual acceptance pending. G1 NOT STARTED.
A package remains Builds/Windows; B package is Builds/VF-G0-20261003-B/Windows. Historical A report is Evidence-A.md.

| Check | Result | Evidence |
|---|---|---|
| Editor compile | PASS | Bootstrap build and Package-B.log |
| Editor compile in fresh verification directory | PASS, exit 0 | Logs/CleanEditor-B.log; Logs/CleanEditor-B-Result.txt |
| Full combined bootstrap | PASS, exit 0 | Logs/Bootstrap-B-Result.txt; Map-B.log; FoundationTests-B.log; Package-B.log |
| Four-spawn genuine map | PASS | Content/Foundation/L_G0.umap; Map-B.log success marker |
| Engine interaction + input/movement tests | PASS, 2 tests / 23 assertions | FoundationTests-B.log, both Result={Success} |
| Windows compile/cook/stage/archive | PASS, exit 0 | Package-B.log BUILD SUCCESSFUL |
| Four processes, 100 ms configured RTT / 2% loss | PASS, count 53 | Network-20261003-065649/Result.txt |
| Four processes, 200 ms configured RTT / 5% loss | PASS, count 54 | Network-20261003-065824/Result.txt |
| Four processes, DX12 baseline | PASS, count 69 | Network-20261003-065930/Result.txt |
| DX12 bounded launch + genuine screenshot | PASS, exit 0 | Capture-B.log; Captures/G0-B.png |
| Blender source/export + Unreal scale/front/up import | PASS | BenchmarkImport-Validated.log, exit 0, zero errors/warnings |
| Manual device events, viewport focus, feel and visual acceptance | NOT RUN | Review steps below |
| Disconnect/reconnect, cross-owner attack, network movement corrections | NOT RUN | TestPlan.md |
| Combat-heavy four-player performance targets | NOT RUN | Mission and combat do not yet exist |

Interaction test: valid activation, cadence, range, aim, LOS blocking/removal, unpossessed requester and null-user rejection/state preservation.
Input test: key mapping modifiers and bound EnhancedInput delegates, diagonal callback, yaw-relative movement, positive yaw/pitch and explicit sensitivity, floor settlement, actual forward displacement, jump lift/release/landing. Uses injected action values and deterministic real CharacterMovement component steps in a synthetic started game world. It does not send Windows physical input events.
Network tests require host + two initial clients to interact and all four peers to agree, including fourth-process late join. They exercise the actual owning component RPC. No future loot/mission/save duplication guarantees are inferred.
Emulation: 50/100 ms outgoing delay on every endpoint adds nominal 100/200 ms RTT over localhost baseline; 2/5% outgoing loss per direction, not combined round-trip loss. Every process confirmed settings in engine logs. No actual ping/performance guarantee. See NetworkValidation.md.
Cube imported at 100.000008 × 100.000008 × 100.000000 cm. Blender -Y marker maps to Unreal +X with +Z preserved using force_front_x_axis=True. UV/smoothing variants imported without warnings. See ImportAndExport.md.

Failures retained and corrected: test-variable PI macro collision; viewport-free controller fixture crash; missing synthetic-world actor BeginPlay/local controller designation and tick stepping. Earlier forward import pointed +Y; explicit X-front conversion corrected it. UV/tangent warnings corrected in preserved variants. These checks passed only after execution, not by inspection.
Compiler remains nonpreferred and machine memory below provisional reference. DX12 startup PSO waits exist. Benchmark launch flag was used solely to bound capture duration; no frame-time target result is inferred.
No generation credits, paid services, purchases, publishing or source-control pushes.

Manual review:
1. Run ./Scripts/G0.ps1 Host. Expected: B test room.
2. Press WASD. Expected: yaw-relative movement and diagonals.
3. Move mouse upward. Expected: view pitches upward.
4. Press/release Space. Expected: jump, release and landing.
5. Aim center at relay within 300 cm.
6. Press E. Expected: visible count increments.
7. Move outside 300 cm.
8. Press E. Expected: count unchanged.
9. Run ./Scripts/G0.ps1 Join. Expected: client receives current count.
10. Interact from either window. Expected: counts agree.
11. Record input/feel/visual acceptance before G1.

Next G1 after acceptance: HQ → residential edge → relay greybox, stable-ID mission/outpost state and host save/reload tests before extending the compact authored route.

Sensitivity follow-up — VF-G0-20261003-C: user reported B manual input PASS, with slow mouse look. Default doubled to 0.24 degrees/count. Editor compile PASS (Logs/EditorBuild-C.log); both engine tests PASS (Logs/FoundationTests-C.log). C Windows compile/cook/stage/archive PASS, exit 0 (Logs/Package-C.log); bounded DX12 launch PASS, exit 0 (Logs/Launch-C.log and Launch-C-Result.txt). This was a launch check, not a performance measurement. B network evidence above remains specific to B. Updated C feel review is pending.

User acceptance update October 3, 2026: manual input passed; C sensitivity improved but remains low. User explicitly deferred further tuning and authorized G1. Historical manual-pending rows above describe the earlier verification snapshot. Current G1 evidence is G1Evidence.md.
