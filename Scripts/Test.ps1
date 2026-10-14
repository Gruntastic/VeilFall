param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$log=Join-Path $root 'Docs/Logs/FoundationTests-C.log'
& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$root/VeilFallG0.uproject" -unattended -nullrhi -nosplash '-ExecCmds=Automation RunTests VeilFallG0.Foundation' '-TestExit=Automation Test Queue Empty' "-abslog=$log"
if($LASTEXITCODE -ne 0){throw 'Engine test process failed'}
if(!(Select-String -LiteralPath $log -Pattern 'Result={Success}' -SimpleMatch -Quiet)){throw 'No passing result'}
if(Select-String -LiteralPath $log -Pattern 'Result={Fail}' -SimpleMatch -Quiet){throw 'Test failed'}

if(@(Select-String -LiteralPath $log -Pattern 'Result={Success}' -SimpleMatch).Count -ne 2){throw 'Expected both foundation tests to finish successfully'}
