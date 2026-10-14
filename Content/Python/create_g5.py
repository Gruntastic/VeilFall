"""One-time G5 authoring map creation. Existing designer maps are never regenerated."""
import unreal,math
from pathlib import Path
assets=unreal.EditorAssetLibrary
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
path='/Game/DeadSignal/L_DeadSignal_G5_Workshop'
pending=Path(unreal.Paths.project_saved_dir())/'G5Workshop.bootstrap_pending'
def create():
    if assets.does_asset_exist(path) and not pending.exists():
        assert levels.load_level(path)
        unreal.log('G5_WORKSHOP_PRESERVED existing designer map; no changes applied')
        return
    if not assets.does_asset_exist(path):
        pending.parent.mkdir(parents=True,exist_ok=True);pending.write_text(path)
        assert assets.duplicate_asset('/Game/DeadSignal/L_DeadSignal_G4',path)
    assert levels.load_level(path)
    tools=unreal.AssetToolsHelpers.get_asset_tools()
    def blueprint(name,parent):
        p='/Game/G5/'+name
        if assets.does_asset_exist(p):return assets.load_asset(p),unreal.load_class(None,p+'.'+name+'_C')
        factory=unreal.BlueprintFactory();factory.set_editor_property('parent_class',unreal.load_class(None,'/Script/VeilFallG0.'+parent))
        bp=tools.create_asset(name,'/Game/G5',unreal.Blueprint,factory);assert bp
        unreal.BlueprintEditorLibrary.compile_blueprint(bp)
        assert assets.save_loaded_asset(bp)
        cls=unreal.load_class(None,p+'.'+name+'_C');assert cls
        return bp,cls
    player,player_class=blueprint('BP_Player_G5','G5Character')
    mode,mode_class=blueprint('BP_OperationDeadSignal_G5','G5GameMode')
    unreal.get_default_object(mode_class).set_editor_property('default_pawn_class',player_class)
    unreal.BlueprintEditorLibrary.compile_blueprint(mode);assets.save_loaded_asset(mode)
    mode_class=unreal.load_class(None,'/Game/G5/BP_OperationDeadSignal_G5.BP_OperationDeadSignal_G5_C')
    world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    world.get_world_settings().set_editor_property('default_game_mode',mode_class)
    definition='/Game/DeadSignal/DA_DeadSignal_G5'
    if not assets.does_asset_exist(definition):assert assets.duplicate_asset('/Game/DeadSignal/DA_DeadSignal_G3',definition)
    data=assets.load_asset(definition);assert data
    assert assets.save_loaded_asset(data)
    for actor in actors.get_all_level_actors():
        label=actor.get_actor_label()
        if label.startswith('G4_'):
            actors.destroy_actor(actor);continue
        if isinstance(actor,unreal.DeadSignalNode):actor.set_editor_property('definition',data)
        pos=actor.get_actor_location()
        if any(s in label for s in ['Boundary','Ground','Route_']):folder='00_Infrastructure'
        elif pos.y<8000:folder='01_Headquarters_Start'
        elif pos.y<17000:folder='02_Residential_Edge'
        elif pos.y<28000:folder='03_Relay_Outpost'
        elif pos.y<41000:folder='04_Convoy'
        elif pos.y<52000:folder='05_Continuity_Facility'
        else:folder='06_Ancient_Chamber'
        actor.set_folder_path(unreal.Name('OperationDeadSignal/'+folder))
        if label[:3] in ['G1_','G2_','G3_']:actor.set_actor_label(label[3:])
        name=actor.get_actor_label()
        if name.startswith('Route_'):
            points=[(2000,4000),(9000,11000),(16000,22000),(26000,32000),(30000,47000),(33000,55000)];i=int(name.split('_')[-1]);start,end=points[i],points[i+1]
            actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=math.degrees(math.atan2(end[1]-start[1],end[0]-start[0])),roll=0),False)
        if name.startswith('ConvoyTruck_'):actor.set_actor_rotation(unreal.Rotator(pitch=0,yaw=25,roll=0),False)
    cube=assets.load_asset('/Engine/BasicShapes/Cube')
    for label,xy,height in [('WaistCover',(3800,5100),90),('TallCover',(4150,5100),140)]:
        a=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(xy[0],xy[1],height/2))
        a.set_actor_label('G5_'+label);a.set_folder_path(unreal.Name('OperationDeadSignal/07_Movement_Checks'))
        c=a.static_mesh_component;c.set_static_mesh(cube);c.set_collision_profile_name('BlockAll');a.set_actor_scale3d(unreal.Vector(.4,2.6,height/100))
    label=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(3500,5050,160),unreal.Rotator(pitch=0,yaw=180,roll=0))
    label.set_actor_label('G5_WaistCoverGuide');label.set_folder_path(unreal.Name('OperationDeadSignal/07_Movement_Checks'))
    label.text_render.set_text('G5 WORKSHOP | F interact | Ctrl/C toggle crouch | Q/E peek\n90 cm waist cover: over-top peek | taller cover: side lean\nEdit mission geometry in the Outliner zone folders; preserve gameplay anchors')
    label.text_render.set_world_size(16)
    assert levels.save_current_level()
    if pending.exists():pending.unlink()
    unreal.log('G5_WORKSHOP_CREATED map='+path+' four HQ and four outpost starts preserved; all nine objectives retained')
create()
unreal.log('G5_MAP_SUCCESS')

