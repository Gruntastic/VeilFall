from pathlib import Path
import bpy, math, json, random, wave, struct
root=Path(__file__).resolve().parents[2]
out=root/'ArtSource'/'G4'
out.mkdir(parents=True,exist_ok=True)
if any(out.glob('*.blend')): raise RuntimeError('Preserve existing G4 source; use existing exports or choose a new source revision')
manifest=[]
def scene():
    bpy.ops.object.select_all(action='SELECT');bpy.ops.object.delete(use_global=False)
    bpy.context.scene.unit_settings.system='METRIC';bpy.context.scene.unit_settings.scale_length=1
    return []
def material(name,color,metal=0,rough=.65):
    m=bpy.data.materials.get(name) or bpy.data.materials.new(name);m.diffuse_color=(*color,1);m.use_nodes=True
    bs=m.node_tree.nodes.get('Principled BSDF');bs.inputs['Base Color'].default_value=(*color,1);bs.inputs['Metallic'].default_value=metal;bs.inputs['Roughness'].default_value=rough
    return m
def box(parts,name,loc,size,mat,bone=None):
    bpy.ops.mesh.primitive_cube_add(size=1,location=loc);o=bpy.context.object;o.name=name;o.scale=size;bpy.ops.object.transform_apply(location=False,rotation=False,scale=True)
    o.data.materials.append(mat)
    if bone: g=o.vertex_groups.new(name=bone);g.add(list(range(len(o.data.vertices))),1,'REPLACE')
    parts.append(o);return o
def join(parts,name):
    bpy.ops.object.select_all(action='DESELECT')
    for o in parts:o.select_set(True)
    bpy.context.view_layer.objects.active=parts[0];bpy.ops.object.join();o=bpy.context.object;o.name=name
    bpy.ops.object.transform_apply(location=True,rotation=True,scale=True)
    bevel=o.modifiers.new('Manufactured edge bevel','BEVEL');bevel.width=.003;bevel.segments=2
    bpy.ops.object.modifier_apply(modifier=bevel.name)
    bpy.ops.object.mode_set(mode='EDIT');bpy.ops.mesh.select_all(action='SELECT');bpy.ops.uv.smart_project(island_margin=.025);bpy.ops.object.mode_set(mode='OBJECT')
    assert o.data.uv_layers.active and not o.data.validate()
    return o
def export(o,name,rig=None):
    bpy.ops.object.select_all(action='DESELECT');o.select_set(True)
    if rig:rig.select_set(True)
    bpy.context.view_layer.objects.active=o
    bpy.ops.wm.save_as_mainfile(filepath=str(out/(name+'.blend')))
    bpy.ops.export_scene.fbx(filepath=str(out/(name+'.fbx')),use_selection=True,object_types={'MESH','ARMATURE'} if rig else {'MESH'},axis_forward='-Y',axis_up='Z',apply_unit_scale=True,add_leaf_bones=False,mesh_smooth_type='FACE',bake_anim=False)
    tris=sum(len(p.vertices)-2 for p in o.data.polygons)
    assert tris<10000
    manifest.append(dict(name=name,triangles=tris,uv_layers=len(o.data.uv_layers),meters=list(o.dimensions),bones=len(rig.data.bones) if rig else 0,license='Original procedural project source; no third-party inputs',forward='Blender -Y; Unreal import force-front-X'))
p=scene();steel=material('VF_BrushedSteel',(.035,.045,.05),.8,.38);polymer=material('VF_Polymer',(.06,.075,.065),0,.72)
box(p,'Receiver',(0,-.02,0),(.09,.35,.14),steel);box(p,'Stock',(0,.28,.02),(.075,.3,.16),polymer);box(p,'Grip',(0,.065,-.12),(.07,.09,.18),polymer);box(p,'Magazine',(0,-.08,-.14),(.065,.09,.19),polymer);box(p,'Forearm',(0,-.26,-.02),(.1,.23,.1),polymer)
bpy.ops.mesh.primitive_cylinder_add(vertices=24,radius=.017,depth=.5,location=(0,-.30,.03),rotation=(math.pi/2,0,0));o=bpy.context.object;o.name='Barrel';o.data.materials.append(steel);p.append(o)
box(p,'RearSight',(0,.04,.09),(.06,.03,.06),steel);box(p,'FrontSight',(0,-.38,.075),(.04,.025,.07),steel)
export(join(p,'Carbine_Original'),'Carbine')
p=scene();concrete=material('VF_Concrete',(.32,.33,.30),0,.88);trim=material('VF_PaintedTrim',(.12,.16,.17),.3,.55)
box(p,'WallPanel',(0,0,1.5),(4,.2,3),concrete)
for x in [-1.96,0,1.96]:box(p,'WallTrim',(x,-.11,1.5),(.08,.02,3),trim)
export(join(p,'Wall_400cm'),'Wall')
p=scene();cloth=material('VF_FieldUniform',(.11,.14,.12),0,.86);armor=material('VF_FieldPlate',(.045,.065,.07),.2,.65);skin=material('VF_Mannequin',(.35,.29,.24),0,.7)
parts=[('pelvis',(0,0,.95),(.32,.23,.24),cloth),('spine',(0,0,1.22),(.40,.25,.34),armor),('head',(0,0,1.68),(.22,.22,.26),skin),('upperarm_l',(.32,0,1.40),(.21,.17,.20),cloth),('lowerarm_l',(.50,0,1.36),(.20,.15,.16),cloth),('hand_l',(.65,0,1.35),(.13,.13,.14),skin),('upperarm_r',(-.32,0,1.40),(.21,.17,.20),cloth),('lowerarm_r',(-.50,0,1.36),(.20,.15,.16),cloth),('hand_r',(-.65,0,1.35),(.13,.13,.14),skin),('thigh_l',(.10,0,.65),(.16,.18,.43),cloth),('calf_l',(.10,0,.28),(.15,.17,.34),cloth),('foot_l',(.10,-.04,.065),(.16,.30,.13),armor),('thigh_r',(-.10,0,.65),(.16,.18,.43),cloth),('calf_r',(-.10,0,.28),(.15,.17,.34),cloth),('foot_r',(-.10,-.04,.065),(.16,.30,.13),armor)]
for bone,loc,size,mat in parts:box(p,bone,loc,size,mat,bone)
mesh=join(p,'Continuity_Enemy_Benchmark');export(mesh,'Enemy_Static')
bpy.ops.object.armature_add();rig=bpy.context.object;rig.name='VF_Enemy_Rig';bpy.ops.object.mode_set(mode='EDIT');bones=rig.data.edit_bones
for b in list(bones):bones.remove(b)
def bone(name,head,tail,parent=None):
    b=bones.new(name);b.head=head;b.tail=tail
    if parent:b.parent=bones[parent]
bone('root',(0,0,0),(0,0,.15));bone('pelvis',(0,0,.85),(0,0,1.03),'root');bone('spine',(0,0,1.03),(0,0,1.45),'pelvis');bone('neck',(0,0,1.45),(0,0,1.55),'spine');bone('head',(0,0,1.55),(0,0,1.81),'neck')
for side,sign in [('l',1),('r',-1)]:
    bone('upperarm_'+side,(sign*.2,0,1.42),(sign*.42,0,1.38),'spine');bone('lowerarm_'+side,(sign*.42,0,1.38),(sign*.59,0,1.35),'upperarm_'+side);bone('hand_'+side,(sign*.59,0,1.35),(sign*.71,0,1.35),'lowerarm_'+side)
    bone('thigh_'+side,(sign*.1,0,.86),(sign*.1,0,.45),'pelvis');bone('calf_'+side,(sign*.1,0,.45),(sign*.1,0,.1),'thigh_'+side);bone('foot_'+side,(sign*.1,0,.1),(sign*.1,-.15,.065),'calf_'+side)
bpy.ops.object.mode_set(mode='OBJECT');mod=mesh.modifiers.new('Editable skeletal binding','ARMATURE');mod.object=rig;mesh.parent=rig
assert all(sum(g.weight for g in v.groups)>.99 for v in mesh.data.vertices)
export(mesh,'Enemy_Rig',rig)
# Original synthetic breathing guide: filtered noise, not a human recording.
rng=random.Random(41005);rate=48000;frames=[];filtered=0
for i in range(rate*2):
    t=i/rate;phase=t%2;envelope=math.sin(math.pi*phase/1.0)**2 if phase<1 else .7*math.sin(math.pi*(phase-1))**2
    filtered=.8*filtered+.2*(rng.random()*2-1);value=int(max(-1,min(1,filtered*envelope*.8))*32767);frames.append(struct.pack('<h',value))
with wave.open(str(out/'HeavyBreathing.wav'),'wb') as wav:wav.setnchannels(1);wav.setsampwidth(2);wav.setframerate(rate);wav.writeframes(b''.join(frames))
(out/'Manifest.json').write_text(json.dumps(manifest,indent=2))
print('G4_BLENDER_SUCCESS',manifest)
