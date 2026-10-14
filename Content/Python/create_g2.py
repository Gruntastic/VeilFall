"""Create genuine G2 assets alongside the accepted G1 map."""
import unreal
assets=unreal.EditorAssetLibrary
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
path='/Game/DeadSignal/L_DeadSignal_G2'
if not assets.does_asset_exist(path):
    assert assets.duplicate_asset('/Game/DeadSignal/L_DeadSignal_G1',path)
assert levels.load_level(path)
for actor in actors.get_all_level_actors():
    if actor.get_actor_label().startswith('G2_'): actors.destroy_actor(actor)
assets.make_directory('/Game/Combat')
catalog=[]
for name in ['carbine','shotgun','marksman','pistol']:
    asset_path='/Game/Combat/DA_'+name
    definition=assets.load_asset(asset_path) if assets.does_asset_exist(asset_path) else None
    if not definition:
        factory=unreal.DataAssetFactory()
        factory.set_editor_property('data_asset_class',unreal.load_class(None,'/Script/VeilFallG0.VeilWeaponDefinition'))
        definition=unreal.AssetToolsHelpers.get_asset_tools().create_asset('DA_'+name,'/Game/Combat',None,factory)
    definition.set_editor_property('spec',unreal.CombatDataLibrary.default_weapon_spec('weapon.'+name))
    assert assets.save_loaded_asset(definition)
    catalog.append(definition)
for i,action in enumerate([unreal.CombatBenchAction.PRIMARY_SWAP,unreal.CombatBenchAction.ARMOR_CYCLE,unreal.CombatBenchAction.RESTOCK,unreal.CombatBenchAction.DOWN_SELF]):
    bench=actors.spawn_actor_from_class(unreal.CombatLocker,unreal.Vector(2400+i*130,4300,80),unreal.Rotator(pitch=0.0,yaw=180.0,roll=0.0))
    bench.set_actor_label('G2_Bench_'+str(i));bench.set_editor_property('action',action);bench.set_editor_property('catalog',catalog)
for i,armor in enumerate([unreal.VeilArmor.LIGHT,unreal.VeilArmor.MEDIUM,unreal.VeilArmor.HEAVY]):
    target=actors.spawn_actor_from_class(unreal.CombatTarget,unreal.Vector(2500+i*180,4800,100),unreal.Rotator(pitch=0.0,yaw=-90.0,roll=0.0))
    target.set_actor_label('G2_Target_'+str(i));target.set_editor_property('armor_preset',armor);target.set_editor_property('stable_id','training.target.'+str(i))
    target.get_editor_property('label').set_world_size(8.0)
    rotation=target.get_actor_rotation()
    assert abs(rotation.pitch)<0.01 and abs(rotation.roll)<0.01 and abs(rotation.yaw+90)<0.01
    unreal.log('G2_TARGET_ROTATION '+str(rotation))
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
world.get_world_settings().set_editor_property('default_game_mode',unreal.load_class(None,'/Script/VeilFallG0.CombatGameMode'))
assert levels.save_current_level()
assert len([a for a in actors.get_all_level_actors() if isinstance(a,unreal.CombatTarget)])==3
unreal.log('G2_MAP_SUCCESS weapon_assets=4 targets=3 benches=4')




