"""VEILFALL developer test range: /Game/VeilFall/Dev/L_VF_TestRange.

A small flat map for trying mechanics without playing the intro: a row of cover pieces at labelled heights, a
training-target lane and an enemy yard. Re-runnable: every generated actor is labelled TR_* and rebuilt; anything a
designer placed by hand is left alone. Needs the intro materials/Blueprints (Scripts/VFIntro.ps1 Map) to exist.
Run: Scripts/VFTestRange.ps1 Map
"""
import unreal

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
levels = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
MAP = '/Game/VeilFall/Dev/L_VF_TestRange'
V = unreal.Vector
assets.make_directory('/Game/VeilFall/Dev')

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

PLAYER = unreal.load_class(None, '/Game/VeilFall/Characters/Player/BP_VF_Player.BP_VF_Player_C')
NCD = unreal.load_class(None, '/Game/VeilFall/Characters/NCD/BP_VF_NCDSoldier.BP_VF_NCDSoldier_C')
assert PLAYER and NCD, 'run Scripts/VFIntro.ps1 Map first (intro Blueprints missing)'
MODE = blueprint('/Game/VeilFall/Dev/BP_VF_GameMode_TestRange', 'VFTestRangeGameMode', {'default_pawn_class': PLAYER})
RANGE = blueprint('/Game/VeilFall/Dev/BP_VF_TestRange', 'VFTestRange')

if assets.does_asset_exist(MAP):
    assert levels.load_level(MAP)
    for a in actors.get_all_level_actors():
        if a.get_actor_label().startswith('TR_'): actors.destroy_actor(a)
else:
    assert levels.new_level(MAP, is_partitioned_world=False)
world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
ws = world.get_world_settings(); ws.set_editor_property('default_game_mode', MODE); ws.set_editor_property('force_no_precomputed_lighting', True)

CUBE = assets.load_asset('/Engine/BasicShapes/Cube.Cube')
def mi(name): return assets.load_asset('/Game/VeilFall/Materials/TEMP_MI_' + name)
counts = {}
def label(a, name, folder, tags=()):
    counts[name] = counts.get(name, 0) + 1
    a.set_actor_label('TR_' + name + ('' if counts[name] == 1 else '_%d' % counts[name]))
    a.set_folder_path(unreal.Name('TestRange/' + folder))
    if tags: a.set_editor_property('tags', [unreal.Name(t) for t in tags])
    return a

def box(name, folder, center, size, mat, yaw=0.0, tags=()):
    a = actors.spawn_actor_from_class(unreal.StaticMeshActor, V(*center), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    c = a.static_mesh_component; c.set_static_mesh(CUBE); c.set_material(0, mi(mat))
    a.set_actor_scale3d(V(size[0] / 100, size[1] / 100, size[2] / 100))
    return label(a, name, folder, tags)

def sign(text, loc, yaw=180.0, size=28.0, folder='Signs'):
    a = actors.spawn_actor_from_class(unreal.TextRenderActor, V(*loc), unreal.Rotator(roll=0, pitch=0, yaw=yaw))
    t = a.text_render
    t.set_editor_property('text', unreal.Text(text)); t.set_editor_property('world_size', size)
    t.set_editor_property('horizontal_alignment', unreal.HorizTextAligment.EHTA_CENTER)
    t.set_editor_property('text_render_color', unreal.Color(r=255, g=230, b=150, a=255))
    return label(a, 'Sign', folder)

def spawn(cls, loc, yaw=0.0, name='Actor', folder='Gameplay', props=None):
    a = actors.spawn_actor_from_class(cls, V(*loc), unreal.Rotator(roll=0, pitch=0, yaw=yaw)); assert a, name
    for k, v in (props or {}).items(): a.set_editor_property(k, v)
    return label(a, name, folder)

# ------------------------------------------------------------------ ground and walls (X+ = downrange)
box('Floor', 'Ground', (0, 0, -10), (7000, 5000, 20), 'Concrete')
for (c, s) in [((3500, 0, 200), (40, 5000, 400)), ((-3500, 0, 200), (40, 5000, 400)), ((0, 2500, 200), (7000, 40, 400)), ((0, -2500, 200), (7000, 40, 400))]:
    box('BoundaryWall', 'Ground', c, s, 'BarndoSiding')
box('LaneDivider', 'Ground', (1800, 0, 60), (2600, 30, 120), 'Steel')

# ------------------------------------------------------------------ cover row: one piece per height, faces -X (the player side)
# Heights cover the intro's real pieces: 90 waist (Q/E pop), 110 shop armored plate, 120 hay bale, 220 too tall to pop over.
sign('COVER ROW: crouch (Ctrl/C) at a piece, hold RMB to pop up, release to drop', (-1500, 0, 330), size=34)
for i, h in enumerate([60, 90, 110, 120, 150, 220]):
    y = -1750 + i * 700
    box('Cover_%dcm' % h, 'Cover', (-1200, y, h / 2), (30, 300, h), 'Rock' if h < 200 else 'BarndoSiding', tags=('VF_CoverTest_%d' % h,))
    sign('%d cm' % h, (-1230, y, h + 25), size=30)
# Side-lean practice: a tall wall with an open right edge, and an L corner.
box('LeanWall', 'Cover', (-300, -1600, 110), (30, 500, 220), 'BarndoSiding')
sign('Q/E: lean around the edge', (-330, -1600, 250))
box('CornerA', 'Cover', (-300, 1400, 110), (30, 400, 220), 'BarndoSiding'); box('CornerB', 'Cover', (-115, 1585, 110), (400, 30, 220), 'BarndoSiding')

# ------------------------------------------------------------------ target lane (Y < 0): training dummies at range, reset themselves
sign('TARGET LANE (dummies reset after dying)', (500, -1300, 260), yaw=180.0)
armors = [unreal.VeilArmor.LIGHT, unreal.VeilArmor.MEDIUM, unreal.VeilArmor.HEAVY]
for i, x in enumerate([1000, 1800, 2600]):
    for j, y in enumerate([-2000, -1400, -800]):
        spawn(unreal.CombatTarget, (x, y, 105), 180, 'Target', 'Targets', {'armor_preset': armors[(i + j) % 3]})

# ------------------------------------------------------------------ enemy yard (Y > 0): NCD riflemen behind cover, hold and fire, respawn
sign('ENEMY YARD (NCD hold their posts, respawn after 6 s)', (500, 1300, 260), yaw=180.0)
for (x, y) in [(2300, 700), (2700, 1300), (2300, 1900), (3000, 1000)]:
    box('EnemyCover', 'Enemies', (x - 90, y, 60), (40, 260, 120), 'Hay')
    spawn(NCD, (x, y, 100), 180, 'NCD', 'Enemies', {'weapon_damage': 6.0})

# ------------------------------------------------------------------ manager, start, lighting, navigation
spawn(RANGE, (-2800, 0, 50), 0, 'TestRange', 'Gameplay')
spawn(unreal.PlayerStart, (-2800, 0, 110), 0, 'PlayerStart', 'Gameplay')
sun = spawn(unreal.DirectionalLight, (0, 0, 3000), 35, 'Sun', 'Lighting'); sun.set_actor_rotation(unreal.Rotator(roll=0, pitch=-45, yaw=35), False)
sun.light_component.set_editor_property('intensity', 7.0)
try: sun.light_component.set_editor_property('atmosphere_sun_light', True)
except Exception as e: unreal.log_warning('TR sun: %s' % e)
sky = spawn(unreal.SkyLight, (0, 0, 3000), 0, 'SkyLight', 'Lighting'); sky.light_component.set_editor_property('real_time_capture', True)
spawn(unreal.SkyAtmosphere, (0, 0, 0), 0, 'Atmosphere', 'Lighting')
spawn(unreal.ExponentialHeightFog, (0, 0, 0), 0, 'HeightFog', 'Lighting')
nav = spawn(unreal.NavMeshBoundsVolume, (0, 0, 200), 0, 'NavBounds', 'Gameplay')
_, ext = nav.get_actor_bounds(False)
assert min(ext.x, ext.y, ext.z) > 1.0, 'Editor volume factory did not create a real brush'
nav.set_actor_scale3d(V(3600 / ext.x, 2600 / ext.y, 400 / ext.z))
navsys = unreal.NavigationSystemV1.get_navigation_system(world)
if navsys: navsys.on_navigation_bounds_updated(nav)
unreal.SystemLibrary.execute_console_command(world, 'RebuildNavigation')
if not [a for a in actors.get_all_level_actors() if isinstance(a, unreal.RecastNavMesh)]:
    actors.spawn_actor_from_class(unreal.RecastNavMesh, V(0, 0, 0))

all_tr = [a for a in actors.get_all_level_actors() if a.get_actor_label().startswith('TR_')]
assert len([a for a in all_tr if a.actor_has_tag('VF_CoverTest_110')]) == 1
assert len([a for a in all_tr if isinstance(a, unreal.VFIntroNPC)]) == 4 and len([a for a in all_tr if isinstance(a, unreal.CombatTarget)]) == 9
assert levels.save_current_level()
unreal.log('VF_TESTRANGE_MAP_SUCCESS map=%s actors=%d' % (MAP, len(all_tr)))
