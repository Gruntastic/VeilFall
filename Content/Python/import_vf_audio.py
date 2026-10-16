"""VEILFALL audio library import: ArtSource/VeilFall/Audio/Library/<Category>/*.wav -> /Game/VeilFall/Audio/<Category>/.

Re-runnable; existing assets are reimported in place. Sources are 48 kHz/16-bit conversions of the licensed D:/SFX packs
(see Docs/VeilFall/AssetLog.md for the source file of each). Run: Scripts/VFIntro.ps1 Audio
"""
import unreal
from pathlib import Path

assets = unreal.EditorAssetLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
SRC = Path(unreal.Paths.project_dir()) / 'ArtSource' / 'VeilFall' / 'Audio' / 'Library'
LOOPING = {'SFX_Vehicle_EngineIdle_Deep', 'SFX_Vehicle_ArmyTruck_IdleInside', 'SFX_Fire_BurningImpacts', 'SFX_Radio_Noise', 'SFX_TV_Noise'}

count = 0
for wav in sorted(SRC.glob('*/*.wav')):
    folder = '/Game/VeilFall/Audio/' + wav.parent.name
    assets.make_directory(folder)
    t = unreal.AssetImportTask()
    t.filename = str(wav); t.destination_path = folder; t.destination_name = wav.stem
    t.automated = True; t.replace_existing = True; t.save = False
    tools.import_asset_tasks([t])
    sw = assets.load_asset(folder + '/' + wav.stem)
    if not sw:
        unreal.log_error('VF_AUDIO_IMPORT_FAILED ' + str(wav)); continue
    sw.set_editor_property('looping', wav.parent.name == 'Ambience' or wav.stem in LOOPING)
    assets.save_loaded_asset(sw)
    count += 1

unreal.log('VF_AUDIO_IMPORT_SUCCESS %d' % count)
