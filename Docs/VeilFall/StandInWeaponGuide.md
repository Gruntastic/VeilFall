# VEILFALL — stand-in weapon guide (intro prototype)

Stand-in weapons are simple, correctly sized and pivoted meshes that replace today's box carbine. They have to work in first person now and support the optic/ADS pass later, and they should drop straight onto the UE5 mannequin (Manny/Quinn) and the animations already in the project. Fictional, generic shapes only. No real manufacturer markings or trademarked designs.

## What's needed for the intro

| Asset name | Used by | Rough size (overall length) | Notes |
|---|---|---|---|
| `TEMP_Rifle_Player` | Hale (first person) and world copy | 85–90 cm | Carbine silhouette. Optic rail on top, removable optic as a separate mesh |
| `TEMP_Optic_Red_Dot` | Player rifle | 8–10 cm | Separate mesh so the ADS pass can swap optics |
| `TEMP_Sidearm_Player` | Hale, slot 3 | 18–20 cm | |
| `TEMP_Rifle_NCD` | NCD soldiers | 85–90 cm | Clean, uniform, dark. All NCD carry the same model |
| `TEMP_Rifle_Vanguard_A`, `_B` | Vanguard | 75–95 cm | Two or three variants: different stock, optic and colour (mixed, practical kit) |
| `TEMP_Rifle_Wife` | Wife | 85–90 cm | Can be `TEMP_Rifle_Player` with a different finish |
| `TEMP_Shotgun_Sheriff` (optional) | Sheriff | 95–105 cm | Only if the sheriff should look different from the others |
| `TEMP_Radio_Handheld` | Vault station, oldest son | 15–20 cm | Prop, held in one hand |
| `TEMP_Remote_Trigger` | Scuttle beat | 10–12 cm | Small generic handheld box with a guarded switch, purely a prop |
| `TEMP_Grenade_NCD` | NCD grenade throw | 9–12 cm | Generic shape |

There's no anti-armor weapon model. The Vanguard's flank shot is sound and impact only, as the brief says.

## Mesh rules (all weapons)

1. **Units:** centimetres, real-world scale. Import scale 1.0.
2. **Orientation:** barrel points along **+X**, top of the weapon is **+Z**, and the right side faces **+Y**. That's what the mannequin's `HandGrip_R` socket and the first-person camera attachment expect.
3. **Pivot:** at the **pistol grip**, where the right hand's palm sits, not the centre of the mesh. Then attaching to `HandGrip_R` needs no offset.
4. **Sockets** (add them in the Static Mesh editor or with `SOCKET_` empties in Blender):
   - `Muzzle`: tip of the barrel, +X forward. Used for muzzle flash, shot origin and tracers.
   - `Grip_L`: where the left hand holds the handguard. Used by left-hand IK.
   - `Sight`: the eye point behind the optic, on the bore axis, +X forward. The optic/ADS view aligns the camera here.
   - `Mag`: centre of the magazine well. Used for reload props later.
   - `Eject`: ejection port. Used for brass and smoke.
5. **Parts:** keep the magazine as its own mesh (`TEMP_Rifle_Player_Mag`) so reloads can animate it. The optic is separate too. Everything else can be one mesh.
6. **Collision:** none on the weapon. Hit detection uses the characters' hitboxes.
7. **Budget:** first-person rifle under about 15k triangles and one material. Third-person copies under about 5k. Generate a couple of LODs on import.
8. **Materials:** one material per weapon, using `M_VF_Greybox` instances (`TEMP_MI_*`) until real textures exist. The NCD finish is near-black. Vanguard is tan or olive.
9. **Naming:** `TEMP_` prefix while placeholder, in `Content/VeilFall/Weapons/`. Log every source in `Docs/VeilFall/AssetLog.md` (made in-house in Blender, or a Fab/free listing with its license).

## Blender export (FBX)

- Scene units: Metric, unit scale 0.01, length in centimetres. Model the barrel along +X.
- Export: *Selected Objects*, *Apply Transform*, Forward **X**, Up **Z**, no armature.
- Name socket empties `SOCKET_Muzzle`, `SOCKET_Grip_L` and so on. UE turns them into sockets on import.

## How they'll be hooked up

- Each weapon gets a `UVeilWeaponDefinition` data asset (the existing combat system already uses `DA_carbine`, `DA_pistol` and others). It holds the mesh, plus the damage and magazine numbers it already has. Today the meshes are hard-coded: `SM_Carbine` for NPCs and a box for the player.
- Third person: the weapon attaches to `HandGrip_R` on the mannequin (already done for NPCs).
- First person: the weapon attaches to the first-person arms' right-hand socket. `Sight` drives the ADS camera position in the optic pass.
- The muzzle flash and the shot sound play at `Muzzle`.

## Acceptance check for each stand-in

Drop it into `L_VF_IntroProperty` and run `PlayIntro.cmd HOLD_SHOP`. It should:
- sit in the hand without floating or clipping in idle, walk and aim,
- point the muzzle where the crosshair is in first person,
- have the `Sight` socket centred on the screen when aimed.
