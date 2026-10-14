# G5 mission workshop evidence - VF-G5-20261005-D

G5 preparation only. User reported G4 E manual testing passed, authorized moving toward G5 and confirmed waist-high cover triggers the over-top peek. Complete G5 art/animation/audio/journal integration remains outstanding.

## Deliverables

Actual editable `/Game/DeadSignal/L_DeadSignal_G5_Workshop`, saved mission data asset `DA_DeadSignal_G5`, player Blueprint `BP_Player_G5` and game-mode Blueprint `BP_OperationDeadSignal_G5`. World Settings selects that game mode and its editable player class. The Outliner organizes headquarters through ancient chamber and movement checks. Nine objective anchors, two optional clues, four HQ/four outpost starts, six guards and six pickups remain. Original maps, campaigns and accepted archives are preserved.

Waist-cover reference is 90 cm with a 10 cm probe clearance tolerance; the player rises to standing eye height. Taller cover uses side leaning. Jump gravity is 1.75 and jump speed is 520 cm/s, editable in Class Defaults and retained after health/loadout updates. Actual isolated engine-world flight measured 0.617 s airborne and 78.83 cm rise at 120 Hz. This does not replace human jump-feel review.

## Actual checks

- PASS native Development Editor build: G5-Build-D.log.
- PASS all 15 engine automation tests: G5-Tests.log / G5-Tests-D-Console.log. Includes real jump flight, waist/tall cover, crouch release, retained tuning, authority, ammo/reload, interaction, inventory/recovery and save migration.
- PASS actual asset creation and fresh reload: G5-Definition-Repair.log and G5-Validate.log. Nine mission descriptions and all 14 node bindings validated. Blueprint defaults load as 90 / 1.75 / 520. All five road strips have zero pitch/roll and intended yaw. This does not prove route reachability after geometry edits.
- PASS Windows Development build/cook/archive: G5-Package-D.log, exit 0. UE 5.8.3 CL 58210709. Native executable SHA256: `5AF0ECF1D4058719B250DA664A480B9014556FBF540006326DB3A8D0E7EB8874`.
- PASS final D packaged separate host/client fatigue, cover/lean, top peek, release and chosen open edge at baseline (G5-Network-20261005-174214), nominal 100 ms RTT / 2% loss (-174240), and two consecutive nominal 200 ms RTT / 5% loss runs (-174122, -174148). Host rendered at 1280x720; client NullRHI; all capped at 60 FPS. Endpoint PktLag is 0/50/100 ms, outgoing PktLoss each endpoint is 0/2/5%. Configured simulation is not measured RTT or performance approval.
- PASS final D four-player lifecycle at nominal 200 ms / 5%: G5-Lifecycle-D.log and G5-Lifecycle-20261005-174306. Pickup race, unowned fire/ammo checks, revive, death pack claim/bank, late join, capacity rejection, disconnect/reconnect and restarted-host recovery persistence ran in separate packaged processes. Exactly two recovery drops and one claim. Host rendered; guests NullRHI.

Diagnostics retain inherited G3/G4 marker names and inject fixture placement/aim and shortened bleed deadlines. They issue actual native/server requests and inspect authoritative/replicated state; physical keys and human play are NOT RUN by these scripts.

- PASS final packaged rendered HQ startup: G5-Capture.log / Docs/Captures/G5-Workshop.png. Actual G5 Blueprint game mode, fresh authoritative stage 0, correct briefing text and UI were visually inspected. A render is not gameplay evidence.

- PASS bootstrap rerun preserves the designer map: before/after SHA256 unchanged, G5-Map-Preservation-D-Result.txt.
- PASS interactive Unreal editor launched on the workshop: G5-Editor-Open.log records engine initialized and real G5 map load with zero map-check errors/warnings. Editor left running for manual authoring; no physical input test performed.

## Corrected failures retained

A missed the host top-peek marker; B separated diagnostic positions and passed all three configurations. B startup capture then exposed wrong completion text at stage 0: the duplicated mission-description asset had not been saved. C explicitly persisted it and rebound all nodes; fresh validation now checks every binding. Two C stress runs missed the host edge marker (G5-Network-20261005-173455 / -173631). D holds opt-in fixture facing throughout the sequence and logs actual edge state; two stress repetitions passed. The original C failure cause is not conclusively established as physical mouse interference. Normal controls/cover logic are unchanged by this diagnostic fix.

The final D package encountered a transient local Zen-store connection refusal after cooking; the engine recovered automatically and completed successfully. G5-Package-D.log retains it. Earlier factory/fixture errors and capture-flag failure remain in logs. A/B/C archives are preliminary; use D.

## Limits and next work

NOT RUN: human G5 jump/route feel, packaged jump correction measurement, full 1080p High four-player CPU/GPU/network/streaming percentiles and RAM/VRAM budgets, human art/audio approval and full mission integration. Original memory incident is not proven resolved; existing lower-memory launcher remains available. No paid tools, generation credits, purchases or publication used.

Next: manually author the HQ opening/residential edge, preserve stable gameplay IDs and tagged starts, then verify route reachability/co-op spawn clearance before integrating approved art, animation and audio. Exact open/build/package/play steps are in MissionBuilding.md. Archive names can be supplied with `-BuildId VF-G5-Mission01` to preserve prior builds.
