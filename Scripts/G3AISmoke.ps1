param([switch]$Rendered)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Builds/VF-G3-20261003-A/Windows/VeilFallG0/Binaries/Win64/VeilFallG0.exe'
$run=Join-Path $root ('Docs/Logs/G3-AI-'+(Get-Date -Format yyyyMMdd-HHmmss))
New-Item -ItemType Directory -Path $run | Out-Null
$mode='-nullrhi';if($Rendered){$mode='-dx12 -windowed -ResX=1280 -ResY=720 -G3Capture'}
$started=Get-Date
$args='/Game/DeadSignal/L_DeadSignal_G3 -G3Smoke -G3Mode=AI -G3AutoExit -unattended -nosound '+$mode+' -ExecCmds="t.MaxFPS 60" -G1SaveRoot="'+$run+'/IsolatedSave" -abslog="'+$run+'/AI.log"'
$p=Start-Process $exe -ArgumentList $args -WindowStyle Hidden -PassThru
try{
 if(!$p.WaitForExit(60000)){throw 'Packaged AI diagnostic exceeded sixty seconds'}
 if($p.ExitCode -ne 0){throw "AI process exited $($p.ExitCode)"}
 foreach($marker in @('G3_AI_NAV_PASS','G3_AI_MEMORY_PASS')){if(!(Select-String -LiteralPath "$run/AI.log" -SimpleMatch -Pattern $marker -Quiet)){throw "Missing $marker"}}
 if($Rendered){
  $shots=Join-Path $root "Builds/VF-G3-20261003-A/Windows/VeilFallG0/Saved/Screenshots/Windows"
  $latest=Get-ChildItem -LiteralPath $shots -Filter *.png | Where-Object LastWriteTime -ge $started | Sort-Object LastWriteTime -Descending | Select-Object -First 1
  if(!$latest){throw "No actual rendered AI screenshot"}
  Copy-Item -LiteralPath $latest.FullName -Destination "$root/Docs/Captures/G3-Relay.png"
 }
 'PASS VF-G3-20261003-A packaged AI: actual Behavior Trees, EQS queries, cover reservations, navigation, shared sightings, authoritative hostile damage, blocked vision and no unseen position/shot tracking. Positions and mission entry are diagnostic fixtures; player receives a diagnostic shield after the first hit to keep the sight-memory check alive. Normal clock capped at 60 FPS; frame-time and route feel NOT RUN.' | Set-Content "$run/Result.txt"
}catch{$_ | Out-String | Set-Content "$run/Failure.txt";throw}finally{if(!$p.HasExited){Stop-Process -Id $p.Id}}
Get-Content "$run/Result.txt"

