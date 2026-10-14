param([ValidateSet('Build','Map','Test','Package','Editor','Host','Join','Bootstrap')][string]$Action='Build',[string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
$engine=Get-Content "$EngineRoot/Engine/Build/Build.version" -Raw | ConvertFrom-Json
if($engine.MajorVersion -ne 5 -or $engine.MinorVersion -ne 8){throw 'Requires audited UE 5.8 line'}
$root=Split-Path -Parent $PSScriptRoot
$project=Join-Path $root 'VeilFallG0.uproject'
$exe=Join-Path $root 'Builds/VF-G2-20261003-A/Windows/VeilFallG0.exe'
switch($Action){
 Build {& "$PSScriptRoot/G0.ps1" Build -EngineRoot $EngineRoot;return}
 Map {& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project -run=pythonscript "-script=$root/Content/Python/create_g2.py" -unattended -nullrhi "-abslog=$root/Docs/Logs/G2-Map.log"}
 Test {& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project -unattended -nullrhi -nosplash '-ExecCmds=Automation RunTests VeilFallG0.' '-TestExit=Automation Test Queue Empty' "-abslog=$root/Docs/Logs/G2-Tests.log";if($LASTEXITCODE -ne 0){throw 'Engine test process failed'};if(@(Select-String -LiteralPath "$root/Docs/Logs/G2-Tests.log" -SimpleMatch -Pattern 'Result={Success}').Count -ne 11 -or (Select-String -LiteralPath "$root/Docs/Logs/G2-Tests.log" -SimpleMatch -Pattern 'Result={Fail}' -Quiet)){throw 'Expected eleven passing engine tests'}}
 Package {$stage=Join-Path $env:LOCALAPPDATA ('VeilFallG2/Stage/'+[guid]::NewGuid().ToString()); & "$EngineRoot/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage "-stagingdirectory=$stage" -pak -archive "-archivedirectory=$root/Builds/VF-G2-20261003-A/Windows" -utf8output}
 Editor {Start-Process "$EngineRoot/Engine/Binaries/Win64/UnrealEditor.exe" -ArgumentList ('"'+$project+'" /Game/DeadSignal/L_DeadSignal_G2') -WindowStyle Hidden;return}
 Host {Start-Process $exe -ArgumentList '/Game/DeadSignal/L_DeadSignal_G2?listen -log -dx12 -windowed -ResX=1280 -ResY=720' -WindowStyle Hidden;return}
 Join {Start-Process $exe -ArgumentList '127.0.0.1 -log -dx12 -windowed -ResX=1280 -ResY=720' -WindowStyle Hidden;return}
 Bootstrap {foreach($step in @('Build','Map','Test','Package')){& $PSCommandPath $step -EngineRoot $EngineRoot};return}
}
if($LASTEXITCODE -ne 0){throw "$Action failed: $LASTEXITCODE"}
if($Action -eq 'Map' -and !(Select-String -LiteralPath "$root/Docs/Logs/G2-Map.log" -SimpleMatch -Pattern G2_MAP_SUCCESS -Quiet)){throw 'G2 asset creation marker missing'}



