"""Run inside the Game Animation Sample project (UE 5.8) as a commandlet.
Retargets a small set of GASP clips (UEFN mannequin) onto VeilFall's UE5 mannequin
(/Game/Characters/Mannequins, temporarily copied into the sample) and puts them in
/Game/VeilFall/Characters/Family/Anims for copying back into VeilFall. Notifies are stripped
because they point at GASP-only Blueprint notify classes."""
import unreal
A = unreal.EditorAssetLibrary
SRC = '/Game/Characters/UEFN_Mannequin/Animations/'
CLIPS = {
    'Idle/M_Relaxed_Stand_Idle_Loop': 'A_VF_Kid_Idle',
    'Idle/M_Relaxed_Stand_Idle_Break_v01': 'A_VF_Kid_IdleBreak_01',
    'Idle/M_Relaxed_Stand_Idle_Break_v02': 'A_VF_Kid_IdleBreak_02',
    'Idle/M_Relaxed_Stand_Idle_Break_v03': 'A_VF_Kid_IdleBreak_03',
    'Idle/M_Neutral_Stand_Idle_Loop': 'A_VF_Kid_Idle_Alert',
    'Idle/M_Neutral_Crouch_Idle_Loop': 'A_VF_Kid_Crouch_Idle',
    'Walk/M_Relaxed_Walk_Loop_F': 'A_VF_Kid_Walk_F',
    'Run/M_Relaxed_Run_Loop_F': 'A_VF_Kid_Run_F',
    'Run/M_Neutral_Run_Loop_F': 'A_VF_Kid_Run_Alert_F',
}
OUT = '/Game/VeilFall/Characters/Family/Anims/'
src_mesh = A.load_asset('/Game/Characters/UEFN_Mannequin/Meshes/SKM_UEFN_Mannequin')
dst_mesh = A.load_asset('/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple')
rtg = A.load_asset('/Game/Characters/UE5_Mannequins/Rigs/RTG_UEFN_to_UE5_Mannequin')
assert src_mesh and dst_mesh and rtg
anims = [A.find_asset_data(SRC + k) for k in CLIPS]
assert all(a.is_valid() for a in anims), [k for k, a in zip(CLIPS, anims) if not a.is_valid()]
A.make_directory(OUT)
new = unreal.IKRetargetBatchOperation.duplicate_and_retarget(anims, src_mesh, dst_mesh, rtg, '', '', 'VFTMP_', '', OUT, False, False, True)
unreal.log('VF_RETARGET created %d' % len(new))
A.make_directory(OUT)
for src_path, name in CLIPS.items():
    base = src_path.split('/')[-1]
    tmp = OUT + 'VFTMP_' + base
    assert A.does_asset_exist(tmp), tmp
    if A.does_asset_exist(OUT + name): A.delete_asset(OUT + name)
    assert A.rename_asset(tmp, OUT + name), name
    seq = A.load_asset(OUT + name)
    unreal.AnimationLibrary.remove_all_animation_notify_tracks(seq)
    for n in set(unreal.AnimationLibrary.get_animation_notify_event_names(seq)):
        unreal.AnimationLibrary.remove_animation_notify_events_by_name(seq, n)
    # Drop GASP-only references so the clips load cleanly in VeilFall.
    seq.set_editor_property('curve_compression_settings', unreal.load_asset('/Engine/Animation/DefaultAnimCurveCompressionSettings'))
    try: seq.set_editor_property('asset_user_data', [])
    except Exception as e: unreal.log_warning('VF_RETARGET user data: %s' % e)
    A.save_loaded_asset(seq)
    unreal.log('VF_RETARGET %s skeleton=%s' % (name, seq.get_editor_property('skeleton').get_path_name()))
unreal.log('VF_RETARGET_DONE')
