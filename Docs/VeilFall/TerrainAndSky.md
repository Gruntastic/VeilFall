# VEILFALL intro — real terrain and sky

## Sky (done)

`create_vf_intro.py` now spawns a **Volumetric Cloud** layer. The sun drives the atmosphere and casts cloud shadows, alongside the existing Sky Atmosphere, real-time Sky Light and height fog. To try a different time of day, rotate `VF_Sun`. Pitch −38 is late morning, and lower angles give warmer light and longer shadows.

## Terrain (ready to import, one manual step)

Unreal's Python API can't create a Landscape actor, so creating it is a short step in the editor. Everything else is prepared:

| File | What it is |
|---|---|
| `ArtSource/VeilFall/Terrain/HM_VF_IntroProperty_1009.png` | 16-bit heightmap, 1009 × 1009, 1 m per vertex (1 km² centred on the property) |
| `ArtSource/VeilFall/Terrain/WM_VF_{Pasture,Gravel,Dirt,Asphalt}_1009.png` | Paint masks: the driveway, county road, yard/bypass dirt and both pastures. Grass is the base layer |
| `Content/VeilFall/Materials/M_VF_Landscape` | Landscape material with weight-blended layers Grass, Pasture, Gravel, Dirt, Asphalt (flat colours for now) |
| `Scripts/VeilFallTerrain/make_terrain_maps.py` | Regenerates the maps (plain Python, no packages) if the layout changes |

The heightmap keeps the house, shop, apron, driveway, county road and gate at exactly **Z = 0**, so every building, marker and AI point stays where it is. It reproduces the greybox hills (north-east ridge, west rise, gate hill, vault berm) and adds gentle rolling farmland that climbs toward the horizon outside the fences (about −12 m to +31 m overall).

### Import steps (≈2 minutes)

1. Open the map (`OpenIntroEditor.cmd`) and switch to **Landscape** mode (Shift+2), then go to **Manage › New**.
2. Choose **Import from File** and pick `HM_VF_IntroProperty_1009.png`.
3. Material: `M_VF_Landscape`. Click the **+** next to each of the five layers to create its Layer Info (weight-blended). Save them in `Content/VeilFall/World/IntroProperty/Landscape`.
4. For each layer except Grass, set its import file to the matching `WM_VF_<Layer>_1009.png`.
5. Section size **63×63 quads**, **2×2** sections per component, giving 8×8 components (the dialog should show 1009 × 1009).
6. **Location** X −39400, Y −50400, Z 0. **Scale** X 100, Y 100, Z 100. Click **Import**, then save the map.
7. Run `Scripts/VFIntro.ps1 Map`. The builder sees the Landscape, removes the greybox ground slab, pasture patches and box hills, and re-seats tree lines and fences on the terrain. Hand-placed actors and the Landscape are kept.

## Next improvements, in the order I'd do them

1. **Ground surfaces:** swap the flat layer colours in `M_VF_Landscape` for Megascans/Fab grass, gravel, dirt and asphalt textures. Add a Landscape Grass Type on the Grass and Pasture layers for real grass and weeds.
2. **Trees:** replace the sphere-on-cylinder trees with real tree meshes (free Fab/Megascans or the UE Electric Dreams/forest samples). `AVFGreyboxLine` already scatters them along lines, so it's a mesh swap.
3. **Lighting and exposure:** add a post-process volume with manual exposure tuned for both outdoors and the darker interiors. The project currently has auto-exposure off.
4. **Buildings:** a modular metal-building kit for the barndominium and shop, keeping today's footprint, door widths and firing ports.
5. **Ambient life:** wind in the trees, birds and insects before the alert, quieter after it, plus distant sirens.

Log every asset you bring in in `Docs/VeilFall/AssetLog.md` with its source and license.
