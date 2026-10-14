from pathlib import Path
import bpy
root=Path(__file__).resolve().parents[2]
dest=root/"ArtSource"/"Benchmarks"
dest.mkdir(parents=True,exist_ok=True)
blend=dest/"ForwardMarker.blend"; fbx=dest/"ForwardMarker.fbx"
if blend.exists() or fbx.exists(): raise RuntimeError("Preserve existing marker source")
bpy.ops.object.select_all(action="SELECT"); bpy.ops.object.delete(use_global=False)
bpy.context.scene.unit_settings.system="METRIC"; bpy.context.scene.unit_settings.scale_length=1
mesh=bpy.data.meshes.new("ForwardMarkerMesh")
mesh.from_pydata([(0,-2,0),(-.25,0,0),(.25,0,0),(0,0,.25)],[],[(0,2,1),(0,1,3),(0,3,2),(1,2,3)])
mesh.update()
obj=bpy.data.objects.new("SM_ForwardMarker_BlenderMinusY",mesh); bpy.context.collection.objects.link(obj)
obj.select_set(True); bpy.context.view_layer.objects.active=obj
bpy.ops.wm.save_as_mainfile(filepath=str(blend))
bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={"MESH"},axis_forward="-Y",axis_up="Z",apply_unit_scale=True,add_leaf_bones=False)
print("G0_FORWARD_EXPORT_SUCCESS",fbx)
