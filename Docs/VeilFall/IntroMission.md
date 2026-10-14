# VEILFALL — Intro mission greybox (Phase 1)

Map: `/Game/VeilFall/World/IntroProperty/L_VF_IntroProperty`. Game mode: `BP_VF_GameMode_Intro`. The older Dead Signal maps, code and builds are untouched.

## Run it

```powershell
./Scripts/VFIntro.ps1 Build      # compile the C++ editor target
./Scripts/VFIntro.ps1 Map        # (re)generate the greybox; only VF_* actors are rebuilt, hand-placed actors are kept
./Scripts/VFIntro.ps1 Validate   # anchors, cover, dialogue beats
./Scripts/VFIntro.ps1 Autoplay   # headless run of the whole mission, NORMAL_HOME to MISSION_COMPLETE (~9 min)
./Scripts/VFIntro.ps1 Editor     # open the map in the editor (press Play)
./Scripts/VFIntro.ps1 Play -State HOLD_SHOP   # standalone window, optionally starting at a mission state
```

Controls: WASD, mouse, Space jump, **F interact**, Shift sprint, Ctrl/C crouch, Q/E lean. Once armed: LMB fire, RMB aim, R reload, 1 rifle, 3 sidearm, G medical kit.

Development: **F10** state menu, **PageDown/PageUp** jump to the next/previous state, **F9** restart from checkpoint. Console: `VFState NCD_ARRIVAL`, `VFSkip ncd|firefight|vanguard|extract|scuttle`, `VFResetFight`, `VFOpenVault`, `VFResetVault`, `VFCheckpoint`. Command-line flags: `-VFState=<STATE>` starts at a state, `-VFAutoplay` plays the mission by itself, and `-VFCapture` takes a screenshot on every state change. The vault code is `0614` (set on the mission controller; the wife says it in the scene).

## Property layout (X+ points north to the county road)

| Area | Where |
|---|---|
| Barndominium | X −700..700, Y −2400..0. Great room and kitchen (north side, windows to the drive), hallway, parents' room, two kids' rooms |
| Attached shop | X −800..800, Y 0..2200. Two bay doors and the fold-down defensive section facing the drive. Workbench, cabinets, monitors, defense panel and PA intercom |
| Vault | Behind a concealed panel in the shop's south wall, set into an earth berm. Keypad on the wall beside it, eight equipment stations inside |
| Driveway | 250 m of gravel from the shop apron to the gate (X 25000), with the culvert landmark at X 14400 |
| Approach and land | Pastures, two tree lines, a barn (west), equipment shed, trailer, propane, woodpile (east), a ridge to the north-east, fences, power poles |

## Mission states

`AVFMissionController` (Blueprint: `BP_VF_MissionController_Intro`) owns the state. Actors read it through `OnStateChanged` and `ReceiveStateEntered`. Every state can also be entered directly: `ArrangeWorld()` puts people, doors, vehicles and loadout where that state expects them, which is what checkpoints and the debug menu use.

| State | What happens | Player does |
|---|---|---|
| NORMAL_HOME | Family life, short | Talk to the wife and sons and look around. The alert fires after the wife plus two sons, or after 5 minutes |
| EMERGENCY_ALERT | Phone and TV alert, family reaction | — |
| MOVE_TO_SHOP | *Checkpoint 1.* Wife heads for the shop | Follow her |
| OPEN_VAULT | Wife gives the code | Keypad: 0614 |
| EQUIP | *Checkpoint 2.* Wife gears up and talks the player through it | Take rifle, sidearm, armor, ammo and radio (medical, bags and supplies are optional) |
| LOCALS_ARRIVE | Truck slides in with Sheriff Mercer, Mayor Vale and Nathan Cole, with blood on the door | Cover the bay door, then check the defense panel ("Are those explosives?") |
| SECURE_CHILDREN | Gate closes, wife brings the boys to the vault | Give the oldest the radio, seal the vault |
| NCD_ARRIVAL | NCD column parks at the gate | Check the camera monitors (cut to the gate camera) |
| NCD_DEMAND | *Checkpoint 3.* PA demands and the countermeasure reveal, then Nathan explains the NCD | Refuse on the intercom, then arm the road-denial system |
| DEFENSE_ARMED | The column pushes through the gate and halts at the culvert | TRIGGER |
| PROPERTY_ATTACK | Scripted driveway blasts, survivors regroup | Release the shop bay defenses |
| HOLD_SHOP | Four waves: wreck survivors, then vehicles plus the west tree line, then the east-side flank with grenades, then the armored reinforcement (*Checkpoint 4* at wave 4) | Fight from the armored port, then move to the east window for the flank |
| VANGUARD_ARRIVAL | Anti-armor hit from the ridge, two Vanguard vehicles from the west and east fields | Keep fighting |
| EXTRACT_FAMILY | "Residence secure." … "Commander." | Open the vault, family reunites |
| EVACUATE | Everyone loads up | Climb into the Vanguard vehicle |
| SCUTTLE_PROPERTY | Convoy pulls away while you face the house | F to detonate, then F to throw the trigger away |
| MISSION_COMPLETE | Fade to VEILFALL | — |

## Where things live

- C++ (`Source/VeilFallG0/VFIntro*.{h,cpp}`): mission controller, interactables, defense panels, animated props (vault door, fold-down wall, gate, bay doors), scripted damage zones, blast VFX, scripted vehicles, tree and fence lines, NPCs and their AI, grenades, the intro player, the HUD and the player controller.
- Blueprints (`Content/VeilFall/...`): `BP_VF_Player`, `BP_VF_FamilyMember`, `BP_VF_LocalNPC`, `BP_VF_NCDSoldier`, `BP_VF_VanguardSoldier`, `BP_VF_Interactable`, `BP_VF_SecurityPanel`, `BP_VF_PropertyDefensePanel`, `BP_VF_VaultDoor`, `BP_VF_ShopDefenseWall`, `BP_VF_AnimatedProp`, `BP_VF_ScriptedDamageZone`, `TEMP_BP_VF_ScriptedVehicle`, `BP_VF_GreyboxLine`, `BP_VF_MissionController_Intro`, `BP_VF_GameMode_Intro`. Tune these, not the C++ defaults.
- Dialogue: `Content/VeilFall/Data/IntroDialogue.csv`, imported to `DT_VF_IntroDialogue` (one row per line, played in order per beat).
- Generator and validator: `Content/Python/create_vf_intro.py`, `Content/Python/validate_vf_intro.py`.

## Combat tuning (prototype)

- NCD rifles do 14 per hit (player has 100 health and armor). Up to 3 NCD may shoot at the player at once (4 in wave 4). The rest suppress with wide spread. They fire in bursts, peek from cover, re-cover when hit and advance every 8–12 s.
- The wife, sheriff and Nathan hold firing ports and are essential (they recover instead of dying). The mayor stays behind the workbench.
- Grenades get an on-screen warning and a callout, and hard cover blocks them.
- Enemy spawning is per wave from markers, so counts and positions can be tuned without code.

## Known limits of this greybox

- Terrain is built from sloped boxes, not a Landscape actor. Replace it in the art pass.
- Story characters use the stock mannequin with coloured vests. There are no facial animations and no voice. Dialogue is subtitles only.
- Vehicles are box-built and follow scripted routes with no physics.
- House destruction is a scripted collapse of the greybox pieces plus fires. There's no Chaos fracture yet.
- The wife and sons are unnamed (subtitles say "Wife", "Oldest son"). Names are a story decision.
