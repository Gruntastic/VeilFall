"""Apply ArtSource/VeilFall/Audio/WeaponSoundAssignments.json to /Game/VeilFall/Audio/DA_VF_WeaponSounds.

Each entry is "weapon.<id>.<fire|npc_fire|reload|equip>" or "grenade.<throw|bounce|explosion>" -> asset path under /Game/VeilFall/Audio
(npc_fire takes a list). Only listed slots change. Run after the WAVs are imported: Scripts/VFTestRange.ps1 Sounds
"""
import unreal, json
from pathlib import Path

assets = unreal.EditorAssetLibrary
table = json.loads((Path(unreal.Paths.project_dir()) / 'ArtSource' / 'VeilFall' / 'Audio' / 'WeaponSoundAssignments.json').read_text())
da = assets.load_asset('/Game/VeilFall/Audio/DA_VF_WeaponSounds'); assert da, 'run Scripts/VFTestRange.ps1 Weapons first'
def snd(p):
    a = assets.load_asset('/Game/VeilFall/Audio/' + p); assert a, 'missing sound ' + p; return a
weapons = dict(da.get_editor_property('weapons'))
for key, value in table.items():
    if key.startswith('_'): continue
    parts = key.split('.')
    if parts[0] == 'grenade':
        da.set_editor_property('grenade_' + parts[1], snd(value))
    else:
        wid, slot = unreal.Name('.'.join(parts[:2])), parts[2]
        s = next((v for k, v in weapons.items() if str(k) == str(wid)), None) or unreal.VFWeaponSoundSet()
        s.set_editor_property(slot, [snd(v) for v in value] if slot == 'npc_fire' else snd(value))
        weapons = {k: v for k, v in weapons.items() if str(k) != str(wid)}; weapons[wid] = s
    unreal.log('VF_SOUND_ASSIGN %s -> %s' % (key, value))
da.set_editor_property('weapons', weapons)
assets.save_loaded_asset(da)
unreal.log('VF_SOUND_ASSIGN_SUCCESS entries=%d' % len([k for k in table if not k.startswith('_')]))
