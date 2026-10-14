"""Run using discovered Blender 5.2.2 --background --python this_file."""
from pathlib import Path
import bpy
root=Path(__file__).resolve().parents[2]
dest=root/"ArtSource"/"Benchmarks"
dest.mkdir(parents=True,exist_ok=True)
blend=dest/"ScaleCube.blend"
fbx=dest/"ScaleCube.fbx"
if blend.exists() or fbx.exists():
    raise RuntimeError("Benchmark exists; preserve editable source")
bpy.ops.object.select_all(action="SELECT")
bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system="METRIC"
bpy.context.scene.unit_settings.scale_length=1
bpy.ops.mesh.primitive_cube_add(size=1)
cube=bpy.context.object
cube.name="SM_ScaleCube_1m"
assert tuple(round(x,6) for x in cube.dimensions)==(1,1,1)
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={"MESH"},axis_forward="-Y",axis_up="Z",apply_unit_scale=True,add_leaf_bones=False)
print("G0_BLENDER_EXPORT_SUCCESS",fbx)
