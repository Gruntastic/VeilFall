param([ValidateSet('Build','Map','ImportBenchmarks','Package','Editor','Host','Join')] [string]$Action='Build',[string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$project=Join-Path $root 'VeilFallG0.uproject'
$v=Get-Content (Join-Path $EngineRoot 'Engine/Build/Build.version') -Raw | ConvertFrom-Json
if($v.MajorVersion -ne 5 -or $v.MinorVersion -ne 8){throw 'Requires audited UE 5.8 line'}
$editor=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
switch($Action){
 ImportBenchmarks { & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project -run=pythonscript "-script=$root/Content/Python/import_benchmarks.py" -unattended -nullrhi "-abslog=$root/Docs/Logs/BenchmarkImport-Validated.log" }
 Build { & "$EngineRoot/Engine/Build/BatchFiles/Build.bat" VeilFallG0Editor Win64 Development "-Project=$project" -WaitMutex -NoHotReloadFromIDE -NoUBA }
 Map { & "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" $project -run=pythonscript "-script=$root/Content/Python/create_g0.py" -unattended -nullrhi "-abslog=$root/Saved/Logs/G0Map.log" }
 Package { $stage=Join-Path $env:LOCALAPPDATA ('VeilFallG0/Stage/'+[guid]::NewGuid().ToString()); & "$EngineRoot/Engine/Build/BatchFiles/RunUAT.bat" BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Development -build -cook -stage "-stagingdirectory=$stage" -pak -archive "-archivedirectory=$root/Builds/VF-G0-20261003-C/Windows" -utf8output }
 Editor { Start-Process $editor -ArgumentList ('"'+$project+'"') -WindowStyle Hidden; return }
 Host { Start-Process "$root/Builds/VF-G0-20261003-C/Windows/VeilFallG0.exe" -ArgumentList '/Game/Foundation/L_G0?listen -log -windowed -ResX=1280 -ResY=720' -WindowStyle Hidden; return }
 Join { Start-Process "$root/Builds/VF-G0-20261003-C/Windows/VeilFallG0.exe" -ArgumentList '127.0.0.1 -log -windowed -ResX=1280 -ResY=720' -WindowStyle Hidden; return }
}
if($LASTEXITCODE -ne 0){throw "$Action failed: $LASTEXITCODE"}
if($Action -eq 'Map' -and !(Select-String -LiteralPath "$root/Saved/Logs/G0Map.log" -SimpleMatch -Pattern G0_MAP_SUCCESS -Quiet)){throw 'Map success marker missing'}
