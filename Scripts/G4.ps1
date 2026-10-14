param([ValidateSet('Build','Import','Test','Package','Editor','Host','Join','Bootstrap')][string]$Action='Host',[string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8',[switch]$FullQuality)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$project=Join-Path $root 'VeilFallG0.uproject'
$v=Get-Content "$EngineRoot/Engine/Build/Build.version" -Raw|ConvertFrom-Json
if($v.MajorVersion-ne 5-or $v.MinorVersion-ne 8){throw 'Requires audited UE 5.8 line'}
$exe=Join-Path $root 'Builds/VF-G4-20261005-E/Windows/VeilFallG0/Binaries/Win64/VeilFallG0.exe'
switch($Action){
 Build {& "$PSScriptRoot/G0.ps1" Build -EngineRoot $EngineRoot;return}
 Import {& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project -run=pythonscript "-script=$root/Content/Python/create_g4.py" -unattended -nullrhi "-abslog=$root/Docs/Logs/G4-Import.log";if(!(Select-String -LiteralPath "$root/Docs/Logs/G4-Import.log" -SimpleMatch -Pattern G4_IMPORT_MAP_SUCCESS -Quiet)){throw 'Missing import/map success marker'}}
 Test {& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project -unattended -nullrhi -nosplash '-ExecCmds=Automation RunTests VeilFallG0.' '-TestExit=Automation Test Queue Empty' "-abslog=$root/Docs/Logs/G4-Tests.log";if(@(Select-String -LiteralPath "$root/Docs/Logs/G4-Tests.log" -SimpleMatch -Pattern 'Result={Success}').Count-ne 15-or(Select-String -LiteralPath "$root/Docs/Logs/G4-Tests.log" -SimpleMatch -Pattern 'Result={Fail}' -Quiet)){throw 'Expected fifteen passing engine tests'}}
 Package {if(Test-Path "$root/Builds/VF-G4-20261005-E/Windows/VeilFallG0.exe"){throw 'Preserve existing candidate; choose a new build ID for repackaging'};$stage=Join-Path $env:LOCALAPPDATA ('VeilFallG4/Stage/'+[guid]::NewGuid());& "$EngineRoot/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage "-stagingdirectory=$stage" -pak -archive "-archivedirectory=$root/Builds/VF-G4-20261005-E/Windows" -utf8output}
 Bootstrap {foreach($step in @('Build','Import','Test','Package')){& $PSCommandPath $step -EngineRoot $EngineRoot};return}
 default {
  $url='/Game/DeadSignal/L_DeadSignal_G4?listen';if($Action-eq 'Join'){$url='127.0.0.1'}
  $args=$url+' -dx12 -windowed -ResX=1280 -ResY=720 -ExecCmds="t.MaxFPS 60"'
  if(!$FullQuality){$args=$url+' -dx12 -windowed -ResX=1280 -ResY=720 -ExecCmds="sg.ViewDistanceQuality 1,sg.AntiAliasingQuality 2,sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,sg.PostProcessQuality 1,sg.TextureQuality 1,sg.EffectsQuality 1,sg.FoliageQuality 1,sg.ShadingQuality 1,t.MaxFPS 60" -ini:Engine:[TextureStreaming]:PoolSizeVRAMPercentage=0,[SystemSettings]:r.Streaming.PoolSize=768,[SystemSettings]:r.RenderTargetPoolMin=200,[SystemSettings]:r.ScreenPercentage=85'}
  if($Action-eq 'Editor'){$exe="$EngineRoot/Engine/Binaries/Win64/UnrealEditor.exe";$args='"'+$project+'" '+$args.Replace('?listen','')}
  if(!(Test-Path $exe)){throw "Missing executable $exe"};Start-Process $exe -ArgumentList $args -WindowStyle Hidden;return
 }
}
if($LASTEXITCODE-ne 0){throw "$Action failed: $LASTEXITCODE"}






