import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
a=unreal.EditorAssetLibrary
tools=unreal.AssetToolsHelpers.get_asset_tools()
folder='/Game/G4/Meshes'
a.make_directory(folder)
for name,source,sk in [('SM_Carbine','Carbine',False),('SM_Wall','Wall',False),('SM_EnemyBenchmark','Enemy_Static',False),('SK_EnemyBenchmark','Enemy_Rig',True)]:
    path=folder+'/'+name
    existing=a.load_asset(path) if a.does_asset_exist(path) else None
    repair=bool(sk and existing and not existing.get_editor_property('skeleton'))
    if not existing or repair:
        t=unreal.AssetImportTask();t.filename=str(root/'ArtSource'/'G4'/(source+'.fbx'));t.destination_path=folder;t.destination_name=name;t.automated=True;t.replace_existing=repair;t.save=True;t.factory=unreal.FbxFactory()
        o=unreal.FbxImportUI();o.automated_import_should_detect_type=False;o.mesh_type_to_import=unreal.FBXImportType.FBXIT_SKELETAL_MESH if sk else unreal.FBXImportType.FBXIT_STATIC_MESH;o.import_mesh=True;o.import_as_skeletal=sk;o.import_materials=False;o.import_textures=False;o.import_animations=False
        data=o.skeletal_mesh_import_data if sk else o.static_mesh_import_data
        data.convert_scene_unit=True;data.force_front_x_axis=True;data.import_uniform_scale=1.0;t.options=o;tools.import_asset_tasks([t])
    mesh=a.load_asset(path);assert mesh,path
    if not sk:
        size=mesh.get_bounds().box_extent*2
        expected={'SM_Carbine':(98,10,35.5),'SM_Wall':(22,400,300),'SM_EnemyBenchmark':(31.5,143,181)}[name]
        assert all(abs(x-y)<1 for x,y in zip((size.x,size.y,size.z),expected)),(name,str(size))
        unreal.log('G4_SCALE_PASS '+name+' '+str(size))
    else:
        assert mesh.get_editor_property('skeleton'), 'No imported skeleton'
        unreal.log('G4_RIG_IMPORTED skeleton='+mesh.get_editor_property('skeleton').get_path_name()+'; basic rigid-weight benchmark; animation quality unapproved')
    if sk:
        assert a.save_loaded_asset(mesh.get_editor_property('skeleton'))
        physics=mesh.get_editor_property('physics_asset')
        if physics: assert a.save_loaded_asset(physics)
    assert a.save_loaded_asset(mesh)
path='/Game/G4/Audio/SW_HeavyBreathing'
if not a.does_asset_exist(path):
    t=unreal.AssetImportTask();t.filename=str(root/'ArtSource'/'G4'/'HeavyBreathing.wav');t.destination_path='/Game/G4/Audio';t.destination_name='SW_HeavyBreathing';t.automated=True;t.save=True;tools.import_asset_tasks([t])
sound=a.load_asset(path);assert sound;sound.set_editor_property('looping',True);assert a.save_loaded_asset(sound)
# Reusable small, original PBR library. Benchmark meshes retain their material slots.
for name,color,metal,rough in [('Metal',(0.07,0.08,0.09),0.8,0.38),('Polymer',(0.025,0.03,0.035),0,0.72),('Concrete',(0.3,0.29,0.26),0,0.85),('Cloth',(0.12,0.15,0.1),0,0.9)]:
    p='/Game/G4/Materials/M_'+name
    m=a.load_asset(p) if a.does_asset_exist(p) else tools.create_asset('M_'+name,'/Game/G4/Materials',unreal.Material,unreal.MaterialFactoryNew())
    if True:
        # Existing materials are preserved on reruns.
        if not unreal.MaterialEditingLibrary.get_num_material_expressions(m):
            c=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant3Vector,-300,0);c.constant=unreal.LinearColor(*color,1);unreal.MaterialEditingLibrary.connect_material_property(c,'',unreal.MaterialProperty.MP_BASE_COLOR)
            for y,v,prop in [(160,metal,unreal.MaterialProperty.MP_METALLIC),(280,rough,unreal.MaterialProperty.MP_ROUGHNESS)]:
                n=unreal.MaterialEditingLibrary.create_material_expression(m,unreal.MaterialExpressionConstant,-300,y);n.r=v;unreal.MaterialEditingLibrary.connect_material_property(n,'',prop)
            unreal.MaterialEditingLibrary.recompile_material(m)
    assert a.save_loaded_asset(m)
for name,mat in [('SM_Carbine','Metal'),('SM_Wall','Concrete'),('SM_EnemyBenchmark','Cloth')]:
    mesh=a.load_asset(folder+'/'+name)
    for i in range(mesh.get_num_sections(0)):
        mesh.set_material(i,a.load_asset('/Game/G4/Materials/M_'+mat))
    a.save_loaded_asset(mesh)
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
map_path='/Game/DeadSignal/L_DeadSignal_G4'
if not a.does_asset_exist(map_path):assert a.duplicate_asset('/Game/DeadSignal/L_DeadSignal_G3',map_path)
assert levels.load_level(map_path)
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/VeilFallG0.TacticalGameMode'))
for actor in actors.get_all_level_actors():
    if actor.get_actor_label().startswith('G4_'):actors.destroy_actor(actor)
def spawn_mesh(label,path,xyz,scale=(1,1,1),yaw=0):
    actor=actors.spawn_actor_from_class(unreal.StaticMeshActor,unreal.Vector(*xyz),unreal.Rotator(pitch=0,yaw=yaw,roll=0));actor.set_actor_label('G4_'+label)
    c=actor.static_mesh_component;c.set_static_mesh(a.load_asset(path));c.set_collision_profile_name('BlockAll');actor.set_actor_scale3d(unreal.Vector(*scale));return actor
def text(label,xyz,value):
    actor=actors.spawn_actor_from_class(unreal.TextRenderActor,unreal.Vector(*xyz),unreal.Rotator(pitch=0,yaw=180,roll=0));actor.set_actor_label('G4_'+label);actor.text_render.set_text(value);actor.text_render.set_world_size(18)
spawn_mesh('WallBenchmark',folder+'/SM_Wall',(4200,4200,0))
spawn_mesh('CarbineBenchmark',folder+'/SM_Carbine',(3800,4500,110))
spawn_mesh('EnemyBenchmark',folder+'/SM_EnemyBenchmark',(4300,4800,0))
spawn_mesh('VehicleCoverProxy','/Engine/BasicShapes/Cube',(4700,5200,65),(3.8,1.8,1.3))
spawn_mesh('TreeCoverProxy','/Engine/BasicShapes/Cylinder',(4200,5700,150),(.65,.65,3))
spawn_mesh('LowWallCover','/Engine/BasicShapes/Cube',(3900,5100,55),(.4,2.6,1.1))
text('Controls',(3900,4000,200),'G4 BENCHMARKS | Shift: sprint (4s) | Ctrl/C: toggle crouch | Q/E: peek near cover\n3 short sprints: temporary heavy breathing | F: interact\nOriginal procedural placeholders: visual approval pending')
text('LowCoverControls',(3600,5050,150),'LOW COVER: Ctrl/C toggle crouch; hold Q/E to pop up; release to drop\nNear an open edge: automatic side lean instead')
assert levels.save_current_level()
unreal.log('G4_IMPORT_MAP_SUCCESS')


