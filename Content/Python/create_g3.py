"""Create a separate real G3 encounter map, bounded navigation and loot assets."""
import unreal
assets=unreal.EditorAssetLibrary
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
path='/Game/DeadSignal/L_DeadSignal_G3'
if not assets.does_asset_exist(path):
    assert assets.duplicate_asset('/Game/DeadSignal/L_DeadSignal_G2',path)
assert levels.load_level(path)
for actor in actors.get_all_level_actors():
    if actor.get_actor_label().startswith('G3_'): actors.destroy_actor(actor)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/VeilFallG0.CoopGameMode'))
definition_path='/Game/DeadSignal/DA_DeadSignal_G3'
if not assets.does_asset_exist(definition_path):
    assert assets.duplicate_asset('/Game/DeadSignal/DA_DeadSignal',definition_path)
definition=assets.load_asset(definition_path)
objectives=list(definition.get_editor_property('objectives'))
objectives[2]='Relay: neutralize the three Continuity guards, then secure the station.'
objectives[5]='Convoy: clear the three guards and recover the manifest.'
definition.set_editor_property('objectives',objectives)
assert assets.save_loaded_asset(definition)
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.DeadSignalNode):
        a.set_editor_property('definition',definition)
        action=str(a.get_editor_property('action'))
        if action=='relay.clear': a.set_editor_property('prompt','Secure relay: neutralize all Continuity guards first')
        if action=='convoy.recover': a.set_editor_property('prompt','Recover manifest: neutralize convoy guards first')
        if action=='relay.resupply': a.set_editor_property('prompt','Restock weapons and two equipped gadgets')

def spawn(label,cls,xyz,yaw=0.0):
    a=actors.spawn_actor_from_class(cls,unreal.Vector(*xyz),unreal.Rotator(pitch=0.0,yaw=yaw,roll=0.0))
    assert a
    a.set_actor_label('G3_'+label)
    return a

cube=assets.load_asset('/Engine/BasicShapes/Cube')
def box(label,xyz,size):
    a=spawn(label,unreal.StaticMeshActor,xyz)
    c=a.static_mesh_component;c.set_static_mesh(cube);c.set_mobility(unreal.ComponentMobility.MOVABLE)
    c.set_collision_profile_name('BlockAll')
    a.set_actor_scale3d(unreal.Vector(*[v/100.0 for v in size]))
    return a

spawn('SquadCoordinator',unreal.SquadCoordinator,(0,0,0))
for group,positions in {
    'relay':[(15800,22250),(16400,22800),(17600,22400)],
    'convoy':[(25300,32200),(26600,32500),(27400,33200)]
}.items():
    for i,(x,y) in enumerate(positions):
        a=spawn(group+'_Enemy_'+str(i),unreal.SquadEnemy,(x,y,100),-135)
        a.set_editor_property('stable_id',group+'.enemy.'+str(i));a.set_editor_property('encounter',group)
        a.set_editor_property('squad_role',[unreal.SquadRole.RIFLEMAN,unreal.SquadRole.SUPPORT,unreal.SquadRole.MARKSMAN][i])

for group,positions in {
    'relay':[(15000,21900),(15600,22400),(16500,23000),(17500,22400)],
    'convoy':[(24900,32000),(25800,32400),(27000,33000),(27700,32700)]
}.items():
    for i,(x,y) in enumerate(positions):
        box(group+'_CoverWall_'+str(i),(x,y-130,50),(250,80,100))
        a=spawn(group+'_CoverPoint_'+str(i),unreal.SquadCover,(x,y,50))
        a.set_editor_property('stable_id',group+'.cover.'+str(i));a.set_editor_property('encounter',group)
    center=(16300,22300,250) if group=='relay' else (26300,32500,250)
    volume=spawn(group+'_NavigationBounds',unreal.NavMeshBoundsVolume,center)
    _,extent=volume.get_actor_bounds(False)
    assert min(extent.x,extent.y,extent.z)>1.0, 'Editor volume factory did not create a real brush'
    volume.set_actor_scale3d(unreal.Vector(5500/(2*extent.x),5500/(2*extent.y),1000/(2*extent.z)))
    _,extent=volume.get_actor_bounds(False)
    assert abs(extent.x-2750)<1 and abs(extent.y-2750)<1 and abs(extent.z-500)<1
    unreal.log('G3_NAV_VOLUME '+group+' '+str(extent))

nav_data=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.RecastNavMesh)]
nav=nav_data[0] if nav_data else spawn("RuntimeNavigation",unreal.RecastNavMesh,(0,0,0))
nav.set_editor_property("runtime_generation",unreal.RuntimeGenerationType.DYNAMIC)
assets.make_directory('/Game/Coop')
loot={}
for short,title in [('medical','Medical supplies'),('ammunition','Ammunition supplies'),('parts','Equipment parts')]:
    p='/Game/Coop/DA_'+short
    d=assets.load_asset(p) if assets.does_asset_exist(p) else None
    if not d:
        factory=unreal.DataAssetFactory();factory.set_editor_property('data_asset_class',unreal.load_class(None,'/Script/VeilFallG0.FieldLootDefinition'))
        d=unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_'+short,'/Game/Coop',None,factory)
    d.set_editor_property('stable_id','loot.'+short);d.set_editor_property('display_name',title)
    assert assets.save_loaded_asset(d);loot[short]=d
for i,(xyz,kind) in enumerate([((8700,11200,20),'medical'),((15200,22400,20),'ammunition'),((16300,23000,20),'parts'),((26200,32000,20),'parts'),((27600,33000,20),'ammunition'),((29100,48200,20),'medical')]):
    a=spawn('FieldPickup_'+str(i),unreal.FieldPickup,xyz,180)
    a.set_editor_property('stable_id','dead_signal.pickup.'+str(i));a.set_editor_property('definition',loot[kind])
spawn('HeadquartersBank',unreal.FieldBank,(2150,3800,40),180)
enemies=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.SquadEnemy)]
assert len(enemies)==6 and len({str(a.get_editor_property('stable_id')) for a in enemies})==6
assert levels.save_current_level()
unreal.log('G3_MAP_SUCCESS enemies=6 cover_points=8 nav_volumes=2 loot_assets=3 pickups=6 bank=1')

