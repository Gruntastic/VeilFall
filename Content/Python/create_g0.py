import unreal
assets=unreal.EditorAssetLibrary
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
path="/Game/Foundation/L_G0"
if assets.does_asset_exist(path):
    assert levels.load_level(path)
else:
    assets.make_directory("/Game/Foundation")
    assert levels.new_level(path, is_partitioned_world=False)
def spawn(label,cls,xyz,scale=None):
    existing=[a for a in actors.get_all_level_actors() if a.get_actor_label()==label]
    if existing:
        assert len(existing)==1
        return existing[0]
    a=actors.spawn_actor_from_class(cls,unreal.Vector(*xyz))
    assert a
    a.set_actor_label(label)
    if scale:
        a.static_mesh_component.set_static_mesh(unreal.load_asset("/Engine/BasicShapes/Cube.Cube"))
        a.static_mesh_component.set_collision_profile_name("BlockAll")
        a.set_actor_scale3d(unreal.Vector(*scale))
    return a
spawn("G0_Floor",unreal.StaticMeshActor,(0,0,-50),(30,20,1))
spawn("G0_Wall",unreal.StaticMeshActor,(600,250,150),(.3,5,3))
for label,y in zip(("G0_StartA","G0_StartB","G0_StartC","G0_StartD"),(-150,-50,50,150)):
    start=spawn(label,unreal.PlayerStart,(-80,y,100))
    start.set_actor_location(unreal.Vector(-80,y,100),False,False)
spawn("G0_Relay",unreal.load_class(None,"/Script/VeilFallG0.FoundationSwitch"),(150,0,100))
spawn("G0_Target",unreal.StaticMeshActor,(700,-400,100),(.25,1,2))
sun=spawn("G0_Sun",unreal.DirectionalLight,(0,0,500))
sun.set_actor_rotation(unreal.Rotator(-50,-25,0),False)
spawn("G0_Sky",unreal.SkyLight,(0,0,500))
spawn("G0_Atmosphere",unreal.SkyAtmosphere,(0,0,0))
assert levels.save_current_level()
unreal.log("G0_MAP_SUCCESS")
