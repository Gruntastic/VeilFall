# Verified benchmark pipeline
Blender 5.2.2 LTS d13f752e3b9c; Unreal 5.8.3 CL 58210709.
Original and failed intermediate source/assets are preserved. Use only the _Validated variants for this benchmark.
- ScaleCube_Validated.blend/.fbx → /Game/Foundation/Benchmarks/SM_ScaleCube_Validated
- ForwardMarker_Validated.blend/.fbx → /Game/Foundation/Benchmarks/SM_ForwardMarker_Validated
Original procedural geometry; no third-party assets or generation credits. Not a production art approval.

Recreate on a fresh checkout without benchmark sources:
1. Run Blender export_cube.py. Expected: 1 m cube source + FBX.
2. Run Blender export_forward.py. Expected: asymmetric Blender -Y-forward marker source + FBX.
3. Run Blender export_validated.py. Expected: preserved UV/smoothing-corrected variants.
4. Run ./Scripts/G0.ps1 ImportBenchmarks. Expected: genuine Unreal static meshes and G0_BENCHMARK_IMPORT_SUCCESS.
The Blender scripts refuse overwrites. On a checkout already containing the sources, use the checked-in _Validated files and run ImportBenchmarks directly; imports preserve existing assets and recheck bounds.

Exact Blender executable:
```powershell
& 'C:/Program Files/Blender Foundation/Blender 5.2/blender.exe' --background --python ./Scripts/Blender/export_validated.py
```
Validated export: selected mesh, meter units, applied rotation/scale, FBX -Y forward/+Z up, apply unit scale, no leaf bones, FACE smoothing, Smart Project UVs.
Unreal legacy FBX factory options: static mesh, import scale 1, convert scene unit true, force front X axis true, no material/texture imports.
Measured cube bounds: 100.000008 × 100.000008 × 100.000000 cm.
Marker center: X 99.999999, Y 0, Z 12.5 cm; extent X 100.000001, Y 25, Z 12.5 cm. Blender -Y maps Unreal +X; +Z remains +Z.
Final commandlet: exit 0, 0 errors, 0 warnings. Logs/BenchmarkImport-Validated.log.
Initial import without force_front_x_axis pointed +Y; retained as SM_ForwardMarker and failed-orientation log. Intermediate XForward had UV/tangent warnings; corrected variants passed without warnings.
No weapon/wall/enemy batching until further collision, shading, materials, rig and performance approvals.
