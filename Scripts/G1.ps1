param([ValidateSet('Build','Map','Test','Package','Editor','Host','Join','Bootstrap')][string]$Action='Build',[string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$project=Join-Path $root 'VeilFallG0.uproject'
$engine=Get-Content "$EngineRoot/Engine/Build/Build.version" -Raw | ConvertFrom-Json
if($engine.MajorVersion -ne 5 -or $engine.MinorVersion -ne 8){throw 'Requires audited UE 5.8 line'}
$exe=Join-Path $root 'Builds/VF-G1-20261003-A/Windows/VeilFallG0.exe'
switch($Action){
 Build { & "$PSScriptRoot/G0.ps1" Build -EngineRoot $EngineRoot; return }
 Map { & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project -run=pythonscript "-script=$root/Content/Python/create_g1.py" -unattended -nullrhi "-abslog=$root/Docs/Logs/G1-Map.log" }
 Test {
  $log=Join-Path $root 'Docs/Logs/G1-Tests.log'
  & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project -unattended -nullrhi -nosplash '-ExecCmds=Automation RunTests VeilFallG0.' '-TestExit=Automation Test Queue Empty' "-abslog=$log"
  if($LASTEXITCODE -ne 0){throw 'G1 engine tests failed'}
  if(@(Select-String -LiteralPath $log -Pattern 'Result={Success}' -SimpleMatch).Count -ne 11 -or (Select-String -LiteralPath $log -Pattern 'Result={Fail}' -SimpleMatch -Quiet)){throw 'Expected all eleven current foundation/campaign/combat/coop tests to pass'}
 }
 Package {
  $stage=Join-Path $env:LOCALAPPDATA ('VeilFallG1/Stage/'+[guid]::NewGuid().ToString())
  & "$EngineRoot/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage "-stagingdirectory=$stage" -pak -archive "-archivedirectory=$root/Builds/VF-G1-20261003-A/Windows" -utf8output
 }
 Editor {Start-Process "$EngineRoot/Engine/Binaries/Win64/UnrealEditor.exe" -ArgumentList ('"'+$project+'" /Game/DeadSignal/L_DeadSignal_G1') -WindowStyle Hidden;return}
 Host {Start-Process $exe -ArgumentList '/Game/DeadSignal/L_DeadSignal_G1?listen -log -dx12 -windowed -ResX=1280 -ResY=720' -WindowStyle Hidden;return}
 Join {Start-Process $exe -ArgumentList '127.0.0.1 -log -dx12 -windowed -ResX=1280 -ResY=720' -WindowStyle Hidden;return}
 Bootstrap {foreach($step in @('Build','Map','Test','Package')){& $PSCommandPath $step -EngineRoot $EngineRoot};return}
}
if($LASTEXITCODE -ne 0){throw "$Action failed: $LASTEXITCODE"}
if($Action -eq 'Map' -and !(Select-String -LiteralPath "$root/Docs/Logs/G1-Map.log" -SimpleMatch -Pattern G1_MAP_SUCCESS -Quiet)){throw 'G1 map success marker missing'}


