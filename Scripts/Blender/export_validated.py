from pathlib import Path
import bpy
root=Path(__file__).resolve().parents[2]
dest=root/"ArtSource"/"Benchmarks"
for name in ("ScaleCube","ForwardMarker"):
    blend=dest/(name+"_Validated.blend"); fbx=dest/(name+"_Validated.fbx")
    if blend.exists() or fbx.exists(): raise RuntimeError("Preserve validated source: "+name)
    bpy.ops.wm.open_mainfile(filepath=str(dest/(name+".blend")))
    meshes=[obj for obj in bpy.context.scene.objects if obj.type=="MESH"]
    assert len(meshes)==1
    obj=meshes[0]; bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True); bpy.context.view_layer.objects.active=obj
    bpy.ops.object.transform_apply(location=False,rotation=True,scale=True)
    bpy.ops.object.mode_set(mode="EDIT"); bpy.ops.mesh.select_all(action="SELECT")
    bpy.ops.uv.smart_project()
    bpy.ops.object.mode_set(mode="OBJECT")
    assert obj.data.uv_layers.active and len(obj.data.uv_layers.active.data)>0
    obj.data.validate()
    bpy.ops.wm.save_as_mainfile(filepath=str(blend))
    bpy.ops.export_scene.fbx(filepath=str(fbx),use_selection=True,object_types={"MESH"},axis_forward="-Y",axis_up="Z",apply_unit_scale=True,add_leaf_bones=False,mesh_smooth_type="FACE")
    print("G0_CLEAN_EXPORT_SUCCESS",name,fbx)
