# G4 low-cover peek test candidate

**VF-G4-20261005-E**, UE 5.8.3 CL58210709, Windows Development. Native executable SHA256: **CA96A60BB11294FBD4C22A3A55A59D5AD2798F35A2162FB5AD26AB3E9235AA2C**. Accepted C archive remains unchanged. D is an intermediate; use E via PlayG4.cmd.

## Requested behavior

- **F:** interact, revive, collect, activate mission nodes and bank supplies. HUD and native prompts updated.
- **Ctrl or C:** toggle crouch; releasing the key keeps crouch latched. Press either again to stand.
- **Hold Q/E:** left/right peek. While crouched at low cover, a blocked chosen edge selects an over-top peek. An open chosen edge selects a side lean instead, staying crouched. Q checks the left edge; E checks the right edge.
- **Release Q/E:** return to the latched crouch stance after a top peek. Leaving cover also ends the peek. If crouch is explicitly toggled off during a peek, release respects that new standing intent.
- Tall-cover side leaning remains available. Simultaneously holding both peek directions cancels the peek. Overhead collision prevents standing. No damage reduction or invulnerability is added.

Over-top peek uncrouches the **real capsule**, raising the camera and authoritative head/body hit regions. The original crouch intent remains in CharacterMovement saved/compressed moves, so release drops back without another crouch press. The server chooses the peek using geometry; the client predicts locally. Foot-relative cover detection is stable across stance changes, including the tested 80 cm fixture. The side eye offset remains clamped to 28 cm with an obstruction sweep.

## Actual execution

| Check | Result | Evidence |
|---|---|---|
| Editor build after final changes | PASS | Logs/G4-Peek-Build.log |
| Updated G4 map/control labels imported by Unreal | PASS | Logs/G4-Import.log and G4-Peek-Import-Console.log |
| Engine fixtures | PASS, 14/14 | Logs/G4-Tests.log |
| Windows build/cook/package E | PASS | Logs/G4-Peek-Package.log, BUILD SUCCESSFUL/exit 0 |
| Separate host/client, rendered host | PASS | Logs/G4-Network-20261005-134309 |
| Separate host/client, nominal 100 ms RTT / 2% directional loss | PASS | Logs/G4-Network-20261005-134335 |
| Separate host/client, nominal 200 ms RTT / 5% directional loss | PASS | Logs/G4-Network-20261005-134401 |
| Four-player lifecycle, rendered host, 200 ms / 5% | PASS | Logs/G4-Lifecycle-20261005-134440 |

The new engine fixture validates toggle without hold, real standing exposure, preserved crouch intent, release-to-drop, stable held pop at 80 cm cover, open-right versus blocked-left edge selection, remaining crouched for a side peek, ceiling obstruction and a second toggle standing. Existing sprint, combat, ownership, interaction, inventory, mission, save/migration and AI-memory tests also pass.

The packaged helper requires **two** authoritative top-peek markers, **two** release-to-crouch markers, **two** automatic open-edge side markers and an actual client's top-peek marker, alongside the previous sprint/fatigue/lean checks. Native diagnostic flags inject positions, movement/peek intent and a replicated collision fixture. This verifies separate-process gameplay state; it does not simulate physical keyboard buttons or measure input feel. Network impairment uses PktLag=50/100 ms at each endpoint and PktLoss=2/5 per outgoing direction; achieved RTT distribution is not measured.

Four-player regression verifies concurrent pickup exclusion, revive, recovery-pack claim/bank, late join, fifth-player rejection, disconnect/reconnect and host save/restart persistence. Its legacy G3 marker names are retained. The old report phrase 'E RPC' means the interaction Request API; current key mapping is F, and a correction is appended without altering original raw logs.

Initial packaging failed after an in-flight prompt/header edit left generated header line macros stale. Failure log is retained at Logs/G4-Peek-Package-Failed.log. The completed source was rebuilt sequentially, generated headers refreshed, fourteen tests rerun and the final E package built successfully. The final cook reports five shader workers and cooker peak physical memory 2185 MiB; this is not a whole-system or GPU memory measurement.

## Manual test

1. Launch PlayG4.cmd. Expected: G4 map, HUD showing F interaction and Ctrl/C toggle crouch, Q/E peek.
2. Approach the low wall east of headquarters in the benchmark area (wall center X=3900, Y=5100, height 110 cm). Expected: the low-cover control label is visible.
3. Face the wall and press C (or Ctrl) once. Release it. Expected: player stays crouched.
4. At the middle of the wall, hold Q or E. Expected: player rises above it; release returns to crouch.
5. Approach the right-hand edge and hold E, or the left-hand edge and hold Q. Expected: side lean instead of rising; release recenters while crouched.
6. Press Ctrl or C again. Expected: player stands. Use F at the briefing terminal, equipment bench or supply bank. Expected: interaction occurs; E peeks rather than activating it.

Physical-key feel, irregular/curved cover edge tuning, human corner-fire review, animation/audio approval and target frame-time/VRAM measurements are **NOT RUN**. Prior benchmark/art limitations in G4Evidence.md still apply. Human review remains the next step; G5 has not started.

Final URL-free DX12 package launch/capture: PASS. Logs/G4-Capture.log confirms TacticalGameMode; Captures/G4-Benchmarks.png was visually reviewed and shows F interaction, Ctrl/C toggle crouch and Q/E peek. The screenshot verifies the displayed controls and benchmark presentation; it does not verify physical input feel. The prior C capture is preserved separately.
