"""Idempotently creates a separate real authored G1 map; never edits L_G0."""
import unreal
assets=unreal.EditorAssetLibrary
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
path='/Game/DeadSignal/L_DeadSignal_G1'
assets.make_directory('/Game/DeadSignal')
definition_path='/Game/DeadSignal/DA_DeadSignal'
definition=assets.load_asset(definition_path) if assets.does_asset_exist(definition_path) else None
if not definition:
    factory=unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class',unreal.load_class(None,'/Script/VeilFallG0.G1RouteDefinition'))
    definition=unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_DeadSignal','/Game/DeadSignal',None,factory)
assert definition
definition.set_editor_property('stable_id','operation.dead_signal')
definition.set_editor_property('objectives',[
 'HQ: receive Operation Dead Signal briefing at the yellow-labelled terminal.',
 'Residential edge: reach the survey post. Main street or rear alley.',
 'Relay: mark resistance cleared (GREYBOX combat placeholder).',
 'Relay: restore generator power.',
 'Relay: establish the outpost and reconnect residents.',
 'Convoy: recover the manifest (GREYBOX encounter placeholder).',
 'Continuity facility: inspect the sealed access record.',
 'Ancient chamber: document the impossible architecture.',
 'Return to HQ: debrief and bank the evidence.'
])
assert assets.save_loaded_asset(definition)
if assets.does_asset_exist(path): assert levels.load_level(path)
else: assert levels.new_level(path,is_partitioned_world=False)
cube=assets.load_asset('/Engine/BasicShapes/Cube.Cube')
def spawn(label,cls,xyz):
    matches=[a for a in actors.get_all_level_actors() if a.get_actor_label()==label]
    assert len(matches)<=1
    a=matches[0] if matches else actors.spawn_actor_from_class(cls,unreal.Vector(*xyz))
    assert a
    a.set_actor_label(label)
    a.set_actor_location(unreal.Vector(*xyz),False,False)
    return a
def box(label,xyz,size,rotation=0):
    a=spawn('G1_'+label,unreal.StaticMeshActor,xyz)
    a.static_mesh_component.set_static_mesh(cube)
    a.static_mesh_component.set_collision_profile_name('BlockAll')
    a.static_mesh_component.set_mobility(unreal.ComponentMobility.MOVABLE)
    a.set_actor_scale3d(unreal.Vector(*(v/100 for v in size)))
    a.set_actor_rotation(unreal.Rotator(0,rotation,0),False)
    return a
node_class=unreal.load_class(None,'/Script/VeilFallG0.DeadSignalNode')
def node(stable_id,action,prompt,xyz):
    a=spawn('G1_'+stable_id,node_class,xyz)
    a.set_editor_property('stable_id',stable_id)
    a.set_editor_property('action',action)
    a.set_editor_property('prompt',prompt)
    a.set_editor_property('definition',definition)
    evidence={
     'hq.brief':'Free States radio: reconnect the relay, recover the convoy manifest, and return with evidence.',
     'edge.arrive':'The evacuated street still has power. Utility markings lead behind the houses.',
     'clue.utility':'FICTIONAL LEDGER: sealed circuits predate the county grid. A survey archive is referenced at the facility.',
     'relay.establish':'Residents return. Radio: the Continuity convoy follows the eastern road. Resupply and new arrivals move here.',
     'convoy.recover':'FICTIONAL MANIFEST: modern equipment is assigned to an unnamed structure beneath the facility.',
     'facility.enter':'FICTIONAL ACCESS LOG: archive dates were overwritten. The stated purpose is containment, not excavation.',
     'clue.archive':'FICTIONAL SURVEY: the original drafts call the chamber a boundary instrument. Later copies erase the authors.',
     'chamber.reveal':'The structure continues through the modern foundations at incompatible angles. Record it; return to HQ.',
     'hq.debrief':'Evidence banked at headquarters. The relay remains established. The purpose of the structure is unresolved.'
    }
    a.set_editor_property('evidence_statement',evidence.get(action,''))
    if action not in ('relay.resident','relay.radio'):
        box('Stand_'+stable_id,(xyz[0],xyz[1],25),(60,60,50))
    return a
# 400 x 600 m bounded footprint. Deliberately conventional non-partitioned greybox.
box('Ground',(20000,30000,-60),(40000,60000,120))
box('WestBoundary',(-100,30000,150),(200,60000,300))
box('EastBoundary',(40100,30000,150),(200,60000,300))
box('SouthBoundary',(20000,-100,150),(40000,200,300))
box('NorthBoundary',(20000,60100,150),(40000,200,300))
points=[(2000,4000),(9000,11000),(16000,22000),(26000,32000),(30000,47000),(33000,55000)]
import math
for i,((x1,y1),(x2,y2)) in enumerate(zip(points,points[1:])):
    length=math.hypot(x2-x1,y2-y1)
    box('Route_'+str(i),((x1+x2)/2,(y1+y2)/2,3),(length,800,6),math.degrees(math.atan2(y2-y1,x2-x1)))
for i,y in enumerate((3700,3850,4000,4150)):
    a=spawn('G1_HQStart_'+str(i),unreal.PlayerStart,(1700,y,100));a.set_editor_property('player_start_tag','Headquarters')
for i,y in enumerate((21400,21550,21700,21850)):
    a=spawn('G1_RelayStart_'+str(i),unreal.PlayerStart,(15100,y,100));a.set_editor_property('player_start_tag','RelayEstablished')
node('HQ_Brief','hq.brief','Briefing: reconnect Ashford relay',(2000,4000,100))
node('HQ_Debrief','hq.debrief','Debrief / bank evidence',(2300,4400,100))
for name,xyz,size in [('HQ_Back',(2500,5000,200),(1800,100,400)),('HQ_Left',(1100,4200,200),(100,1600,400)),('HQ_Right',(3000,4200,200),(100,1600,400))]:box(name,xyz,size)
node('Edge_Survey','edge.arrive','Ashford residential survey',(9000,11000,100))
for i,(x,y) in enumerate([(7900,11800),(9400,12300),(10600,11400),(11000,13300)]):
    box('House_'+str(i),(x,y,200),(800,600,400))
box('StreetCover',(9100,11900,60),(250,500,120))
node('Clue_Utility','clue.utility','Optional: utility ledger / anomalous sealed circuits',(7400,12800,100))
node('Relay_Clear','relay.clear','Mark cleared: combat placeholder',(16000,22000,100))
node('Relay_Power','relay.power','Restore relay generator',(16500,22500,100))
node('Relay_Establish','relay.establish','Establish Free States outpost',(15700,22700,100))
node('Relay_Resupply','relay.resupply','Resupply station: G2 supplies pending',(15100,22200,100))
node('Relay_Resident','relay.resident','Residents returned / authored placeholder',(15300,22900,100))
node('Relay_Radio','relay.radio','Radio: convoy route confirmed east / facility beyond',(15500,23000,100))
box('RelayMast',(16600,23000,1000),(60,60,2000))
box('RelayCoverFront',(16000,21400,75),(1200,100,150))
box('RelayCoverSide',(16800,21800,75),(100,800,150))
node('Convoy_Manifest','convoy.recover','Recover convoy manifest: encounter placeholder',(26000,32000,100))
for i in range(3):
    box('ConvoyTruck_'+str(i),(25200+i*800,32800,160),(600,250,320),25)
    box('ConvoyCover_'+str(i),(25000+i*1000,31200,65),(250,150,130))
node('Facility_Access','facility.enter','Continuity: sealed access record',(30000,47000,100))
node('Clue_Archive','clue.archive','Optional: pre-Veil survey / overwritten provenance',(29000,48200,100))
box('FacilityWest',(28800,49000,350),(100,5000,700))
box('FacilityEast',(31200,49000,350),(100,5000,700))
box('FacilityDivider',(30000,48800,175),(800,100,350))
node('Chamber_Reveal','chamber.reveal','Document structure: modern foundations meet impossible geometry',(33000,55000,100))
for i in range(4):
    box('AncientPillar_'+str(i),(31800+i*800,56400,750),(200,200,1500),i*25)
box('AncientCrossbeam',(33000,56400,1550),(3200,180,180),15)
box('AncientSlantedBridge',(33300,56000,900),(2400,100,100),-35)
box('ChamberModernWall',(34000,54800,400),(120,1400,800))
box('ChamberRoof',(33000,55400,1900),(3800,4200,120))
box('ChamberWestWall',(31200,55400,900),(120,4200,1800))
box('ChamberBackWall',(33000,57500,900),(3800,120,1800))
box('ChamberFrontLeft',(31900,53300,900),(1500,120,1800))
box('ChamberFrontRight',(34100,53300,900),(1500,120,1800))
light=spawn('G1_ChamberWorkLight',unreal.PointLight,(33000,55000,550))
light.point_light_component.set_editor_property('intensity',8000.0)
light.point_light_component.set_editor_property('attenuation_radius',3500.0)
sun=spawn('G1_Sun',unreal.DirectionalLight,(0,0,5000));sun.set_actor_rotation(unreal.Rotator(-45,-30,0),False)
spawn('G1_Sky',unreal.SkyLight,(0,0,5000));spawn('G1_Atmosphere',unreal.SkyAtmosphere,(0,0,0))
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/VeilFallG0.DeadSignalGameMode'))
world.get_world_settings().set_editor_property('force_no_precomputed_lighting',True)
for a in actors.get_all_level_actors():
    if isinstance(a,unreal.Light):
        for component in a.get_components_by_class(unreal.SceneComponent):
            component.set_mobility(unreal.ComponentMobility.MOVABLE)
nodes=[a for a in actors.get_all_level_actors() if isinstance(a,unreal.DeadSignalNode)]
assert len(nodes)==14
assert len({str(a.get_editor_property('stable_id')) for a in nodes})==14
assert len(definition.get_editor_property('objectives'))==9
for a in nodes: assert a.get_editor_property('definition')==definition
assert levels.save_current_level()
unreal.log('G1_MAP_SUCCESS actors='+str(len(actors.get_all_level_actors())))
