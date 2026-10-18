"""Import the stand-in weapon models (ArtSource/VeilFall/Weapons/TEMP_*.fbx, made by Scripts/Blender/export_vf_weapons.py)
into /Game/VeilFall/Weapons, and create /Game/VeilFall/Audio/DA_VF_WeaponSounds (per-weapon sound table) if it is missing.
Re-runnable: meshes are re-imported in place; existing sound assignments in the data asset are never overwritten.
Run: Scripts/VFTestRange.ps1 Weapons
"""
import unreal
from pathlib import Path

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
ROOT = Path(unreal.Paths.project_dir()).resolve()
SRC = ROOT / 'ArtSource' / 'VeilFall' / 'Weapons'
DEST = '/Game/VeilFall/Weapons'
assets.make_directory(DEST)
# Blender material slot -> greybox material instance (made by create_vf_intro.py).
FINISH = {'VF_Steel': 'Dark', 'VF_Polymer': 'Rock', 'VF_Rubber': 'Dark', 'VF_OliveDrab': 'Pasture'}

def import_mesh(name):
    task = unreal.AssetImportTask()
    task.filename = str(SRC / (name + '.fbx')); task.destination_path = DEST; task.destination_name = name
    task.automated = True; task.replace_existing = True; task.save = True
    task.factory = unreal.FbxFactory()
    o = unreal.FbxImportUI()
    o.set_editor_property('automated_import_should_detect_type', False)
    o.set_editor_property('mesh_type_to_import', unreal.FBXImportType.FBXIT_STATIC_MESH)
    o.set_editor_property('import_mesh', True); o.set_editor_property('import_materials', False); o.set_editor_property('import_textures', False)
    d = o.static_mesh_import_data
    d.set_editor_property('convert_scene_unit', True); d.set_editor_property('force_front_x_axis', True); d.set_editor_property('import_uniform_scale', 1.0)
    d.set_editor_property('auto_generate_collision', False); d.set_editor_property('combine_meshes', True)
    task.options = o
    tools.import_asset_tasks([task])
    mesh = assets.load_asset(DEST + '/' + name); assert mesh, 'import failed: ' + name
    for i, m in enumerate(mesh.static_materials):
        slot = str(m.material_slot_name)
        key = next((k for k in FINISH if slot.startswith(k)), None)
        mi = assets.load_asset('/Game/VeilFall/Materials/TEMP_MI_' + (FINISH[key] if key else 'Dark'))
        mesh.set_material(i, mi)
    b = mesh.get_bounds(); size = b.box_extent * 2
    socks = [n for n in ('Muzzle', 'Grip_L', 'Sight', 'Mag', 'Eject', 'Grip', 'Top') if mesh.find_socket(n)]
    unreal.log('VF_WEAPON_IMPORT %s size_cm=(%.1f, %.1f, %.1f) sockets=%s' % (name, size.x, size.y, size.z, socks))
    if name != 'TEMP_Grenade':
        assert size.x > size.y and size.x > 90, '%s: barrel should run along +X (%s)' % (name, size)
        muzzle = mesh.find_socket('Muzzle'); assert muzzle, name + ' has no Muzzle socket'
        assert muzzle.relative_location.x > 60, '%s: Muzzle should be ahead of the grip (%s)' % (name, muzzle.relative_location)
    assets.save_loaded_asset(mesh)
    return mesh

for n in ['TEMP_Shotgun', 'TEMP_MachineGun', 'TEMP_SniperRifle', 'TEMP_Grenade']:
    import_mesh(n)

# ------------------------------------------------------------------ DA_VF_WeaponSounds: start from the sounds the intro already uses
DA = '/Game/VeilFall/Audio/DA_VF_WeaponSounds'
if not assets.does_asset_exist(DA):
    fac = unreal.DataAssetFactory(); fac.set_editor_property('data_asset_class', unreal.VFWeaponSounds)
    assert tools.create_asset('DA_VF_WeaponSounds', '/Game/VeilFall/Audio', unreal.VFWeaponSounds, fac)
da = assets.load_asset(DA)
def snd(n): return assets.load_asset('/Game/VeilFall/Audio/' + n) if assets.does_asset_exist('/Game/VeilFall/Audio/' + n) else None
weapons = dict(da.get_editor_property('weapons'))
defaults = {
    'weapon.carbine': dict(fire='Weapons/SFX_Gun_Rifle_Shot', reload='Weapons/SFX_Gun_Reload_RifleMag', npc=['Weapons/SFX_Gun_Rifle_NPC_01', 'Weapons/SFX_Gun_Rifle_NPC_02', 'Weapons/SFX_Gun_Rifle_NPC_03']),
    'weapon.marksman': dict(fire='Weapons/SFX_Gun_Rifle_Shot', reload='Weapons/SFX_Gun_Reload_RifleMag'),
    'weapon.pistol': dict(fire='Weapons/SFX_Gun_Pistol_9mm'),
    'weapon.shotgun': {}, 'weapon.mg': {},
}
existing = {str(k) for k in weapons}
for wid, d in defaults.items():
    if wid in existing: continue
    s = unreal.VFWeaponSoundSet()
    if d.get('fire') and snd(d['fire']): s.set_editor_property('fire', snd(d['fire']))
    if d.get('reload') and snd(d['reload']): s.set_editor_property('reload', snd(d['reload']))
    if d.get('npc'): s.set_editor_property('npc_fire', [snd(n) for n in d['npc'] if snd(n)])
    weapons[unreal.Name(wid)] = s
da.set_editor_property('weapons', weapons)
if da.get_editor_property('grenade_explosion') is None and snd('Explosions/SFX_Explosion_Hybrid'):
    da.set_editor_property('grenade_explosion', snd('Explosions/SFX_Explosion_Hybrid'))
assets.save_loaded_asset(da)
unreal.log('VF_WEAPONS_IMPORT_SUCCESS weapons=%d sounds=%s' % (4, sorted(str(k) for k in da.get_editor_property('weapons'))))
