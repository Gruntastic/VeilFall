param([ValidateSet(0,100,200)][int]$RttMs=0,[ValidateRange(0,100)][int]$LossPercent=0,[switch]$Rendered,[ValidateRange(1024,65535)][int]$Port=7782)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Builds/VF-G2-20261003-A/Windows/VeilFallG0/Binaries/Win64/VeilFallG0.exe'
$run=Join-Path $root ('Docs/Logs/G2-Network-'+(Get-Date -Format yyyyMMdd-HHmmss))
New-Item -ItemType Directory -Path $run | Out-Null
$owned=@()
$mode='-nullrhi';if($Rendered){$mode='-dx12 -windowed -ResX=1280 -ResY=720'}
function Launch($url,$name){$log=Join-Path $run ($name+'.log');$args=$url+' -G2Smoke -unattended -nosound -nosplash '+$mode+" -PktLag=$([int]($RttMs/2)) -PktLoss=$LossPercent -abslog=`"$log`"";return Start-Process $exe -ArgumentList $args -WindowStyle Hidden -PassThru}
function WaitMarker($name,$pattern){$log=Join-Path $run ($name+'.log');$deadline=(Get-Date).AddSeconds(60);while((Get-Date) -lt $deadline){if((Test-Path $log) -and (Select-String -LiteralPath $log -Pattern $pattern -Quiet)){return};Start-Sleep -Seconds 1};throw "Timed out $name / $pattern"}
try{
 $owned+=Launch "/Game/DeadSignal/L_DeadSignal_G2?listen?port=$Port" Host
 WaitMarker Host G0_PAWN_BEGIN
 foreach($name in @('Client1','Client2','Client3')){$owned+=Launch "127.0.0.1:$Port" $name;WaitMarker $name G0_PAWN_BEGIN}
 foreach($name in @('Client1','Client2','Client3')){WaitMarker $name G2_SMOKE_CLIENT_PASS}
 WaitMarker Host 'G2_SMOKE_SERVER_PASS.*shots=3 validAmmo=1.*recoveredHP=50 med=1'
 if(@(Select-String -LiteralPath "$run/Host.log" -Pattern 'G2_SMOKE_SERVER_PASS.*shots=3 validAmmo=1.*recoveredHP=50 med=1').Count -ne 3){throw 'Expected server verification for all three clients'}
 "PASS VF-G2-20261003-A; four simultaneous processes; rendered=$Rendered; configured nominal RTT=$RttMs ms; outgoing directional packet loss=$LossPercent percent; endpoint PktLag=$([int]($RttMs/2)) ms. Actual combat RPCs and replication in isolated floating training fixtures. Shots/reload/slots/ammo kit/self recovery checked. Physical input, teammate revive, late-join combat, reconnect cleanup and performance NOT RUN." | Set-Content "$run/Result.txt"
}finally{foreach($p in $owned){if(!$p.HasExited){Stop-Process -Id $p.Id}}}
Get-Content "$run/Result.txt"


