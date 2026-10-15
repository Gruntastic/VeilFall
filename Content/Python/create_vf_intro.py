"""VEILFALL intro greybox builder: /Game/VeilFall/World/IntroProperty/L_VF_IntroProperty.

Re-runnable. Every generated actor is labelled VF_*; on re-run those are rebuilt and anything else a designer
placed in the map is left alone. Placeholder assets are engine primitives plus project-generated TEMP_ assets
(see Docs/VeilFall/AssetLog.md). Run: Scripts/VFIntro.ps1 Map
"""
import unreal, math, random, wave, struct
from pathlib import Path

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
ROOT = Path(unreal.Paths.project_dir())
MAP = '/Game/VeilFall/World/IntroProperty/L_VF_IntroProperty'
V = unreal.Vector

# ------------------------------------------------------------------ folders
for f in ['Blueprints', 'Characters/Player', 'Characters/NPC', 'Characters/NCD', 'Characters/Vanguard', 'Characters/Family',
          'Interactions', 'Weapons', 'World/IntroProperty/House', 'World/IntroProperty/Shop', 'World/IntroProperty/Vault',
          'World/IntroProperty/Landscape', 'Missions/IntroMission', 'UI', 'Audio', 'VFX', 'Materials', 'Data', 'Dev']:
    assets.make_directory('/Game/VeilFall/' + f)

# ------------------------------------------------------------------ TEMP audio (synthesised, project-original)
def synth(name, seconds, fn, rate=22050):
    out = ROOT / 'ArtSource' / 'VeilFall' / 'Audio' / (name + '.wav')
    if not out.exists():
        out.parent.mkdir(parents=True, exist_ok=True)
        rnd = random.Random(name)
        with wave.open(str(out), 'wb') as w:
            w.setnchannels(1); w.setsampwidth(2); w.setframerate(rate)
            frames = bytearray(); state = {'lp': 0.0, 'lp2': 0.0}
            for i in range(int(seconds * rate)):
                v = max(-1.0, min(1.0, fn(i / rate, rnd, state)))
                frames += struct.pack('<h', int(v * 30000))
            w.writeframes(bytes(frames))
    dest = '/Game/VeilFall/Audio/' + name
    if not assets.does_asset_exist(dest):
        t = unreal.AssetImportTask(); t.filename = str(out); t.destination_path = '/Game/VeilFall/Audio'; t.destination_name = name; t.automated = True; t.save = True
        tools.import_asset_tasks([t])
    return assets.load_asset(dest)

def tone(t, r, s):  # two-tone attention signal, pulsed
    on = (t % 1.0) < 0.8
    return 0.45 * on * (math.sin(2 * math.pi * 853 * t) + math.sin(2 * math.pi * 960 * t)) * min(1, t * 20)
def gunshot(t, r, s):
    s['lp'] += (r.uniform(-1, 1) - s['lp']) * 0.35
    return s['lp'] * 2.2 * math.exp(-t * 18) + 0.5 * math.sin(2 * math.pi * 70 * t) * math.exp(-t * 25)
def explosion(t, r, s):
    s['lp'] += (r.uniform(-1, 1) - s['lp']) * 0.08; s['lp2'] += (s['lp'] - s['lp2']) * 0.2
    return s['lp2'] * 6 * math.exp(-t * 1.6) * min(1, t * 200)
def engine(t, r, s):
    s['lp'] += (r.uniform(-1, 1) - s['lp']) * 0.05
    f = 38 + 3 * math.sin(2 * math.pi * 0.5 * t)
    return 0.35 * math.sin(2 * math.pi * f * t) + 0.2 * math.sin(2 * math.pi * 2 * f * t) + s['lp'] * 0.8
def pa(t, r, s):  # PA chime/feedback before announcements
    return 0.4 * math.sin(2 * math.pi * (660 if t < 0.35 else 520) * t) * math.exp(-((t % 0.35) * 4))

synth('TEMP_SFX_EmergencyTone', 2.4, tone)
synth('TEMP_SFX_Gunshot', 0.45, gunshot)
synth('TEMP_SFX_Explosion', 2.6, explosion)
eng = synth('TEMP_SFX_EngineLoop', 2.0, engine)
eng.set_editor_property('looping', True); assets.save_loaded_asset(eng)
synth('TEMP_SFX_PAChime', 0.7, pa)

# ------------------------------------------------------------------ materials
MEL = unreal.MaterialEditingLibrary
def base_material(name, kind):
    p = '/Game/VeilFall/Materials/' + name
    m = assets.load_asset(p) if assets.does_asset_exist(p) else tools.create_asset(name, '/Game/VeilFall/Materials', unreal.Material, unreal.MaterialFactoryNew())
    if MEL.get_num_material_expressions(m) == 0:
        c = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -400, 0)
        c.set_editor_property('parameter_name', 'Color'); c.set_editor_property('default_value', unreal.LinearColor(0.5, 0.5, 0.5, 1))
        if kind == 'lit':
            MEL.connect_material_property(c, '', unreal.MaterialProperty.MP_BASE_COLOR)
            r = MEL.create_material_expression(m, unreal.MaterialExpressionConstant, -400, 200); r.set_editor_property('r', 0.85)
            MEL.connect_material_property(r, '', unreal.MaterialProperty.MP_ROUGHNESS)
        else:
            m.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
            MEL.connect_material_property(c, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
            if kind == 'smoke':
                m.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
                o = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -400, 200)
                o.set_editor_property('parameter_name', 'Opacity'); o.set_editor_property('default_value', 0.6)
                MEL.connect_material_property(o, '', unreal.MaterialProperty.MP_OPACITY)
    for flag in ['used_with_instanced_static_meshes', 'used_with_skeletal_mesh']:
        try: m.set_editor_property(flag, True)
        except Exception: pass
    MEL.recompile_material(m); assets.save_loaded_asset(m)
    return m

GREY = base_material('M_VF_Greybox', 'lit')
base_material('M_VF_Emissive', 'emissive')
base_material('M_VF_Smoke', 'smoke')

PALETTE = {
    'Grass': (0.10, 0.17, 0.06), 'Pasture': (0.16, 0.20, 0.08), 'Gravel': (0.30, 0.28, 0.25), 'Asphalt': (0.045, 0.045, 0.05),
    'Concrete': (0.40, 0.39, 0.37), 'BarndoSiding': (0.10, 0.105, 0.11), 'BarndoTrim': (0.30, 0.20, 0.12), 'InteriorWall': (0.70, 0.67, 0.61),
    'WoodFloor': (0.30, 0.19, 0.11), 'Roof': (0.07, 0.07, 0.08), 'ShopInterior': (0.42, 0.44, 0.46), 'Steel': (0.20, 0.22, 0.24),
    'Vault': (0.27, 0.29, 0.31), 'Furniture': (0.33, 0.23, 0.15), 'Fabric': (0.22, 0.27, 0.36), 'Bedding': (0.55, 0.55, 0.6),
    'Hay': (0.52, 0.42, 0.18), 'Dirt': (0.22, 0.16, 0.10), 'Rock': (0.28, 0.28, 0.27), 'BarnRed': (0.32, 0.07, 0.05),
    'Utility': (0.35, 0.35, 0.33), 'Pole': (0.20, 0.15, 0.10), 'Appliance': (0.75, 0.75, 0.73), 'Dark': (0.03, 0.03, 0.035),
}
MI = {}
for name, c in PALETTE.items():
    p = '/Game/VeilFall/Materials/TEMP_MI_' + name
    mi = assets.load_asset(p) if assets.does_asset_exist(p) else tools.create_asset('TEMP_MI_' + name, '/Game/VeilFall/Materials', unreal.MaterialInstanceConstant, unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property('parent', GREY)
    MEL.set_material_instance_vector_parameter_value(mi, 'Color', unreal.LinearColor(c[0], c[1], c[2], 1))
    MEL.update_material_instance(mi)
    assets.save_loaded_asset(mi); MI[name] = mi

def color(name): c = PALETTE[name]; return unreal.LinearColor(c[0], c[1], c[2], 1)

# ------------------------------------------------------------------ landscape material (weight-blended layers, flat colours for now)
LM = '/Game/VeilFall/Materials/M_VF_Landscape'
if not assets.does_asset_exist(LM):
    lm = tools.create_asset('M_VF_Landscape', '/Game/VeilFall/Materials', unreal.Material, unreal.MaterialFactoryNew())
    blend = MEL.create_material_expression(lm, unreal.MaterialExpressionLandscapeLayerBlend, -300, 0)
    layers = []
    for name in ['Grass', 'Pasture', 'Gravel', 'Dirt', 'Asphalt']:
        li = unreal.LayerBlendInput(); li.set_editor_property('layer_name', name); li.set_editor_property('blend_type', unreal.LandscapeLayerBlendType.LB_WEIGHT_BLEND)
        li.set_editor_property('preview_weight', 1.0 if name == 'Grass' else 0.0); layers.append(li)
    blend.set_editor_property('layers', layers)
    for i, name in enumerate(['Grass', 'Pasture', 'Gravel', 'Dirt', 'Asphalt']):
        c = PALETTE[name]
        k = MEL.create_material_expression(lm, unreal.MaterialExpressionVectorParameter, -700, i * 150)
        k.set_editor_property('parameter_name', name + 'Color'); k.set_editor_property('default_value', unreal.LinearColor(c[0], c[1], c[2], 1))
        MEL.connect_material_expressions(k, '', blend, 'Layer ' + name)
    MEL.connect_material_property(blend, '', unreal.MaterialProperty.MP_BASE_COLOR)
    r = MEL.create_material_expression(lm, unreal.MaterialExpressionConstant, -300, 300); r.set_editor_property('r', 0.9)
    MEL.connect_material_property(r, '', unreal.MaterialProperty.MP_ROUGHNESS)
    MEL.recompile_material(lm); assets.save_loaded_asset(lm)

# ------------------------------------------------------------------ dialogue table
DT = '/Game/VeilFall/Data/DT_VF_IntroDialogue'
row = unreal.load_object(None, '/Script/VeilFallG0.VFDialogueLine')
if not assets.does_asset_exist(DT):
    fac = unreal.DataTableFactory(); fac.set_editor_property('struct', row)
    tools.create_asset('DT_VF_IntroDialogue', '/Game/VeilFall/Data', unreal.DataTable, fac)
dt = assets.load_asset(DT)
assert unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(dt, str(ROOT / 'Content' / 'VeilFall' / 'Data' / 'IntroDialogue.csv'))
assets.save_loaded_asset(dt)

# ------------------------------------------------------------------ Blueprints (designer-facing children of the C++ systems)
def blueprint(path, parent, defaults=None):
    folder, name = path.rsplit('/', 1)
    if not assets.does_asset_exist(path):
        fac = unreal.BlueprintFactory(); fac.set_editor_property('parent_class', unreal.load_class(None, '/Script/VeilFallG0.' + parent))
        assert tools.create_asset(name, folder, unreal.Blueprint, fac)
    bp = assets.load_asset(path)
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    cls = unreal.load_class(None, path + '.' + name + '_C'); assert cls, path
    if defaults:
        cdo = unreal.get_default_object(cls)
        for k, v in defaults.items(): cdo.set_editor_property(k, v)
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    assets.save_loaded_asset(bp)
    return unreal.load_class(None, path + '.' + name + '_C')

B = '/Game/VeilFall/Blueprints/'
C = '/Game/VeilFall/Characters/'
PLAYER = blueprint(C + 'Player/BP_VF_Player', 'VFIntroCharacter')
FAMILY = blueprint(C + 'Family/BP_VF_FamilyMember', 'VFIntroNPC', {'faction': unreal.VFFaction.FAMILY, 'essential': True})
LOCAL = blueprint(C + 'NPC/BP_VF_LocalNPC', 'VFIntroNPC', {'faction': unreal.VFFaction.LOCAL, 'essential': True})
NCD = blueprint(C + 'NCD/BP_VF_NCDSoldier', 'VFIntroNPC', {'faction': unreal.VFFaction.NCD, 'armed': True, 'helmet': True, 'weapon_damage': 14.0})
VANGUARD = blueprint(C + 'Vanguard/BP_VF_VanguardSoldier', 'VFIntroNPC', {'faction': unreal.VFFaction.VANGUARD, 'armed': True, 'weapon_damage': 45.0})
INTERACT = blueprint(B + 'Interactions/BP_VF_Interactable', 'VFInteractable')
SECURITY = blueprint(B + 'Interactions/BP_VF_SecurityPanel', 'VFInteractable')
DEFENSE = blueprint(B + 'Interactions/BP_VF_PropertyDefensePanel', 'VFDefensePanel')
PROP = blueprint(B + 'World/BP_VF_AnimatedProp', 'VFAnimatedProp')
VAULTDOOR = blueprint(B + 'World/BP_VF_VaultDoor', 'VFAnimatedProp')
WALL = blueprint(B + 'World/BP_VF_ShopDefenseWall', 'VFAnimatedProp')
ZONE = blueprint(B + 'World/BP_VF_ScriptedDamageZone', 'VFScriptedDamageZone')
VEHICLE = blueprint(B + 'World/TEMP_BP_VF_ScriptedVehicle', 'VFScriptedVehicle')
LINE = blueprint(B + 'World/BP_VF_GreyboxLine', 'VFGreyboxLine')
MISSION = blueprint('/Game/VeilFall/Missions/IntroMission/BP_VF_MissionController_Intro', 'VFMissionController',
                    {'local_class': LOCAL, 'ncd_class': NCD, 'vanguard_class': VANGUARD, 'vehicle_class': VEHICLE})
MODE = blueprint('/Game/VeilFall/Missions/IntroMission/BP_VF_GameMode_Intro', 'VFIntroGameMode', {'default_pawn_class': PLAYER})

# ------------------------------------------------------------------ level
if assets.does_asset_exist(MAP):
    assert levels.load_level(MAP)
    for a in actors.get_all_level_actors():
        if a.get_actor_label().startswith('VF_'): actors.destroy_actor(a)
else:
    assert levels.new_level(MAP, is_partitioned_world=False)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
ws = world.get_world_settings(); ws.set_editor_property('default_game_mode', MODE); ws.set_editor_property('force_no_precomputed_lighting', True)

CUBE = assets.load_asset('/Engine/BasicShapes/Cube.Cube')
CYL = assets.load_asset('/Engine/BasicShapes/Cylinder.Cylinder')
SPHERE = assets.load_asset('/Engine/BasicShapes/Sphere.Sphere')
counts = {}
def label(a, name, folder, tags=()):
    counts[name] = counts.get(name, 0) + 1
    a.set_actor_label('VF_' + name + ('' if counts[name] == 1 else '_%d' % counts[name]))
    a.set_folder_path(unreal.Name('IntroProperty/' + folder))
    if tags: a.set_editor_property('tags', [unreal.Name(t) for t in tags])
    return a

def box(name, folder, center, size, mat, yaw=0.0, pitch=0.0, mesh=None, tags=(), collide=True, hidden=False, profile=None):
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, V(*center), unreal.Rotator(roll=0, pitch=pitch, yaw=yaw))
    comp = a.static_mesh_component
    if 'VF_Scuttle' in tags: comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_static_mesh(mesh or CUBE)
    if mat: comp.set_material(0, MI[mat])
    a.set_actor_scale3d(V(size[0] / 100, size[1] / 100, size[2] / 100))
    if profile: comp.set_collision_profile_name(profile)
    elif not collide: comp.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    if hidden: a.set_actor_hidden_in_game(True)
    return label(a, name, folder, tags)

def wall(name, folder, axis, fixed, a0, a1, z0, z1, thick, mat, openings=(), tags=()):
    """Wall along X (axis='x', fixed=y) or Y (axis='y', fixed=x) with rectangular openings (s0, s1, oz0, oz1)."""
    def piece(s0, s1, b, t):
        if s1 - s0 < 1 or t - b < 1: return
        c = (s0 + s1) / 2; h = (b + t) / 2
        if axis == 'x': box(name, folder, (c, fixed, h), (s1 - s0, thick, t - b), mat, tags=tags)
        else: box(name, folder, (fixed, c, h), (thick, s1 - s0, t - b), mat, tags=tags)
    cur = a0
    for (s0, s1, oz0, oz1) in sorted(openings):
        piece(cur, s0, z0, z1); piece(s0, s1, z0, oz0); piece(s0, s1, oz1, z1); cur = s1
    piece(cur, a1, z0, z1)

def glass(folder, axis, fixed, s0, s1, z0, z1):
    # Window panes: block the player, never bullets or sight (Visibility ignored by InvisibleWall).
    c = (s0 + s1) / 2; h = (z0 + z1) / 2
    size = (s1 - s0, 4, z1 - z0) if axis == 'x' else (4, s1 - s0, z1 - z0)
    box('WindowPane', folder, (c, fixed, h) if axis == 'x' else (fixed, c, h), size, None, hidden=True, profile='InvisibleWall', tags=('VF_Scuttle',))

def mound(name, cx, cy, w, d, h, mat='Grass', slope=9.0, skip=()):
    folder = 'Landscape/Terrain'
    box(name + 'Top', folder, (cx, cy, h - 50), (w, d, 100), mat)
    a = math.radians(slope); L = h / math.tan(a); Ls = math.hypot(L, h) + 60
    for yaw, half, width in [(0, w / 2, d), (180, w / 2, d), (90, d / 2, w), (-90, d / 2, w)]:
        if yaw in skip: continue
        dx, dy = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        off = half + L / 2
        box(name + 'Slope', folder, (cx + dx * off, cy + dy * off, h / 2 - 50 * math.cos(a)), (Ls, width + 40, 100), mat, yaw=yaw, pitch=-slope)

def spawn(cls, loc, yaw=0.0, name='Actor', folder='Gameplay', props=None, pitch=0.0):
    a = actors.spawn_actor_from_class(cls, V(*loc), unreal.Rotator(roll=0, pitch=pitch, yaw=yaw))
    assert a, name
    for k, v in (props or {}).items(): a.set_editor_property(k, v)
    return label(a, name, folder)

def marker(mid, x, y, yaw=0.0, z=10.0, pitch=0.0):
    return spawn(unreal.VFMarker, (x, y, z), yaw, 'Marker_' + mid, 'Gameplay/Markers', {'marker_id': mid}, pitch)

def cover(group, x, y, yaw=180.0):
    return spawn(unreal.VFCoverPoint, (x, y, 10), yaw, 'Cover_' + group, 'Gameplay/Cover/' + group, {'group': group})

def interact(cls, iid, prompt, loc, scale, mat_color, yaw=0.0, screen='', folder='Gameplay/Interactions', states=None, oneshot=False, hide=False):
    a = spawn(cls, loc, yaw, 'Use_' + iid.replace('.', '_'), folder,
              {'interaction_id': iid, 'prompt': unreal.Text(prompt), 'color': mat_color, 'screen_text': screen, 'one_shot': oneshot, 'hide_when_used': hide})
    if states: a.set_editor_property('active_states', states)
    a.get_editor_property('mesh').set_editor_property('relative_scale3d', V(*scale))
    a.rebuild()
    return a

def prop(cls, pid, loc, yaw, mesh_loc, mesh_scale, col, open_offset=(0, 0, 0), open_rot=(0, 0, 0), duration=1.5, folder='Gameplay/Doors', tags=()):
    a = spawn(cls, loc, yaw, 'Prop_' + pid, folder, {'prop_id': pid, 'color': col, 'open_offset': V(*open_offset),
              'open_rotation': unreal.Rotator(roll=open_rot[2], pitch=open_rot[0], yaw=open_rot[1]), 'duration': duration})
    m = a.get_editor_property('mesh'); m.set_editor_property('relative_location', V(*mesh_loc)); m.set_editor_property('relative_scale3d', V(*mesh_scale))
    if tags: a.set_editor_property('tags', [unreal.Name(t) for t in tags])
    a.rebuild()
    return a

def line(name, start, end, trees=True, spacing=600.0, rows=2, jitter=250.0, seed=7, folder='Landscape/Trees'):
    a = spawn(LINE, (start[0], start[1], 0), 0, name, folder, {'trees': trees, 'end': V(end[0] - start[0], end[1] - start[1], 0), 'spacing': spacing, 'rows': rows, 'jitter': jitter, 'seed': seed})
    a.rebuild(); return a

S = ('VF_Scuttle',)
# ================================================================== LANDSCAPE & BOUNDARIES (X+ = north toward the road)
# Real terrain: once a Landscape has been imported (Docs/VeilFall/TerrainAndSky.md) the greybox ground, pasture
# patches and box mounds are skipped and the landscape gets M_VF_Landscape.
LANDSCAPES = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.LandscapeProxy)]
if LANDSCAPES:
    for l in LANDSCAPES:
        if not l.get_editor_property('landscape_material'): l.set_editor_property('landscape_material', assets.load_asset('/Game/VeilFall/Materials/M_VF_Landscape'))
else:
    box('Ground', 'Landscape', (11000, 0, -50), (40000, 32000, 100), 'Grass')
    box('PastureWest', 'Landscape', (12000, -7500, 1), (18000, 8000, 2), 'Pasture', collide=False)
    box('PastureEast', 'Landscape', (12000, 8000, 1), (16000, 6000, 2), 'Pasture', collide=False)
box('CountyRoad', 'Landscape/Road', (26200, 0, 2), (900, 32000, 4), 'Asphalt')
box('Driveway', 'Landscape/Road', (12900, 1100, 2), (24200, 650, 4), 'Gravel')
box('DrivewayBypass', 'Landscape/Road', (14500, 2500, 1), (6000, 400, 2), 'Dirt', collide=False)
box('ShopApron', 'Landscape/Road', (1650, 1100, 3), (1700, 2600, 6), 'Concrete')
box('Parking', 'Landscape/Road', (1500, -1300, 2), (1400, 2000, 4), 'Gravel')
if not LANDSCAPES:  # the heightmap reproduces these hills
    mound('RidgeNE', 14000, 9500, 4000, 2500, 520)
    mound('RiseWest', 9500, -9500, 5000, 2500, 300, 'Pasture')
    mound('GateHill', 21500, 7500, 2500, 2500, 380)
    mound('VaultBerm', -1960, 700, 900, 1800, 300, skip=(0,))  # earth-sheltered vault: no slope toward the shop
# Culvert: the landmark the column halts at.
box('Culvert', 'Landscape/Road', (14400, 1100, 30), (160, 1300, 60), 'Concrete')
for y in (350, 1850): box('CulvertHeadwall', 'Landscape/Road', (14400, y, 50), (260, 120, 100), 'Concrete')
# Fences (posts collide, rails are visual) and tree lines.
line('FenceFrontWest', (25000, -12000), (25000, 650), trees=False, spacing=300, rows=1, folder='Landscape/Fences')
line('FenceFrontEast', (25000, 1550), (25000, 12000), trees=False, spacing=300, rows=1, folder='Landscape/Fences')
line('FenceWest', (-6000, -12000), (25000, -12000), trees=False, spacing=300, rows=1, folder='Landscape/Fences')
line('FenceEast', (-6000, 12000), (25000, 12000), trees=False, spacing=300, rows=1, folder='Landscape/Fences')
line('FencePastureWest', (2600, -2900), (20000, -2900), trees=False, spacing=350, rows=1, folder='Landscape/Fences')
line('FencePastureEast', (5000, 5200), (20000, 5200), trees=False, spacing=350, rows=1, folder='Landscape/Fences')
line('TreeLineWest', (1000, -6900), (23500, -6900), spacing=700, rows=2, seed=3)
line('TreeLineEast', (3500, 6800), (20500, 6800), spacing=700, rows=2, seed=5)
line('TreeLineBack', (-5200, -11500), (-5200, 11500), spacing=650, rows=3, seed=9)
line('TreeLineRoadWest', (24000, -11500), (24000, -2500), spacing=900, rows=1, seed=11)
# Driveway tree rows, both sides, set back ~17 m from the gravel so the firing ports still see straight down the drive.
# The first ~30 m from the shop stays open; the rows start at the barn/shed line.
line('DrivewayTreesWest', (3500, -650), (23500, -650), spacing=650, rows=1, jitter=120, seed=13)
line('DrivewayTreesEast', (3500, 2850), (23500, 2850), spacing=650, rows=1, jitter=120, seed=21)
line('YardTrees', (-1800, -3600), (2600, -3600), spacing=1100, rows=1, jitter=200, seed=17)
line('YardTreesEast', (-2600, 3400), (-600, 5200), spacing=900, rows=1, jitter=150, seed=19)
# Rural utilities.
for i, y in enumerate(range(-14000, 15000, 4000)): box('PowerPoleRoad', 'Landscape/Utilities', (26800, y, 500), (30, 30, 1000), 'Pole', mesh=CYL)
for x in range(3000, 25000, 4000): box('PowerPoleDrive', 'Landscape/Utilities', (x, 2950, 450), (28, 28, 900), 'Pole', mesh=CYL)
box('Transformer', 'Landscape/Utilities', (3000, 2950, 700), (60, 60, 90), 'Utility', mesh=CYL)
box('WaterTank', 'Landscape/Utilities', (-1800, -1800, 250), (350, 350, 500), 'Utility', mesh=CYL)
box('PropaneTank', 'Landscape/Utilities', (300, 3300, 70), (110, 110, 300), 'Appliance', mesh=CYL, pitch=90)
box('WellHouse', 'Landscape/Utilities', (-2500, -3000, 100), (250, 250, 200), 'BarnRed')
box('Mailbox', 'Landscape/Utilities', (25500, 400, 60), (50, 30, 120), 'Dark')

# ================================================================== GATE
box('GatePost', 'Gate', (25000, 700, 90), (30, 30, 180), 'Pole', mesh=CYL)
box('GatePost', 'Gate', (25000, 1500, 90), (30, 30, 180), 'Pole', mesh=CYL)
box('GateArch', 'Gate', (25000, 1100, 330), (30, 900, 30), 'Pole')
prop(PROP, 'gate', (25000, 720, 0), 90, (380, 0, 75), (7.6, .08, 1.2), color('Steel'), open_rot=(0, -95, 0), duration=2.0, folder='Gate')

# ================================================================== BARNDOMINIUM (house X -700..700, Y -2400..0; floor z=10)
F = 'House'
box('HouseSlab', F, (0, -1200, 5), (1400, 2400, 10), 'WoodFloor', tags=S)
EXT, H = 'BarndoSiding', 320
wall('HouseNorth', F, 'y', 700, -2400, 0, 10, H + 10, 20, EXT, [(-2010, -1890, 10, 230), (-1700, -1400, 100, 240), (-1100, -800, 100, 240), (-500, -200, 100, 240)], S)
wall('HouseSouth', F, 'y', -700, -2400, 0, 10, H + 10, 20, EXT, [(-2200, -1900, 110, 220), (-1300, -1000, 110, 220), (-550, -250, 110, 220)], S)
wall('HouseWest', F, 'x', -2400, -700, 700, 10, H + 10, 20, EXT, [(150, 550, 100, 240)], S)
for (s0, s1, z0, z1) in [(-1700, -1400, 100, 240), (-1100, -800, 100, 240), (-500, -200, 100, 240)]: glass(F, 'y', 700, s0, s1, z0, z1)
for (s0, s1, z0, z1) in [(-2200, -1900, 110, 220), (-1300, -1000, 110, 220), (-550, -250, 110, 220)]: glass(F, 'y', -700, s0, s1, z0, z1)
glass(F, 'x', -2400, 150, 550, 100, 240)
# interior
IW = 'InteriorWall'
wall('HallBedroomWall', F, 'y', -200, -2400, 0, 10, 300, 14, IW, [(-2010, -1880, 10, 220), (-1265, -1135, 10, 220), (-515, -385, 10, 220)], S)
box('TVWall', F, (0, -1575, 160), (14, 550, 300), IW, tags=S)
wall('BedroomDivider', F, 'x', -1500, -700, -200, 10, 300, 14, IW, (), S)
wall('BedroomDivider', F, 'x', -750, -700, -200, 10, 300, 14, IW, (), S)
box('HouseRoof', F, (0, -1200, H + 20), (1500, 2500, 20), 'Roof', tags=S)
box('PorchSlab', F, (850, -1500, 4), (300, 1600, 8), 'Concrete', tags=S)
box('PorchBeam', F, (950, -1500, 300), (20, 1600, 20), 'BarndoTrim', tags=S)
for y in (-2250, -750): box('PorchPost', F, (950, y, 150), (20, 20, 290), 'BarndoTrim', tags=S)
# furniture (placeholder)
FF = 'House/Furniture'
box('Sofa', FF, (250, -2150, 45), (90, 260, 70), 'Fabric', yaw=0, tags=S)
box('CoffeeTable', FF, (330, -1900, 30), (90, 140, 40), 'Furniture', tags=S)
box('Rug', FF, (330, -1900, 11), (260, 360, 2), 'Fabric', collide=False, tags=S)
box('KitchenIsland', FF, (350, -650, 50), (120, 300, 90), 'Furniture', tags=S)
box('KitchenCounter', FF, (640, -500, 50), (80, 700, 90), 'Appliance', tags=S)
box('Fridge', FF, (620, -80, 100), (80, 90, 190), 'Appliance', tags=S)
box('DiningTable', FF, (330, -1250, 45), (110, 200, 75), 'Furniture', tags=S)
box('BedParents', FF, (-450, -2050, 35), (200, 180, 50), 'Bedding', tags=S)
box('BedKid', FF, (-550, -1300, 30), (190, 100, 40), 'Bedding', tags=S)
box('BedKid', FF, (-550, -950, 30), (190, 100, 40), 'Bedding', tags=S)
box('BedKid', FF, (-550, -400, 30), (190, 100, 40), 'Bedding', tags=S)
box('Bookshelf', FF, (-30, -2200, 100), (30, 150, 190), 'Furniture', tags=S)
box('ShoeBench', FF, (620, -1700, 25), (50, 120, 40), 'Furniture', tags=S)

# ================================================================== ATTACHED SHOP (X -800..800, Y 0..2200)
F = 'Shop'
box('ShopSlab', F, (0, 1100, 5), (1600, 2200, 10), 'Concrete', tags=S)
SH = 610
# Shared house/shop wall (Y=0) with the interior door into the kitchen.
wall('HouseShopWall', F, 'x', 0, -800, 800, 10, SH, 20, 'ShopInterior', [(100, 240, 10, 230)], S)
# North face to the driveway: bay 1, defensive bay section (armored plate below, fold panel 110..230), bay 2.
wall('ShopNorth', F, 'y', 800, 0, 2200, 10, SH, 24, EXT, [(150, 650, 10, 460), (800, 1500, 10, 230), (1600, 2050, 10, 460)], S)
box('ArmoredPlate', F, (800, 1150, 60), (30, 700, 100), 'Steel', tags=S)
for y in (975, 1150, 1325): box('FiringPortPost', F, (800, y, 170), (30, 14, 120), 'Steel', tags=S)
box('PortFrameInner', F, (775, 1150, 232), (20, 720, 10), 'Steel', tags=S)
wall('ShopSouth', F, 'y', -800, 0, 2200, 10, SH, 24, EXT, [(500, 700, 10, 230)], S)
wall('ShopEast', F, 'x', 2200, -800, 800, 10, SH, 24, EXT, [(-100, 250, 110, 220), (-610, -490, 10, 220)], S)
box('ShopRoof', F, (0, 1100, SH + 10), (1700, 2300, 20), 'Roof', tags=S)
prop(PROP, 'bay1', (800, 400, 10), 0, (0, 0, 225), (.12, 5.0, 4.5), color('BarndoSiding'), open_offset=(0, 0, 410), duration=2.2, folder='Shop', tags=S)
prop(PROP, 'bay2', (800, 1825, 10), 0, (0, 0, 225), (.12, 4.5, 4.5), color('BarndoSiding'), open_offset=(0, 0, 410), duration=2.2, folder='Shop', tags=S)
prop(PROP, 'house_door', (100, 0, 10), 0, (70, 0, 110), (1.4, .06, 2.2), color('Furniture'), open_rot=(0, 95, 0), duration=1.0, folder='Shop', tags=S)
# The fold-down defensive panel: hinged at the bottom of the second section, folds outward and down.
prop(WALL, 'defense_wall', (812, 1150, 110), 0, (0, 0, 60), (.12, 7.0, 1.2), color('BarndoSiding'), open_rot=(-100, 0, 0), duration=1.2, folder='Shop', tags=S)
FS = 'Shop/Props'
box('FamilyTruck', FS, (-150, 1450, 95), (480, 200, 150), 'Fabric', tags=S)
box('Workbench', FS, (-650, 1650, 50), (100, 700, 90), 'Furniture', tags=S)
box('ToolWall', FS, (-780, 1650, 220), (10, 700, 150), 'Steel', tags=S)
for y in (1250, 1420): box('UtilityCabinet', FS, (-740, y, 100), (60, 150, 190), 'Steel', tags=S)
box('Shelving', FS, (-200, 2150, 120), (600, 60, 230), 'Steel', tags=S)
box('Compressor', FS, (500, 2120, 60), (100, 80, 110), 'Utility', tags=S)
box('StorageCrates', FS, (-300, 60, 50), (200, 80, 90), 'Furniture', tags=S)
interact(INTERACT, 'shop.workbench', 'Look at the tractor parts', (-640, 1500, 105), (.5, .5, .15), color('Utility'), oneshot=True)
# Vault (behind the concealed south panel), earth-sheltered.
F = 'Vault'
box('VaultSlab', F, (-1150, 700, 5), (700, 800, 10), 'Vault', tags=S)
wall('VaultBack', F, 'y', -1500, 300, 1100, 10, 290, 40, 'Vault', (), S)
wall('VaultSide', F, 'x', 300, -1500, -800, 10, 290, 40, 'Vault', (), S)
wall('VaultSide', F, 'x', 1100, -1500, -800, 10, 290, 40, 'Vault', (), S)
box('VaultRoof', F, (-1150, 700, 300), (760, 860, 20), 'Vault', tags=S)
prop(VAULTDOOR, 'vault_door', (-800, 600, 10), 0, (0, 0, 110), (.3, 2.0, 2.2), color('BarndoSiding'), open_offset=(0, -215, 0), duration=2.5, folder='Vault', tags=S)
interact(SECURITY, 'vault.keypad', 'Enter the code', (-778, 760, 140), (.04, .22, .3), color('Dark'), yaw=0, screen='SECURITY', folder='Vault')
for (iid, prompt, loc, sc, col) in [
    ('equip.rifle', 'Take rifle', (-1440, 420, 120), (.15, .8, 1.0), 'Dark'), ('equip.sidearm', 'Take sidearm', (-1440, 620, 100), (.2, .5, .2), 'Dark'),
    ('equip.armor', 'Put on body armor', (-1440, 820, 120), (.3, .6, .8), 'Fabric'), ('equip.ammo', 'Take ammunition', (-1440, 1010, 60), (.5, .5, .4), 'Furniture'),
    ('equip.radio', 'Take radios', (-1150, 1060, 110), (.3, .4, .2), 'Dark'), ('equip.medical', 'Take medical kit', (-950, 1060, 90), (.4, .3, .25), 'BarnRed'),
    ('equip.backpack', 'Check the go bags', (-1150, 340, 60), (.5, .5, .6), 'Fabric'), ('equip.supplies', 'Check emergency supplies', (-950, 340, 60), (.5, .4, .5), 'Utility')]:
    interact(INTERACT, iid, prompt, loc, sc, color(col), folder='Vault/Stations', oneshot=True, hide=iid in ('equip.rifle', 'equip.sidearm', 'equip.armor', 'equip.ammo', 'equip.radio', 'equip.medical'))
box('VaultShelf', F, (-1450, 700, 30), (80, 760, 40), 'Steel', tags=S)

# ================================================================== HOUSEHOLD + SHOP INTERACTIONS
G = 'Gameplay/Interactions'
interact(INTERACT, 'home.window_1', 'Look outside', (690, -1550, 100), (.2, 3.0, .08), color('BarndoTrim'))
interact(INTERACT, 'home.window_2', 'Look outside', (-690, -1150, 110), (.2, 3.0, .08), color('BarndoTrim'))
interact(INTERACT, 'home.photo', 'Look at the photo', (-185, -1700, 160), (.05, .4, .3), color('Furniture'))
interact(INTERACT, 'home.pantry', 'Check the pantry', (450, -60, 110), (.6, .1, 1.8), color('Furniture'))
interact(INTERACT, 'home.scanner', 'Listen to the scanner', (640, -700, 105), (.25, .2, .12), color('Dark'))
interact(INTERACT, 'home.tv', 'Watch TV', (12, -1575, 170), (.06, 1.4, .8), color('Dark'), screen='~ Saturday cartoons ~')
interact(INTERACT, 'home.drawing', 'Look at the drawing', (570, -80, 140), (.02, .3, .4), color('Appliance'))
interact(INTERACT, 'shop.monitors', 'Check the camera monitors', (-450, 22, 170), (.05, 1.4, .7), color('Dark'), yaw=90, screen='CAM 1 GATE   CAM 2 DRIVE')
interact(INTERACT, 'panel.intercom', 'Answer on the PA intercom', (-680, 22, 150), (.05, .3, .4), color('Utility'), yaw=90, screen='PA / GATE')
interact(DEFENSE, 'panel.road', 'ARM road-denial system', (-150, 22, 150), (.06, .7, .6), color('Steel'), yaw=90, screen='APPROACH ROAD\nSAFE')
interact(DEFENSE, 'panel.bay', 'Release the bay defenses', (770, 1560, 150), (.06, .5, .6), color('Steel'), yaw=180, screen='SHOP BAY\nSAFE')

# ================================================================== OUTBUILDINGS & COVER
O = 'Outbuildings'
box('Barn', O, (6500, -4500, 300), (1800, 1200, 600), 'BarnRed')
box('BarnRoof', O, (6500, -4500, 640), (1900, 1300, 80), 'Roof')
box('EquipmentShed', O, (5500, 4500, 200), (1000, 800, 400), 'Utility')
box('Trailer', O, (1500, 3800, 90), (700, 250, 140), 'Steel')
box('Woodpile', O, (-400, 3800, 60), (300, 120, 120), 'Furniture')
box('HayStack', O, (4200, -3600, 90), (300, 300, 180), 'Hay')
CV = 'Cover'
drive_cover = []
for i, x in enumerate([3000, 4600, 6200, 7800, 9400, 11000, 12600, 16800, 18400, 20000]):
    for j, y in enumerate((250, 1950)):
        kind = (i + j) % 3
        if kind == 0: box('HayBale', CV, (x, y, 60), (150, 240, 120), 'Hay')
        elif kind == 1: box('RockPile', CV, (x, y, 50), (200, 220, 100), 'Rock')
        else: box('FallenLog', CV, (x, y, 45), (100, 420, 90), 'Pole', yaw=8)
        drive_cover.append((x + 170, y))
for (x, y) in drive_cover: cover('drive', x, y)
for (x, y) in [(14600, 250), (14600, 1950)]: cover('drive', x, y)
for (x, y) in [(4000, -3500), (5500, -4200 + 900), (7600, -3300), (8500, -4000), (10000, -3500), (6000, -5600), (9000, -5600), (11500, -4200)]: cover('west', x, y)
for (x, y) in [(5000, 4000), (3500, 3800), (2200, 3300), (1300, 4200), (400, 3700), (-500, 4250), (-150, 3200), (6200, 3700)]: cover('east', x, y)
for (x, y) in [(4200, -1800), (5200, -1200), (6200, -2200), (7500, -1500), (4300, 3200), (5600, 2900), (6900, 3300), (8200, 2800), (9500, -900), (9500, 3000), (11000, 400), (11000, 1800)]: cover('vanguard', x, y, 0)
for (x, y) in [(5200, -1200), (7500, -1500), (5600, 2900), (8200, 2800)]: box('VanguardRock', CV, (x - 150, y, 50), (150, 200, 100), 'Rock')

# ================================================================== SCRIPTED DAMAGE ZONES (fictional, scripted VFX only)
for i, x in enumerate([13400, 14600, 15800]):
    z = spawn(ZONE, (x, 1100, 250), 0, 'DamageZone', 'Gameplay/Driveway', {'delay': 0.15 + 0.45 * i})
    z.get_editor_property('box').set_editor_property('box_extent', V(650, 700, 250))

# ================================================================== MARKERS
M = marker
# player
M('player_start', 470, -1750, 180); M('player_kitchen', 400, -600, 90); M('player_shop', 0, 700, 180); M('player_vault', -1000, 700, 180)
M('player_panel', -150, 260, -90); M('player_vault_front', -500, 700, 180); M('player_monitors', -450, 260, -90)
M('player_port', 640, 1062, 0); M('player_baydoor', 650, 400, 0); M('player_east', 100, 2060, 90)
# family
M('wife_kitchen', 470, -420, 180); M('wife_living', 300, -1450, 0); M('wife_vault', -560, 430, 180); M('wife_vault_in', -1200, 760, 180)
M('wife_baydoor', 700, 250, 0); M('wife_shop', 200, 900, 0); M('wife_hall', -100, -1000, -90); M('wife_port', 640, 887, 0)
M('wife_east', 100, 1980, 90); M('wife_kids', -380, 950, 180)
M('oldest_living', 480, -2250, 180); M('middle_room', -330, -1100, 180); M('youngest_living', 520, -1900, 180)
for i, y in enumerate((-1700, -1550, -1400)): M('hallway_%d' % (i + 1), -100, y, 0)
for i, y in enumerate((520, 720, 920)): M('vault_%d' % (i + 1), -1300, y, 0)
for i, y in enumerate((600, 750, 900)): M('kids_shop_%d' % (i + 1), -300, y, 0)
# locals
M('sheriff_shop', 300, 700, 180); M('mayor_shop', 100, 1000, 180); M('nathan_shop', 200, 1300, 180); M('nathan_monitors', -450, 300, -90)
M('sheriff_port', 640, 1237, 0); M('nathan_port', 640, 1412, 0); M('mayor_cover', -480, 1700, 0)
M('defend_point', 900, 1100)
# locals' truck
for i, (x, y) in enumerate([(26200, -9000), (26200, 500), (25300, 1100), (20000, 1100), (6000, 1100), (2500, 1000)]): M('route_truck_%02d' % (i + 1), x, y, 90 if i == 0 else 180)
M('truck_park', 2200, 900, 205)
# NCD arrival / column
M('route_ncd_01', 26200, 14000, -90); M('route_ncd_02', 26200, 9000, -90)
for i, y in enumerate((3000, 4400, 5800)): M('ncd_gate_%d' % (i + 1), 26200, y, -90)
M('pa_source', 26000, 3200, 0, 300)
for i, (x, y) in enumerate([(25600, 2600), (25700, 3700), (25600, 5000), (25700, 6200), (26750, 2800), (26800, 4000), (26750, 5200), (26800, 6400)]): M('gate_t_%02d' % (i + 1), x, y, 180)
M('cam_gate', 23800, -1800, 64, 700, -8)
M('route_column_01', 25300, 1100, 180)
for i, x in enumerate((15600, 14500, 13400)): M('ncd_halt_%d' % (i + 1), x, 1100, 180)
for i, (x, y) in enumerate([(16300, 600), (16100, 1650), (15200, 550), (15000, 1700), (14000, 600), (13800, 1650), (13100, 500), (12900, 1700)]): M('column_t_%02d' % (i + 1), x, y, 180)
for i, (x, y) in enumerate([(12200, 300), (12000, 1950), (11700, -500), (17300, 2300), (17200, -100)]): M('w1_%02d' % (i + 1), x, y, 180)
# waves
for i, (x, y) in enumerate([(26200, 9000), (26200, 1700), (25000, 1100), (17500, 2500), (12000, 2500)]): M('route_wave_%02d' % (i + 1), x, y, 180)
M('w2_stop_1', 9800, 500, 180); M('w2_stop_2', 9600, 1800, 180)
M('w2_tree_01', 8000, -6200, 180); M('w2_tree_02', 9500, -6000, 180)
for i, (x, y) in enumerate([(8000, 6200), (8800, 6000), (7400, 5800), (9500, 6300)]): M('w3_%02d' % (i + 1), x, y, -150)
M('w4_stop_1', 7600, 300, 180); M('w4_stop_2', 7400, 2000, 180)
M('w4_tree_01', 9000, -6200, 180); M('w4_tree_02', 10500, -6400, 180)
# Vanguard
M('vg_w_start', 4500, -16000, 90); M('vg_w_stop', 3800, -2400, 90); M('vg_e_start', 4500, 16000, -90); M('vg_e_stop', 3800, 4600, -90)
M('ncd_front', 8000, 1100); M('aa_origin', 14000, 9500, 0, 900); M('vg_meet', 1150, 1150, 180)
M('evac_1', 2000, 350, 0); M('evac_2', 2100, 1900, 0)
for i, (x, y) in enumerate([(3500, 700), (9000, 700), (12000, 2400), (17000, 2400), (20000, 1100), (24000, 1100), (25300, 1100), (26200, -3000), (26200, -12000)]): M('route_convoy_%02d' % (i + 1), x, y)
for i, (x, y, z) in enumerate([(0, -2000, 200), (0, -800, 200), (-400, -1400, 200), (400, -300, 200), (0, 500, 300), (0, 1600, 300), (-1100, 700, 150), (600, 1100, 300)]): M('scuttle_%02d' % (i + 1), x, y, 0, z)

# ================================================================== FAMILY (placed; locals/NCD/Vanguard spawn from the mission)
def family(role, name, station, colr, scale):
    t = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.VFMarker) and str(a.get_editor_property('marker_id')) == station][0].get_actor_transform()
    l = t.translation
    a = spawn(FAMILY, (l.x, l.y, 110 * scale), t.rotation.rotator().yaw, 'Family_' + role, 'Characters/Family',
              {'role_id': role, 'display_name': name, 'clothing_color': colr, 'body_scale': scale, 'faction': unreal.VFFaction.FAMILY, 'essential': True,
               'kid_animation': role.startswith('son_')})
    return a
family('wife', 'your wife', 'wife_kitchen', unreal.LinearColor(.32, .12, .14, 1), .95)
family('son_oldest', 'your oldest son', 'oldest_living', unreal.LinearColor(.12, .2, .32, 1), .9)
family('son_middle', 'your middle son', 'middle_room', unreal.LinearColor(.2, .3, .12, 1), .78)
family('son_youngest', 'your youngest son', 'youngest_living', unreal.LinearColor(.45, .3, .08, 1), .66)

# ================================================================== FRAMEWORK: mission, start, lighting, navigation
spawn(MISSION, (0, 0, 0), 0, 'MissionController_Intro', 'Gameplay')
spawn(unreal.PlayerStart, (470, -1750, 110), 180, 'PlayerStart', 'Gameplay')
sun = spawn(unreal.DirectionalLight, (0, 0, 5000), 35, 'Sun', 'Lighting', pitch=-38)
sun.light_component.set_editor_property('intensity', 7.0)
sky = spawn(unreal.SkyLight, (0, 0, 5000), 0, 'SkyLight', 'Lighting'); sky.light_component.set_editor_property('real_time_capture', True)
spawn(unreal.SkyAtmosphere, (0, 0, 0), 0, 'Atmosphere', 'Lighting')
spawn(unreal.VolumetricCloud, (0, 0, 0), 0, 'Clouds', 'Lighting')
for k, v in [('atmosphere_sun_light', True), ('cast_cloud_shadows', True)]:
    try: sun.light_component.set_editor_property(k, v)
    except Exception as e: unreal.log_warning('VF sun %s: %s' % (k, e))
fog = spawn(unreal.ExponentialHeightFog, (0, 0, 0), 0, 'HeightFog', 'Lighting'); fog.component.set_editor_property('fog_density', 0.006)
for (x, y, z, inten, col) in [(300, -2000, 290, 25, (1, .85, .7)), (350, -700, 290, 25, (1, .85, .7)), (-450, -1200, 280, 15, (1, .85, .7)),
                               (0, 600, 560, 40, (.9, .95, 1)), (0, 1650, 560, 40, (.9, .95, 1)), (-1150, 700, 270, 20, (.85, .9, 1))]:
    l = spawn(unreal.PointLight, (x, y, z), 0, 'InteriorLight', 'Lighting/Interior')
    l.point_light_component.set_editor_property('intensity_units', unreal.LightUnits.CANDELAS); l.point_light_component.set_editor_property('intensity', float(inten)); l.point_light_component.set_editor_property('light_color', unreal.Color(int(col[0] * 255), int(col[1] * 255), int(col[2] * 255), 255))
    l.point_light_component.set_editor_property('attenuation_radius', 1400.0)
nav = spawn(unreal.NavMeshBoundsVolume, (9000, 0, 300), 0, 'NavBounds', 'Gameplay')
_, ext = nav.get_actor_bounds(False)
assert min(ext.x, ext.y, ext.z) > 1.0, 'Editor volume factory did not create a real brush'
nav.set_actor_scale3d(V(17500 / ext.x, 12000 / ext.y, 800 / ext.z))  # X -8500..26500, Y -12000..12000
# Make the editor create the RecastNavMesh actor for these bounds (the game does not create nav data on its own).
navsys = unreal.NavigationSystemV1.get_navigation_system(world)
if navsys: navsys.on_navigation_bounds_updated(nav)
unreal.SystemLibrary.execute_console_command(world, 'RebuildNavigation')
recast = [a for a in actors.get_all_level_actors() if isinstance(a, unreal.RecastNavMesh)]
if not recast:
    r = actors.spawn_actor_from_class(unreal.RecastNavMesh, V(0, 0, 0))
    recast = [r] if r else []
assert recast, 'no RecastNavMesh in the intro map'
for r in recast:
    r.set_editor_property('runtime_generation', unreal.RuntimeGenerationType.DYNAMIC)
unreal.log('VF_INTRO_NAVDATA %d' % len(recast))

assert levels.save_current_level()
unreal.log('VF_INTRO_MAP_SUCCESS map=%s actors=%d' % (MAP, len([a for a in actors.get_all_level_actors() if a.get_actor_label().startswith('VF_')])))
