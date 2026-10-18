"""VEILFALL stand-in weapons: shotgun, machine gun, sniper rifle, grenade (fictional, generic shapes).

Run: blender --background --factory-startup --python Scripts/Blender/export_vf_weapons.py
Writes ArtSource/VeilFall/Weapons/TEMP_*.blend + .fbx and Preview.png. Re-runnable (overwrites its own outputs).
Conventions follow Docs/VeilFall/StandInWeaponGuide.md and the project's proven Blender pipeline: metres, model the
barrel along Blender -Y (exported -Y forward, Unreal imports with force-front-X so the barrel ends up on +X), top +Z,
pivot at the pistol grip (grenade: centre). SOCKET_* empties become Unreal sockets (Muzzle, Grip_L, Sight, Mag, Eject).
"""
from pathlib import Path
import bpy, math, json

root = Path(__file__).resolve().parents[2]
out = root / 'ArtSource' / 'VeilFall' / 'Weapons'
out.mkdir(parents=True, exist_ok=True)
manifest = []

def scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    bpy.context.scene.unit_settings.system = 'METRIC'; bpy.context.scene.unit_settings.scale_length = 1
    return []

def material(name, color, metal=0.0, rough=.6):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    m.diffuse_color = (*color, 1); m.use_nodes = True
    bs = m.node_tree.nodes.get('Principled BSDF')
    bs.inputs['Base Color'].default_value = (*color, 1); bs.inputs['Metallic'].default_value = metal; bs.inputs['Roughness'].default_value = rough
    return m

def _finish(o, name, mat, parts, rot=None):
    o.name = name
    if rot: o.rotation_euler = [math.radians(a) for a in rot]
    bpy.ops.object.transform_apply(location=False, rotation=True, scale=True)
    o.data.materials.append(mat); parts.append(o); return o

def box(parts, name, loc, size, mat, rot=None):
    bpy.ops.mesh.primitive_cube_add(size=1, location=loc); o = bpy.context.object; o.scale = size
    return _finish(o, name, mat, parts, rot)

def cyl(parts, name, loc, radius, depth, mat, axis='Y', verts=16, rot=None):
    r = {'Y': (math.pi / 2, 0, 0), 'X': (0, math.pi / 2, 0), 'Z': (0, 0, 0)}[axis]
    bpy.ops.mesh.primitive_cylinder_add(vertices=verts, radius=radius, depth=depth, location=loc, rotation=r)
    o = bpy.context.object; bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    return _finish(o, name, mat, parts, rot)

def ball(parts, name, loc, radius, mat, scale=(1, 1, 1), segs=16):
    bpy.ops.mesh.primitive_uv_sphere_add(segments=segs, ring_count=max(6, segs // 2), radius=radius, location=loc)
    o = bpy.context.object; o.scale = scale
    return _finish(o, name, mat, parts)

def ring(parts, name, loc, major, minor, mat, rot=(0, 0, 0)):
    bpy.ops.mesh.primitive_torus_add(major_radius=major, minor_radius=minor, major_segments=16, minor_segments=6, location=loc, rotation=[math.radians(a) for a in rot])
    o = bpy.context.object; bpy.ops.object.transform_apply(location=False, rotation=True, scale=False)
    return _finish(o, name, mat, parts)

def join(parts, name):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts: o.select_set(True)
    bpy.context.view_layer.objects.active = parts[0]; bpy.ops.object.join(); o = bpy.context.object; o.name = name
    bpy.ops.object.transform_apply(location=True, rotation=True, scale=True)
    bev = o.modifiers.new('Edge bevel', 'BEVEL'); bev.width = .0015; bev.segments = 1; bev.limit_method = 'ANGLE'
    bpy.ops.object.modifier_apply(modifier=bev.name)
    bpy.ops.object.mode_set(mode='EDIT'); bpy.ops.mesh.select_all(action='SELECT'); bpy.ops.uv.smart_project(island_margin=.02); bpy.ops.object.mode_set(mode='OBJECT')
    assert o.data.uv_layers.active and not o.data.validate()
    return o

def sockets(o, spots):
    made = []
    for n, loc in spots.items():
        e = bpy.data.objects.new('SOCKET_' + n, None); e.empty_display_size = .02; e.location = loc
        bpy.context.collection.objects.link(e); e.parent = o; made.append(e)
    return made

def export(o, name, spots):
    ems = sockets(o, spots)
    bpy.ops.object.select_all(action='DESELECT'); o.select_set(True)
    for e in ems: e.select_set(True)
    bpy.context.view_layer.objects.active = o
    bpy.ops.wm.save_as_mainfile(filepath=str(out / (name + '.blend')))
    bpy.ops.export_scene.fbx(filepath=str(out / (name + '.fbx')), use_selection=True, object_types={'MESH', 'EMPTY'}, axis_forward='-Y', axis_up='Z',
                             apply_unit_scale=True, add_leaf_bones=False, mesh_smooth_type='FACE', bake_anim=False)
    tris = sum(len(p.vertices) - 2 for p in o.data.polygons)
    assert tris < 15000, (name, tris)
    manifest.append(dict(name=name, triangles=tris, materials=[m.name for m in o.data.materials], cm=[round(d * 100, 1) for d in o.dimensions],
                         sockets=sorted(spots), license='Original procedural project source; no third-party inputs',
                         forward='Blender -Y; Unreal import force-front-X; pivot at pistol grip' if name != 'TEMP_Grenade' else 'Centre pivot'))
    print('VF_WEAPON_EXPORTED', name, tris, [round(d * 100, 1) for d in o.dimensions])

def grip(p, mat, y=0.0):
    box(p, 'Grip', (0, y + .015, -.045), (.032, .045, .11), mat, rot=(18, 0, 0))
    box(p, 'TriggerGuard', (0, y - .035, -.012), (.012, .065, .008), mat)
    box(p, 'GuardFront', (0, y - .066, .0), (.012, .008, .03), mat)

# ------------------------------------------------------------------ shotgun: pump action, ~100 cm
p = scene(); steel = material('VF_Steel', (.03, .033, .036), .85, .35); poly = material('VF_Polymer', (.07, .065, .055), 0, .75)
box(p, 'Receiver', (0, -.10, .045), (.042, .22, .07), steel)
cyl(p, 'Barrel', (0, -.47, .068), .011, .52, steel)
cyl(p, 'MagTube', (0, -.43, .038), .012, .44, steel)
cyl(p, 'Pump', (0, -.40, .042), .022, .20, poly, verts=12)
for i in range(5): cyl(p, 'PumpRib', (0, -.33 - i * .035, .042), .024, .008, poly, verts=12)
cyl(p, 'MagCap', (0, -.655, .038), .014, .02, steel)
ball(p, 'Bead', (0, -.725, .084), .004, steel, segs=8)
box(p, 'Rib', (0, -.47, .0805), (.008, .50, .004), steel)
grip(p, poly)
box(p, 'StockNeck', (0, .07, .045), (.036, .12, .055), poly)
box(p, 'Stock', (0, .22, .025), (.042, .20, .11), poly, rot=(-4, 0, 0))
box(p, 'ButtPad', (0, .328, .018), (.044, .022, .125), material('VF_Rubber', (.02, .02, .02), 0, .9), rot=(-4, 0, 0))
export(join(p, 'TEMP_Shotgun'), 'TEMP_Shotgun', {'Muzzle': (0, -.73, .068), 'Grip_L': (0, -.40, .02), 'Sight': (0, .06, .105), 'Mag': (0, -.12, .01), 'Eject': (.022, -.10, .055)})

# ------------------------------------------------------------------ machine gun: belt-fed, folded bipod, ~112 cm
p = scene(); steel = material('VF_Steel', (.03, .033, .036), .85, .35); poly = material('VF_Polymer', (.06, .07, .06), 0, .75)
box(p, 'Receiver', (0, -.11, .06), (.068, .38, .11), steel)
box(p, 'TopCover', (0, -.10, .127), (.07, .25, .026), steel)
box(p, 'Rail', (0, -.10, .145), (.022, .22, .01), steel)
cyl(p, 'Shroud', (0, -.43, .06), .026, .28, steel, verts=12)
for i in range(6): cyl(p, 'ShroudVent', (.026, -.33 - i * .04, .07), .006, .004, poly, axis='X', verts=8)
cyl(p, 'Barrel', (0, -.70, .06), .014, .34, steel)
cyl(p, 'FlashHider', (0, -.89, .06), .018, .06, steel, verts=12)
box(p, 'FrontPost', (0, -.84, .088), (.006, .01, .03), steel)
box(p, 'Handle', (0, -.42, .135), (.016, .15, .012), poly)
for y in (-.36, -.48): box(p, 'HandlePost', (0, y, .11), (.012, .012, .04), poly)
box(p, 'AmmoBox', (.0, -.15, -.045), (.10, .12, .10), material('VF_OliveDrab', (.11, .13, .07), 0, .8))
box(p, 'BeltFeed', (.04, -.13, .03), (.03, .07, .07), steel)
for side in (-.014, .014): cyl(p, 'BipodLeg', (side, -.50, .022), .006, .26, steel, verts=8)
box(p, 'BipodHinge', (0, -.635, .03), (.04, .02, .02), steel)
grip(p, poly)
box(p, 'Stock', (0, .20, .035), (.05, .28, .10), poly, rot=(-3, 0, 0))
box(p, 'ButtPlate', (0, .345, .03), (.052, .02, .135), poly)
export(join(p, 'TEMP_MachineGun'), 'TEMP_MachineGun', {'Muzzle': (0, -.92, .06), 'Grip_L': (0, -.42, .03), 'Sight': (0, .04, .16), 'Mag': (0, -.15, -.06), 'Eject': (-.034, -.10, .05)})

# ------------------------------------------------------------------ sniper rifle: bolt action, fixed scope, folded bipod, ~120 cm
p = scene(); steel = material('VF_Steel', (.03, .033, .036), .85, .35); poly = material('VF_Polymer', (.10, .095, .075), 0, .75)
box(p, 'Forend', (0, -.37, .03), (.052, .40, .058), poly)
cyl(p, 'Receiver', (0, -.07, .058), .019, .25, steel)
cyl(p, 'Barrel', (0, -.53, .058), .011, .72, steel)
box(p, 'Brake', (0, -.905, .058), (.034, .06, .028), steel)
cyl(p, 'BoltArm', (.03, .03, .058), .005, .05, steel, axis='X', verts=8)
ball(p, 'BoltKnob', (.058, .035, .045), .009, steel, segs=10)
box(p, 'Chassis', (0, -.05, .02), (.05, .32, .05), poly)
box(p, 'Magazine', (0, -.07, -.03), (.036, .08, .07), steel)
cyl(p, 'ScopeTube', (0, -.08, .118), .015, .34, steel)
cyl(p, 'ScopeBell', (0, -.28, .118), .024, .08, steel)
cyl(p, 'ScopeEye', (0, .11, .118), .02, .06, steel)
cyl(p, 'TurretTop', (0, -.07, .14), .011, .022, steel, axis='Z')
cyl(p, 'TurretSide', (.022, -.07, .118), .011, .022, steel, axis='X')
for y in (-.18, .02): box(p, 'ScopeRing', (0, y, .093), (.032, .02, .05), steel)
for side in (-.014, .014): cyl(p, 'BipodLeg', (side, -.46, .0), .005, .24, steel, verts=8)
grip(p, poly)
box(p, 'Stock', (0, .20, .005), (.048, .26, .115), poly, rot=(-3, 0, 0))
box(p, 'CheekRest', (0, .17, .078), (.04, .16, .03), poly)
box(p, 'ButtPad', (0, .34, .0), (.05, .022, .13), material('VF_Rubber', (.02, .02, .02), 0, .9))
export(join(p, 'TEMP_SniperRifle'), 'TEMP_SniperRifle', {'Muzzle': (0, -.94, .058), 'Grip_L': (0, -.33, .0), 'Sight': (0, .17, .118), 'Mag': (0, -.07, -.03), 'Eject': (.02, -.08, .07)})

# ------------------------------------------------------------------ grenade: generic frag, ~11 cm, centre pivot
p = scene(); body = material('VF_OliveDrab', (.11, .13, .07), 0, .8); steel = material('VF_Steel', (.12, .12, .11), .8, .4)
ball(p, 'Body', (0, 0, 0), .031, body, scale=(1, 1, 1.25), segs=20)
for z in (-.018, 0, .018): cyl(p, 'Band', (0, 0, z), .0318 * (1 - abs(z) * 8), .004, body, axis='Z', verts=20)
cyl(p, 'Fuze', (0, 0, .043), .011, .018, steel, axis='Z', verts=12)
cyl(p, 'FuzeCap', (0, 0, .054), .008, .006, steel, axis='Z', verts=12)
box(p, 'Lever', (0, -.024, .03), (.012, .004, .06), steel, rot=(-14, 0, 0))
box(p, 'LeverTop', (0, -.012, .052), (.012, .026, .004), steel)
ring(p, 'PullRing', (.019, .0, .046), .011, .0018, steel, rot=(0, 90, 0))
export(join(p, 'TEMP_Grenade'), 'TEMP_Grenade', {'Grip': (0, 0, 0), 'Top': (0, 0, .056)})

(out / 'Manifest.json').write_text(json.dumps(manifest, indent=2))

# ------------------------------------------------------------------ preview: all four side by side (workbench render)
scene()
y = 0
for i, n in enumerate(['TEMP_Shotgun', 'TEMP_MachineGun', 'TEMP_SniperRifle', 'TEMP_Grenade']):
    with bpy.data.libraries.load(str(out / (n + '.blend'))) as (src, dst): dst.objects = [n]
    o = dst.objects[0]; bpy.context.collection.objects.link(o)
    o.location = (0, 0, -i * .33) if n != 'TEMP_Grenade' else (0, -.15, -.9)
    o.rotation_euler = (0, 0, 0)
sc = bpy.context.scene
cam = bpy.data.objects.new('Cam', bpy.data.cameras.new('Cam')); sc.collection.objects.link(cam)
cam.data.type = 'ORTHO'; cam.data.ortho_scale = 1.5; cam.location = (2.0, -.28, -.45); cam.rotation_euler = (math.pi / 2, 0, math.pi / 2)
sc.camera = cam
sc.render.engine = 'BLENDER_WORKBENCH'; sc.display.shading.light = 'STUDIO'; sc.display.shading.color_type = 'MATERIAL'
sc.display.shading.show_cavity = True; sc.display.shading.show_shadows = False
sc.render.resolution_x = 1600; sc.render.resolution_y = 1150; sc.render.film_transparent = False
sc.world = bpy.data.worlds.new('W'); sc.world.color = (.55, .57, .6)
sc.display.shading.background_type = 'WORLD'
sc.render.filepath = str(out / 'Preview.png'); bpy.ops.render.render(write_still=True)
print('VF_WEAPONS_SUCCESS', len(manifest))
