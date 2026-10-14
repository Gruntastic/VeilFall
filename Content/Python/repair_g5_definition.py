import unreal
from pathlib import Path
marker=Path(unreal.Paths.project_saved_dir())/'G5DefinitionBootstrapCorrected.done'
assert not marker.exists(), 'One-time bootstrap repair already completed'
a=unreal.EditorAssetLibrary
path='/Game/DeadSignal/DA_DeadSignal_G5'
if not a.does_asset_exist(path):assert a.duplicate_asset('/Game/DeadSignal/DA_DeadSignal_G3',path)
d=a.load_asset(path);assert d and len(d.get_editor_property('objectives'))==9
assert a.save_loaded_asset(d)
l=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);assert l.load_level('/Game/DeadSignal/L_DeadSignal_G5_Workshop')
count=0
for node in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
    if isinstance(node,unreal.DeadSignalNode):node.set_editor_property('definition',d);count+=1
assert count==14 and l.save_current_level()
marker.write_text('Persistent G5 mission definition and 14 bindings corrected before manual authoring')
unreal.log('G5_DEFINITION_BOOTSTRAP_CORRECTED_PASS')
