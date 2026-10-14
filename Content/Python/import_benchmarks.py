import unreal
from pathlib import Path
root=Path(unreal.Paths.project_dir()).resolve()
folder="/Game/Foundation/Benchmarks"
assets=unreal.EditorAssetLibrary
assets.make_directory(folder)
def load_or_import(name,filename):
    path=folder+"/"+name
    if assets.does_asset_exist(path): return assets.load_asset(path)
    task=unreal.AssetImportTask()
    task.filename=str(root/"ArtSource"/"Benchmarks"/filename)
    task.destination_path=folder; task.destination_name=name
    task.automated=True; task.replace_existing=False; task.save=True
    task.factory=unreal.FbxFactory()
    options=unreal.FbxImportUI()
    options.set_editor_property("automated_import_should_detect_type",False)
    options.set_editor_property("mesh_type_to_import",unreal.FBXImportType.FBXIT_STATIC_MESH)
    options.set_editor_property("import_mesh",True)
    options.set_editor_property("import_materials",False)
    options.set_editor_property("import_textures",False)
    options.static_mesh_import_data.set_editor_property("convert_scene_unit",True)
    options.static_mesh_import_data.set_editor_property("force_front_x_axis",True)
    options.static_mesh_import_data.set_editor_property("import_uniform_scale",1.0)
    task.options=options
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
    mesh=assets.load_asset(path)
    assert mesh, "Import failed: "+path
    return mesh
cube=load_or_import("SM_ScaleCube_Validated","ScaleCube_Validated.fbx")
marker=load_or_import("SM_ForwardMarker_Validated","ForwardMarker_Validated.fbx")
size=cube.get_bounds().box_extent*2
assert all(abs(v-100)<.1 for v in (size.x,size.y,size.z)),str(size)
bounds=marker.get_bounds()
unreal.log("G0_SCALE_PASS cm="+str(size))
unreal.log("G0_FORWARD_OBSERVED center="+str(bounds.origin)+" extent="+str(bounds.box_extent))
assert bounds.origin.x>90 and abs(bounds.origin.y)<1 and bounds.origin.z>0,"Blender -Y forward did not map to Unreal +X"
assert bounds.box_extent.x>90 and bounds.box_extent.z<20,"Forward/up marker dimensions disagree"
unreal.log("G0_FORWARD_PASS Blender -Y mapped Unreal +X; Blender +Z remained +Z")
unreal.log("G0_BENCHMARK_IMPORT_SUCCESS")
