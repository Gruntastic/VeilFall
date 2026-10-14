import unreal
assets=unreal.EditorAssetLibrary
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/DeadSignal/L_DeadSignal_G5_Workshop')
world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
mode=world.get_world_settings().get_editor_property('default_game_mode');assert mode and 'G5' in mode.get_name()
all_actors=actors.get_all_level_actors()
nodes=[a for a in all_actors if isinstance(a,unreal.DeadSignalNode)]
ids=[str(a.get_editor_property('stable_id')) for a in nodes];assert len(ids)==len(set(ids)), 'Duplicate mission stable ID'
definition=assets.load_asset('/Game/DeadSignal/DA_DeadSignal_G5');assert definition
assert len(definition.get_editor_property('objectives'))==9, 'Mission data requires nine objective descriptions'
for node in nodes:assert node.get_editor_property('definition')==definition, 'Mission node definition binding missing'
unreal.log('G5_DEFINITION_VALIDATE_PASS descriptions=9 bindings='+str(len(nodes)))
actions={str(a.get_editor_property('action')) for a in nodes}
required={'hq.brief','edge.arrive','relay.clear','relay.power','relay.establish','convoy.recover','facility.enter','chamber.reveal','hq.debrief','clue.utility','clue.archive'}
assert required<=actions, 'Missing required gameplay anchors: '+str(required-actions)
starts=[a for a in all_actors if isinstance(a,unreal.PlayerStart)]
for tag in ['Headquarters','RelayEstablished']:assert sum(str(a.get_editor_property('player_start_tag'))==tag for a in starts)==4, 'Require four '+tag+' starts'
assert len([a for a in all_actors if isinstance(a,unreal.SquadEnemy)])==6, 'Require existing six encounter guards'
assert len([a for a in all_actors if isinstance(a,unreal.FieldPickup)])==6, 'Require existing six field pickups'
player=assets.load_asset('/Game/G5/BP_Player_G5');assert player
cls=unreal.load_class(None,'/Game/G5/BP_Player_G5.BP_Player_G5_C');defaults=unreal.get_default_object(cls)
assert unreal.get_default_object(mode).get_editor_property('default_pawn_class')==cls, 'Game mode must use editable player Blueprint'
unreal.log('G5_TUNING waist='+str(defaults.get_editor_property('waist_cover_height'))+' gravity='+str(defaults.get_editor_property('jump_gravity_scale'))+' jump='+str(defaults.get_editor_property('jump_speed')))
unreal.log('G5_AUTHORING_VALIDATE_PASS nodes='+str(len(nodes))+' objectives=9 clues=2 starts=8 enemies=6 pickups=6 mode='+mode.get_name())


for actor in all_actors:
    if actor.get_actor_label().startswith('Route_'):unreal.log('G5_ROUTE_ROTATION '+actor.get_actor_label()+' '+str(actor.get_actor_rotation()))

