# Operation Dead Signal — G5 mission workshop



Current starter: VF-G5-20261005-D (G5 preparation, not a complete G5 gate). User accepted the G4 E tests and authorized G5. Prior accepted G4 package/map and CampaignG4 are preserved.



## Open the editable mission in Unreal



1. Double-click **OpenMissionEditor.cmd** in the project folder. Expected: installed UE 5.8.3 opens VeilFallG0.uproject directly on the G5 workshop map, using the reduced-memory preset.

2. Open Content Drawer. Expected: Content/DeadSignal contains **L_DeadSignal_G5_Workshop**. This is a genuine editable .umap, not a packaged-only map or text scaffold.

3. Open World Outliner. Expected: **OperationDeadSignal** folders contain Infrastructure, Headquarters_Start, Residential_Edge, Relay_Outpost, Convoy, Continuity_Facility, Ancient_Chamber and Movement_Checks.

4. Select an actor in Headquarters_Start and press F in the editor viewport to frame it. Expected: the initial headquarters area is visible. Editor F framing is separate from gameplay F interaction.

5. Press Play. Expected: initial headquarters start with F interaction, Ctrl/C toggle crouch, Q/E peek and the new jump tuning. The launcher uses **G5Preview**, so each PIE/session begins with isolated preview campaign state; it does not overwrite CampaignG4 or the host's normal G5 progress.

6. Stop Play before moving or adding actors. Expected: editor returns to the authoring map.

7. Save the level after changes. Expected: L_DeadSignal_G5_Workshop shows no unsaved indicator.



The map is also the project's EditorStartupMap and GameDefaultMap. To open manually, launch VeilFallG0.uproject with UE 5.8.3, then double-click Content/DeadSignal/L_DeadSignal_G5_Workshop. Ordinary packaged G5 progress is in Saved/CampaignG5/DeadSignal.vfs; direct project launches without G5Preview may retain ordinary G5 progress.



## First mission layout and protected anchors



The compact approximately 400 x 600 m greybox route retains headquarters → residential edge → relay reclamation → convoy → concealed facility → ancient chamber → return/debrief. Nine objective actions, two optional clue nodes, four HQ starts, four established-outpost starts, six guards, six pickups, squad coordinator and authored navigation bounds remain. The briefing/debrief and supply bank are at HQ. Stable IDs and action fields connect authored anchors to persistent mission state.



Move, replace or decorate greybox geometry in the corresponding zone folder. Preserve gameplay anchors' StableId, Action, Definition and Encounter values. Keep the HQ/Relay PlayerStart tags and four collision-clear starts per group. Keep mission interaction nodes reachable within the existing 300 cm interaction range. Keep navigation volumes covering encounter geometry when reshaping relay/convoy. Run the validator before a new package; it checks IDs, required objectives/clues, starts, guard/pickup counts and the editable player Blueprint binding. It does not automatically prove every path is reachable after manual geometry edits.



Inherited road strips had pitch instead of yaw rotations. This newly created G5 map corrects the five strips to planar yaw alignment and corrects the static convoy proxy orientation. Prior maps are untouched. The one-time bootstrap repair refuses a repeat; the normal Map action preserves an existing designer map rather than regenerating it. No scheduled automation rewrites your level.



## Editable player settings



1. Open Content/G5/**BP_Player_G5**. Expected: a real Blueprint derived from native G5Character.

2. Select Class Defaults. Expected: VeilFall Cover and Jump categories.

3. Edit **Waist Cover Height** if needed. Default **90 cm**, a waist-height reference for the current approximately 181 cm benchmark. Expected: low cover around this height uses over-top peeking, with a 10 cm probe tolerance; taller cover uses side leaning. The user confirmed waist-high *cover* triggers the peek; the player rises to a normal standing eye position.

4. Edit **Jump Gravity Scale** or **Jump Speed** if needed. Defaults **1.75** and **520 cm/s**. Expected: shorter airborne time with practical jump height; native health/loadout updates preserve these settings.

5. Compile and Save. Expected: no Blueprint compile errors and persistent defaults.



Content/G5/**BP_OperationDeadSignal_G5** is the map's game mode and points to BP_Player_G5. World Settings already binds it. Gameplay logic remains C++; these Blueprints provide editable tuning/class selection without unsupported graph generation.



## Build and test



Close the interactive editor before a native build so its loaded module can be replaced. From the project folder:



    .\Scripts\G5.ps1 Build

    .\Scripts\G5.ps1 Map

    .\Scripts\G5.ps1 Validate

    .\Scripts\G5.ps1 Test

    .\Scripts\G5.ps1 Package

    .\Scripts\G5.ps1 Editor

    .\Scripts\G5.ps1 Host

    .\Scripts\G5.ps1 Join



Package refuses to overwrite the current archive. After manual edits, use `.\Scripts\G5.ps1 Package -BuildId VF-G5-Mission01`, then `.\Scripts\G5.ps1 Host -BuildId VF-G5-Mission01` to play that new archive. Choose a fresh VF-G5- name for each review build. Build/import/test use the discovered engine path C:/Program Files/Epic Games/UE_5.8. Use PlayG5.cmd for normal packaged listen-server play. See G5Evidence.md for actual results and unrun checks.



This is mission-building preparation. The route still uses greybox/proxy content, the enemy benchmark is not animation-integrated, recon sensor remains unfinished, and final art/audio/cinematics/journal polish and performance approval are not implied. Next G5 work: manually author the HQ opening and residential edge, verify reachability/co-op starts, then integrate approved art/animation/audio through the rest of the existing route.



