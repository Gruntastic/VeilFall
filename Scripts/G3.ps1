param([ValidateSet('Build','Map','Test','Package','Editor','Host','Join','Bootstrap')][string]$Action='Build',[string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
$engine=Get-Content "$EngineRoot/Engine/Build/Build.version" -Raw | ConvertFrom-Json
if($engine.MajorVersion -ne 5 -or $engine.MinorVersion -ne 8){throw 'Requires audited UE 5.8 line'}
$root=Split-Path -Parent $PSScriptRoot
$project=Join-Path $root 'VeilFallG0.uproject'
$exe=Join-Path $root 'Builds/VF-G3-20261003-A/Windows/VeilFallG0.exe'
switch($Action){
 Build {& "$PSScriptRoot/G0.ps1" Build -EngineRoot $EngineRoot;return}
 Map {& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project -run=pythonscript "-script=$root/Content/Python/create_g3.py" -unattended -nullrhi "-abslog=$root/Docs/Logs/G3-Map.log"}
 Test {& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project -unattended -nullrhi -nosplash '-ExecCmds=Automation RunTests VeilFallG0.' '-TestExit=Automation Test Queue Empty' "-abslog=$root/Docs/Logs/G3-Tests.log";if($LASTEXITCODE -ne 0){throw 'Engine test process failed'};if(@(Select-String -LiteralPath "$root/Docs/Logs/G3-Tests.log" -SimpleMatch -Pattern 'Result={Success}').Count -ne 15 -or (Select-String -LiteralPath "$root/Docs/Logs/G3-Tests.log" -SimpleMatch -Pattern 'Result={Fail}' -Quiet)){throw 'Expected fifteen passing current-source engine tests'}}
 Package {$stage=Join-Path $env:LOCALAPPDATA ('VeilFallG3/Stage/'+[guid]::NewGuid().ToString());& "$EngineRoot/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage "-stagingdirectory=$stage" -pak -archive "-archivedirectory=$root/Builds/VF-G3-20261003-A/Windows" -utf8output}
 Editor {& "$PSScriptRoot/MemoryProfile.ps1" Editor -EngineRoot $EngineRoot;return}
 Host {& "$PSScriptRoot/MemoryProfile.ps1" Host;return}
 Join {& "$PSScriptRoot/MemoryProfile.ps1" Join;return}
 Bootstrap {foreach($step in @('Build','Map','Test','Package')){& $PSCommandPath $step -EngineRoot $EngineRoot};return}
}
if($LASTEXITCODE -ne 0){throw "$Action failed: $LASTEXITCODE"}
if($Action -eq 'Map' -and !(Select-String -LiteralPath "$root/Docs/Logs/G3-Map.log" -SimpleMatch -Pattern G3_MAP_SUCCESS -Quiet)){throw 'G3 asset creation marker missing'}





