# Genuine engine captures using an explicit diagnostic camera and two-second warmup.
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Builds/VF-G1-20261003-A/Windows/VeilFallG0/Binaries/Win64/VeilFallG0.exe'
$shots=Join-Path $root 'Builds/VF-G1-20261003-A/Windows/VeilFallG0/Saved/Screenshots/Windows'
foreach($name in @('HQ','Chamber')){
 $log=Join-Path $root ('Docs/Logs/G1-Capture-'+$name+'.log')
 $started=Get-Date
 $args='/Game/DeadSignal/L_DeadSignal_G1 -dx12 -windowed -ResX=1280 -ResY=720 -benchmark -seconds=10 -G1CaptureView='+$name+' -abslog="'+$log+'"'
 $p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru
 if(!$p.WaitForExit(60000)){Stop-Process -Id $p.Id;throw 'Capture process timed out'}
 if($p.ExitCode -ne 0){throw "Capture $name failed"}
 $latest=Get-ChildItem -LiteralPath $shots -Filter *.png | Where-Object LastWriteTime -ge $started | Sort-Object LastWriteTime -Descending | Select-Object -First 1
 if(!$latest){throw "No actual screenshot from $name"}
 Copy-Item -LiteralPath $latest.FullName -Destination (Join-Path $root ('Docs/Captures/G1-'+$name+'.png'))
 "$name DX12 camera capture exit 0; source $($latest.FullName). No gameplay or performance inference." | Set-Content (Join-Path $root ('Docs/Logs/G1-Capture-'+$name+'-Result.txt'))
}
