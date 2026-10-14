param([ValidateSet(0,100,200)][int]$RttMs=0,[ValidateRange(0,100)][int]$LossPercent=0,[switch]$Rendered,[switch]$HostRendered,[ValidateRange(1024,65535)][int]$Port=7786)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Builds/VF-G3-20261003-A/Windows/VeilFallG0/Binaries/Win64/VeilFallG0.exe'
$run=Join-Path $root ('Docs/Logs/G3-Network-'+(Get-Date -Format yyyyMMdd-HHmmss))
$saveRoot=Join-Path $run 'IsolatedSave'
New-Item -ItemType Directory -Path $run | Out-Null
$owned=@()
$memorySamples=@()
$mode='-nullrhi -ExecCmds="t.MaxFPS 60"';if($Rendered){$mode='-dx12 -windowed -ResX=1280 -ResY=720 -ExecCmds="t.MaxFPS 60"'}
function Launch($url,$name,$extra){$log=Join-Path $run ($name+'.log');if($HostRendered -and $name -in @('Host','RestoredHost')){$launchMode='-dx12 -windowed -ResX=1280 -ResY=720 -ini:Engine:[TextureStreaming]:PoolSizeVRAMPercentage=0,[SystemSettings]:r.Streaming.PoolSize=768,[SystemSettings]:r.RenderTargetPoolMin=200,[SystemSettings]:r.ScreenPercentage=85 -ExecCmds="sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,sg.PostProcessQuality 1,sg.EffectsQuality 1,t.MaxFPS 60"'}else{$launchMode=$mode};$args=$url+' -G3Smoke -unattended -nosound -nosplash '+$launchMode+" -PktLag=$([int]($RttMs/2)) -PktLoss=$LossPercent -G1SaveRoot=`"$saveRoot`" -abslog=`"$log`" "+$extra;return Start-Process $exe -ArgumentList $args -WindowStyle Hidden -PassThru}
function SampleMemory(){ $ws=0;$commit=0;$count=0;foreach($testProcess in $owned){$testProcess.Refresh();if(!$testProcess.HasExited){$count++;$ws+=$testProcess.WorkingSet64;$commit+=$testProcess.PrivateMemorySize64}};$script:memorySamples+=[pscustomobject]@{UTC=[DateTime]::UtcNow.ToString("o");Processes=$count;WorkingSetMB=[math]::Round($ws/1MB,2);PrivateCommitMB=[math]::Round($commit/1MB,2)} }
function WaitMarker($name,$pattern){$log=Join-Path $run ($name+'.log');$deadline=(Get-Date).AddSeconds(55);while((Get-Date) -lt $deadline){SampleMemory;if((Test-Path $log) -and (Select-String -LiteralPath $log -Pattern $pattern -Quiet)){return};Start-Sleep -Seconds 1};throw "Timed out $name / $pattern"}
function StopOwned(){foreach($p in $owned){if(!$p.HasExited){Stop-Process -Id $p.Id}}}
try{
 $hostProcess=Launch "/Game/DeadSignal/L_DeadSignal_G3?listen?port=$Port" Host '-G3Mode=Lifecycle';$owned+=$hostProcess
 WaitMarker Host G0_PAWN_BEGIN
 $first=Launch "127.0.0.1:$Port" Client1 ''; $owned+=$first;WaitMarker Client1 G0_PAWN_BEGIN
 $second=Launch "127.0.0.1:$Port" Client2 ''; $owned+=$second;WaitMarker Client2 G0_PAWN_BEGIN
 WaitMarker Host 'G3_TEAM_REVIVE_PASS hp=50 charge=1 fieldRetained=1'
 WaitMarker Host G3_RECOVERY_READY
 $late=Launch "127.0.0.1:$Port" LateJoin '-G3Observer';$owned+=$late
 WaitMarker LateJoin G3_LATE_JOIN_CLIENT_PASS
 $extra=Launch "127.0.0.1:$Port" CapacityProbe '-G3Observer';$owned+=$extra
 WaitMarker CapacityProbe 'session full';if(!$extra.HasExited){Stop-Process -Id $extra.Id}
 WaitMarker Host G3_DISCONNECT_READY
 if(Select-String -LiteralPath "$run/Client1.log" -Pattern '^.*G3_WINNER$' -Quiet){$winner=$first;$winnerName='Client1';$remaining='Client2'}else{$winner=$second;$winnerName='Client2';$remaining='Client1'}
 if(!$winner.HasExited){Stop-Process -Id $winner.Id}
 WaitMarker Host G3_LOGOUT
 WaitMarker Host 'G3_DISCONNECT_SERVER_PASS packs=1 stash=1 playerPawns=3'
 $reconnect=Launch "127.0.0.1:$Port" Reconnect '-G3Observer';$owned+=$reconnect
 WaitMarker Reconnect G3_RECONNECT_CLIENT_PASS
 WaitMarker Host G3_LIFECYCLE_SERVER_PASS
 WaitMarker $remaining G3_LIFECYCLE_CLIENT_PASS
 WaitMarker Reconnect G3_LIFECYCLE_CLIENT_PASS
 StopOwned
 $restored=Launch "/Game/DeadSignal/L_DeadSignal_G3?listen?port=$Port" RestoredHost '-G3Mode=Restore';$owned+=$restored
 WaitMarker RestoredHost G3_RESTORE_SERVER_PASS
 $restoredGuest=Launch "127.0.0.1:$Port" RestoredGuest '-G3Observer';$owned+=$restoredGuest
 WaitMarker RestoredGuest G3_RESTORE_CLIENT_PASS
 $dropCount=@(Select-String -LiteralPath "$run/Host.log" -Pattern G3_PACK_DROP).Count
 $claimCount=@(Select-String -LiteralPath "$run/Host.log" -Pattern G3_PACK_CLAIM).Count
 if($dropCount -ne 2 -or $claimCount -ne 1){throw "Wrong pack transaction counts: drops=$dropCount claims=$claimCount"}
 if(!(Select-String -LiteralPath "$run/Client1.log" -Pattern G3_UNOWNED_RPC_PROBE_SENT -Quiet) -or !(Select-String -LiteralPath "$run/Client2.log" -Pattern G3_UNOWNED_RPC_PROBE_SENT -Quiet)){throw 'Ownership probes did not execute'}
 "PASS VF-G3-20261003-A; rendered=$Rendered; hostRenderedOnly=$HostRendered; all diagnostic processes capped at 60 FPS; configured nominal RTT=$RttMs ms, outgoing directional loss=$LossPercent percent, endpoint PktLag=$([int]($RttMs/2)) ms. Real owned E RPC pickup race, timed teammate revive, death pack claim/bank, late join, capacity rejection, timed disconnect cleanup/drop, reconnect and host restart persistence. Exactly two drops and one pack claim. Unowned fire attempts leave authoritative ammo unchanged. Fixture positions/aim are injected and bleed deadline shortened; physical input, route feel and performance NOT RUN. Winner killed: $winnerName." | Set-Content "$run/Result.txt"
}catch{$_ | Out-String | Set-Content "$run/Failure.txt";throw}finally{$memorySamples | Export-Csv "$run/ProcessMemory.csv" -NoTypeInformation;StopOwned}
Get-Content "$run/Result.txt"



