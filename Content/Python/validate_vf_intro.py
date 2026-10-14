"""Checks L_VF_IntroProperty has every anchor the intro mission script needs. Prints VF_INTRO_VALIDATE_PASS on success.
Counts are minimums, so adding encounters or props never breaks validation."""
import unreal
assets = unreal.EditorAssetLibrary
MAP = '/Game/VeilFall/World/IntroProperty/L_VF_IntroProperty'
assert unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).load_level(MAP), 'map missing'
all_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
problems = []
def of(cls): return [a for a in all_actors if isinstance(a, cls)]
markers = {str(a.get_editor_property('marker_id')) for a in of(unreal.VFMarker)}
need_markers = ['player_start', 'player_kitchen', 'player_shop', 'player_vault', 'player_panel', 'player_vault_front', 'player_monitors', 'player_port', 'player_baydoor', 'player_east',
    'wife_kitchen', 'wife_living', 'wife_vault', 'wife_vault_in', 'wife_baydoor', 'wife_shop', 'wife_hall', 'wife_port', 'wife_east', 'wife_kids', 'oldest_living', 'middle_room', 'youngest_living',
    'sheriff_shop', 'mayor_shop', 'nathan_shop', 'nathan_monitors', 'sheriff_port', 'nathan_port', 'mayor_cover', 'defend_point', 'truck_park', 'pa_source', 'cam_gate', 'route_column_01',
    'w2_stop_1', 'w2_stop_2', 'w4_stop_1', 'w4_stop_2', 'vg_w_start', 'vg_w_stop', 'vg_e_start', 'vg_e_stop', 'ncd_front', 'aa_origin', 'vg_meet', 'evac_1', 'evac_2']
need_markers += ['hallway_%d' % i for i in (1, 2, 3)] + ['vault_%d' % i for i in (1, 2, 3)] + ['kids_shop_%d' % i for i in (1, 2, 3)]
need_markers += ['ncd_gate_%d' % i for i in (1, 2, 3)] + ['ncd_halt_%d' % i for i in (1, 2, 3)]
problems += ['missing marker ' + m for m in need_markers if m not in markers]
for prefix, n in [('route_truck_', 3), ('route_ncd_', 1), ('gate_t_', 4), ('column_t_', 4), ('w1_', 3), ('route_wave_', 2), ('w2_tree_', 1), ('w3_', 2), ('w4_tree_', 1), ('route_convoy_', 3), ('scuttle_', 3)]:
    if len([m for m in markers if m.startswith(prefix)]) < n: problems.append('need at least %d markers %s*' % (n, prefix))
items = {str(a.get_editor_property('interaction_id')) for a in of(unreal.VFInteractable)}
for i in ['vault.keypad', 'panel.road', 'panel.bay', 'panel.intercom', 'shop.monitors', 'home.tv', 'equip.rifle', 'equip.sidearm', 'equip.armor', 'equip.ammo', 'equip.radio']:
    if i not in items: problems.append('missing interaction ' + i)
props = {str(a.get_editor_property('prop_id')) for a in of(unreal.VFAnimatedProp)}
problems += ['missing prop ' + p for p in ['vault_door', 'defense_wall', 'gate', 'bay1'] if p not in props]
roles = {str(a.get_editor_property('role_id')) for a in of(unreal.VFIntroNPC)}
problems += ['missing family member ' + r for r in ['wife', 'son_oldest', 'son_middle', 'son_youngest'] if r not in roles]
if len(of(unreal.VFMissionController)) != 1: problems.append('need exactly one mission controller')
if len(of(unreal.VFScriptedDamageZone)) < 1: problems.append('no scripted damage zones')
if not of(unreal.NavMeshBoundsVolume): problems.append('no navigation bounds')
covers = {}
for c in of(unreal.VFCoverPoint): g = str(c.get_editor_property('group')); covers[g] = covers.get(g, 0) + 1
problems += ['cover group %s needs 4+' % g for g in ['drive', 'west', 'east', 'vanguard'] if covers.get(g, 0) < 4]
dt = assets.load_asset('/Game/VeilFall/Data/DT_VF_IntroDialogue')
beats = set(unreal.DataTableFunctionLibrary.get_data_table_column_as_string(dt, 'Beat')) if dt else set()
need_beats = ('opening talk_wife talk_son_oldest talk_son_middle talk_son_youngest home_window_1 home_window_2 home_photo home_pantry home_scanner home_tv home_drawing '
    'shop_workbench shop_monitors alert_reaction vault_code vault_wrong_code vault_open equip_wife equip_rifle equip_sidearm equip_armor equip_ammo equip_radio equip_medical '
    'equip_backpack equip_supplies equip_done locals_incoming locals_standoff locals_brief locals_nathan_aside panel_explosives children_go kids_in_vault radio_handoff vault_sealed '
    'ncd_engines ncd_cameras ncd_demand ncd_reveal ncd_refusal column_advancing defense_armed defense_now after_blast wave1 wave2 wave3 wave4 wave4_radio bark_grenade '
    'vanguard_hit vanguard_radio commander_reveal family_reunion evac_order scuttle_wife scuttle_after').split()
problems += ['dialogue beat missing: ' + b for b in need_beats if b not in beats]
for p in problems: unreal.log_error('VF_INTRO_VALIDATE ' + p)
unreal.log('VF_INTRO_VALIDATE markers=%d interactions=%d covers=%s beats=%d problems=%d' % (len(markers), len(items), covers, len(beats), len(problems)))
if not problems: unreal.log('VF_INTRO_VALIDATE_PASS')
