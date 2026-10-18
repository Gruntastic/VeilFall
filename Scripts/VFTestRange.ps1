param([ValidateSet('Map','CoverTest','Editor','Play')][string]$Action='Play',[string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
# VEILFALL developer test range (L_VF_TestRange). Map = (re)generate it; CoverTest = headless cover self-test (pass/fail on VF_COVER_TEST_PASS);
# Editor = open it in the editor; Play = standalone game window on it. Build the C++ first with Scripts/VFIntro.ps1 Build.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot;$project=Join-Path $root 'VeilFallG0.uproject';$map='/Game/VeilFall/Dev/L_VF_TestRange'
$cmd="$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe";New-Item -ItemType Directory -Force "$root/Docs/Logs"|Out-Null
function Need($log,$marker,$what){if(!(Select-String -LiteralPath $log -SimpleMatch -Pattern $marker -Quiet)){throw "$what failed: see $log"}}
switch($Action){
 Map {& $cmd $project -run=pythonscript "-script=$root/Content/Python/create_vf_testrange.py" -unattended -nullrhi "-abslog=$root/Docs/Logs/VF-TestRange-Map.log";Need "$root/Docs/Logs/VF-TestRange-Map.log" VF_TESTRANGE_MAP_SUCCESS 'Test range generation'}
 CoverTest {& $cmd $project $map -game -nullrhi -nosound -unattended -VFCoverTest -log "-abslog=$root/Docs/Logs/VF-CoverTest.log";Need "$root/Docs/Logs/VF-CoverTest.log" VF_COVER_TEST_PASS 'Cover self-test'}
 Editor {Start-Process "$EngineRoot/Engine/Binaries/Win64/UnrealEditor.exe" -ArgumentList "`"$project`" $map"}
 Play {Start-Process "$EngineRoot/Engine/Binaries/Win64/UnrealEditor.exe" -ArgumentList "`"$project`" $map -game -windowed -ResX=1600 -ResY=900 -log"}
}
