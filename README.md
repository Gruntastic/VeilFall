# VEILFALL — G3 AI/co-op review candidate

Build VF-G3-20261003-A uses UE 5.8.3 CL 58210709. Mouse sensitivity is 0.36 degrees/count (+50%). The user's G2 review authorized G3. This candidate adds three squad roles, server perception/navigation/Behavior Trees/EQS, field supplies, durable recovery packs/banking and co-op lifecycle validation. Presentation remains a primitive greybox.

From this project directory:
```powershell
./Scripts/G3.ps1 Bootstrap
./Scripts/G3.ps1 Host
./Scripts/G3.ps1 Join
./Scripts/G3.ps1 Editor
./Scripts/G3.ps1 Test
./Scripts/G3AISmoke.ps1
./Scripts/G3NetworkSmoke.ps1 -RttMs 100 -LossPercent 2
```

Executable: `Builds/VF-G3-20261003-A/Windows/VeilFallG0.exe`. Current source default: `/Game/DeadSignal/L_DeadSignal_G3`. Prior G0/G1/G2 maps/packages are preserved. WASD/mouse/Space; E interact/revive/loot/bank; left/right mouse fire/aim; R reload; 1/2/3 weapon; G medical, H ammo; Enter respawn. Kill the three guards before reclaiming the relay or recovering convoy evidence. Supply bank is at HQ.

Play/build/save details: [Docs/G3.md](Docs/G3.md). Actual evidence: [Docs/G3Evidence.md](Docs/G3Evidence.md). Limitations: [Docs/KnownIssues.md](Docs/KnownIssues.md). Exact environment: [Docs/Environment.md](Docs/Environment.md). Recon sensor, final art/audio, human G3 review and performance budgets remain outstanding; this is not a finished game.

Historical evidence is preserved in Docs/Evidence.md, G1Evidence.md and G2Evidence.md. Current Test entry points run the full eleven-test suite. G1 route and pacing were accepted manually; latest user "Looks good" explicitly authorized G3 progression without adding undocumented test coverage.

October 5 memory preflight: PlayG3.cmd and G3.ps1 launch a reversible lower-memory testing preset. See Docs/MemoryOptimization.md for actual measurements and normal-quality launch commands. Peak RAM was essentially unchanged; end memory commitment improved modestly. The reported original memory issue is not yet reproduced or declared resolved.

G4 review candidate: use PlayG4.cmd. Shift sprint; Ctrl crouch; Q/F lean near blocking cover. E remains interaction. See Docs/G4.md for tuning/build/import commands and Docs/G4Evidence.md for actual runs. G3 package remains preserved. Benchmark art and synthetic breathing loop need human review.

G4 current test candidate is VF-G4-20261005-E: F interacts; Ctrl/C toggle crouch; Q/E hold to peek. At low cover, holding peeks above the wall unless the chosen side has an open edge, where it leans sideways; release returns to crouch. Use PlayG4.cmd. See Docs/G4PeekEvidence.md. Accepted C archive remains available unchanged.

G5 manual-authoring entry: double-click OpenMissionEditor.cmd to open the genuine editable G5 map in UE 5.8.3. Project EditorStartupMap now points to L_DeadSignal_G5_Workshop. Use OperationDeadSignal Outliner folders for the HQ opening/mission areas; player tuning is Content/G5/BP_Player_G5. Instructions: Docs/MissionBuilding.md. PlayG5.cmd launches the separate packaged starter; accepted G4 E remains preserved.

Developer test range: double-click PlayTestRange.cmd (OpenTestRangeEditor.cmd to edit) for a flat map with cover at 60/90/110/120/150/220 cm, a training-target lane and an NCD enemy yard. The player starts armed and cannot die; enemies and targets come back after dying. Cover: Ctrl/C crouch at a piece, hold RMB to pop up over it, release to drop back. Scripts/VFTestRange.ps1 Map regenerates the map; CoverTest runs a headless cover self-test.
