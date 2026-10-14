$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Builds/VF-G2-20261003-A/Windows/VeilFallG0/Binaries/Win64/VeilFallG0.exe'
$shots=Join-Path $root 'Builds/VF-G2-20261003-A/Windows/VeilFallG0/Saved/Screenshots/Windows'
$log=Join-Path $root 'Docs/Logs/G2-Capture.log'
$started=Get-Date
$args='/Game/DeadSignal/L_DeadSignal_G2 -dx12 -windowed -ResX=1280 -ResY=720 -benchmark -seconds=10 -G1CaptureView=Combat -abslog="'+$log+'"'
$p=Start-Process $exe -ArgumentList $args -WindowStyle Hidden -PassThru
if(!$p.WaitForExit(60000)){Stop-Process -Id $p.Id;throw 'Capture process timed out'}
if($p.ExitCode -ne 0){throw "Capture failed: $($p.ExitCode)"}
$latest=Get-ChildItem -LiteralPath $shots -Filter *.png | Where-Object LastWriteTime -ge $started | Sort-Object LastWriteTime -Descending | Select-Object -First 1
if(!$latest){throw 'No actual screenshot'}
Copy-Item -LiteralPath $latest.FullName -Destination "$root/Docs/Captures/G2-Combat.png"
'VF-G2-20261003-A DX12 camera capture exit 0. Explicit diagnostic camera; no gameplay/performance inference.' | Set-Content "$root/Docs/Logs/G2-Capture-Result.txt"

