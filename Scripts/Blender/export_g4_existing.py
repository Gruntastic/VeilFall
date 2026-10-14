"""Export an existing editable G4 .blend into a fresh FBX revision without changing its source."""
import bpy,sys,argparse
from pathlib import Path
p=argparse.ArgumentParser();p.add_argument('--output',required=True)
args=p.parse_args(sys.argv[sys.argv.index('--')+1:])
out=Path(args.output).resolve()
assert not out.exists(), 'Preserve existing export; choose a new revision destination'
assert bpy.data.filepath, 'Open an existing .blend with Blender before invoking this exporter'
out.parent.mkdir(parents=True,exist_ok=True)
bpy.ops.object.select_all(action='DESELECT')
objects=[o for o in bpy.context.scene.objects if o.type in {'MESH','ARMATURE'}]
assert objects, 'No source mesh/rig'
for obj in objects:
    obj.select_set(True)
    if obj.type=='MESH':
        assert obj.data.uv_layers, 'Missing source UV layer'
        assert all(abs(x-1)<.001 for x in obj.scale), 'Apply source scale in Blender before export'
bpy.context.view_layer.objects.active=objects[0]
bpy.ops.export_scene.fbx(filepath=str(out),use_selection=True,object_types={'MESH','ARMATURE'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,add_leaf_bones=False,bake_anim=False,mesh_smooth_type='FACE')
assert out.is_file()
print('G4_EXISTING_SOURCE_EXPORT_SUCCESS',str(out))
