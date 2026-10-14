import unreal,math
from pathlib import Path
root=Path(unreal.Paths.project_saved_dir());marker=root/'G5RouteBootstrapCorrected.done'
assert not marker.exists(), 'One-time bootstrap repair already applied; preserve designer changes'
levels=unreal.get_editor_subsystem(unreal.LevelEditorSubsystem);actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
assert levels.load_level('/Game/DeadSignal/L_DeadSignal_G5_Workshop')
points=[(2000,4000),(9000,11000),(16000,22000),(26000,32000),(30000,47000),(33000,55000)]
count=0
for a in actors.get_all_level_actors():
    name=a.get_actor_label()
    if name.startswith('Route_'):
        i=int(name.split('_')[-1]);start,end=points[i],points[i+1]
        yaw=math.degrees(math.atan2(end[1]-start[1],end[0]-start[0]));a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=yaw,roll=0),False);count+=1
    if name.startswith('ConvoyTruck_'):a.set_actor_rotation(unreal.Rotator(pitch=0,yaw=25,roll=0),False)
assert count==5;assert levels.save_current_level();marker.write_text('Applied to newly created G5 workshop before human authoring; prior maps untouched')
unreal.log('G5_ROUTE_BOOTSTRAP_CORRECTION_PASS planarRoutes=5')
