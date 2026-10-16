param([ValidateSet('Build','Map','Audio','Validate','Autoplay','Editor','Play','All')][string]$Action='Editor',[string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8',[string]$State='')
# VEILFALL intro greybox (L_VF_IntroProperty). Build = C++ editor target; Map = (re)generate the greybox; Audio = import ArtSource/VeilFall/Audio/Library; Validate = layout/data checks;
# Autoplay = headless run of the whole mission (pass/fail on VF_AUTOPLAY_COMPLETE); Editor = open the map; Play = standalone game window on the map (-State=HOLD_SHOP etc. starts at a mission state).
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot;$project=Join-Path $root 'VeilFallG0.uproject';$map='/Game/VeilFall/World/IntroProperty/L_VF_IntroProperty'
$cmd="$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe";New-Item -ItemType Directory -Force "$root/Docs/Logs"|Out-Null
function Need($log,$marker,$what){if(!(Select-String -LiteralPath $log -SimpleMatch -Pattern $marker -Quiet)){throw "$what failed: see $log"}}
switch($Action){
 Build {& "$EngineRoot/Engine/Build/BatchFiles/Build.bat" VeilFallG0Editor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA;if($LASTEXITCODE-ne 0){throw "Build failed: $LASTEXITCODE"}}
 Map {& $cmd $project -run=pythonscript "-script=$root/Content/Python/create_vf_intro.py" -unattended -nullrhi "-abslog=$root/Docs/Logs/VF-Intro-Map.log";Need "$root/Docs/Logs/VF-Intro-Map.log" VF_INTRO_MAP_SUCCESS 'Map generation'}
 Audio {& $cmd $project -run=pythonscript "-script=$root/Content/Python/import_vf_audio.py" -unattended -nullrhi "-abslog=$root/Docs/Logs/VF-Intro-Audio.log";Need "$root/Docs/Logs/VF-Intro-Audio.log" VF_AUDIO_IMPORT_SUCCESS 'Audio import'}
 Validate {& $cmd $project -run=pythonscript "-script=$root/Content/Python/validate_vf_intro.py" -unattended -nullrhi "-abslog=$root/Docs/Logs/VF-Intro-Validate.log";Need "$root/Docs/Logs/VF-Intro-Validate.log" VF_INTRO_VALIDATE_PASS 'Validation'}
 Autoplay {& $cmd $project $map -game -nullrhi -nosound -unattended -VFAutoplay -log "-abslog=$root/Docs/Logs/VF-Intro-Autoplay.log";Need "$root/Docs/Logs/VF-Intro-Autoplay.log" VF_AUTOPLAY_COMPLETE 'Autoplay'}
 Editor {Start-Process "$EngineRoot/Engine/Binaries/Win64/UnrealEditor.exe" -ArgumentList "`"$project`" $map"}
 Play {$extra='';if($State){$extra=" -VFState=$State"};Start-Process "$EngineRoot/Engine/Binaries/Win64/UnrealEditor.exe" -ArgumentList "`"$project`" $map -game -windowed -ResX=1600 -ResY=900 -log$extra"}
 All {foreach($s in 'Build','Map','Validate','Autoplay'){& $PSCommandPath $s -EngineRoot $EngineRoot}}
}
