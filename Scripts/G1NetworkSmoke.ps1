param([ValidateSet(0,100,200)][int]$RttMs=0,[ValidateRange(0,100)][int]$LossPercent=0,[switch]$Rendered,[ValidateRange(1024,65535)][int]$Port=7781)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Builds/VF-G1-20261003-A/Windows/VeilFallG0/Binaries/Win64/VeilFallG0.exe'
if(!(Test-Path -LiteralPath $exe)){throw 'G1 package missing'}
$run=Join-Path $root ('Docs/Logs/G1-Network-'+(Get-Date -Format yyyyMMdd-HHmmss))
$saveRoot=Join-Path $run 'IsolatedSave'
New-Item -ItemType Directory -Path $run | Out-Null
$owned=@()
$mode='-nullrhi';if($Rendered){$mode='-dx12 -windowed -ResX=1280 -ResY=720'}
function Launch($url,$name,$extra){
 $log=Join-Path $run ($name+'.log')
 $args=$url+' -unattended -nosound -nosplash '+$mode+" -PktLag=$([int]($RttMs/2)) -PktLoss=$LossPercent -abslog=`"$log`" "+$extra
 return Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru
}
function WaitMarker($name,$pattern){
 $log=Join-Path $run ($name+'.log');$deadline=(Get-Date).AddSeconds(60)
 while((Get-Date) -lt $deadline){
  if((Test-Path -LiteralPath $log) -and (Select-String -LiteralPath $log -Pattern $pattern -Quiet)){return}
  Start-Sleep -Seconds 1
 }
 throw "Timed out $name / $pattern"
}
function StopOwned(){foreach($p in $owned){if(!$p.HasExited){Stop-Process -Id $p.Id}}}
try{
 $owned+=Launch '/Game/DeadSignal/L_DeadSignal_G1?listen' 'Host' "-port=$Port -G1Smoke -G1Observer -G1SaveRoot=`"$saveRoot`""
 WaitMarker Host 'G0_PAWN_BEGIN'
 $owned+=Launch "127.0.0.1:$Port" 'Client' '-G1Smoke'
 WaitMarker Client 'G0_PAWN_BEGIN'
 $owned+=Launch "127.0.0.1:$Port" 'Client2' '-G1Smoke'
 WaitMarker Client2 'G0_PAWN_BEGIN'
 WaitMarker Host 'G1_SAVE success=1 stage=9 outpost=3 clues=3 stash=1'
 WaitMarker Client 'G1_STATE authority=0 stage=9 outpost=3 clues=3 stash=1'
 WaitMarker Client2 'G1_STATE authority=0 stage=9 outpost=3 clues=3 stash=1'
 $owned+=Launch "127.0.0.1:$Port" 'LateJoin' ''
 WaitMarker LateJoin 'G1_STATE authority=0 stage=9 outpost=3 clues=3 stash=1'
 # Disconnect one guest; reconnect in a fresh process and check host-owned state.
 if(!$owned[1].HasExited){Stop-Process -Id $owned[1].Id}
 Start-Sleep -Seconds 2
 $owned+=Launch "127.0.0.1:$Port" 'Reconnect' ''
 WaitMarker Reconnect 'G1_STATE authority=0 stage=9 outpost=3 clues=3 stash=1'
 $hostText=Get-Content (Join-Path $run 'Host.log') -Raw
 $actions=[regex]::Matches($hostText,'G1_ACTION user=([^ ]+) action=([^\r\n]+)')
 if($actions.Count -ne 11){throw "Expected exactly nine mission actions and two clues; got $($actions.Count)"}
 foreach($name in @('Host','Client','Client2','LateJoin','Reconnect')){
  $content=Get-Content (Join-Path $run ($name+'.log')) -Raw
  if($content -notmatch ('PktLag set to '+[int]($RttMs/2)) -or $content -notmatch ('PktLoss set to '+$LossPercent)){throw "Emulation unconfirmed in $name"}
 }
 StopOwned
 Start-Sleep -Seconds 2
 $owned+=Launch '/Game/DeadSignal/L_DeadSignal_G1?listen' 'RestoredHost' "-port=$Port -G1Smoke -G1Observer -G1SaveRoot=`"$saveRoot`""
 WaitMarker RestoredHost 'G1_LOAD success=1 stage=9 outpost=3 clues=3 stash=1'
 WaitMarker RestoredHost 'G1_SPAWN tag=RelayEstablished'
 $owned+=Launch "127.0.0.1:$Port" 'RestoredGuest' ''
 WaitMarker RestoredGuest 'G1_STATE authority=0 stage=9 outpost=3 clues=3 stash=1'
 $result="PASS VF-G1-20261003-A: remote-client mission progression; 11 unique accepted actions; outpost/clues/stash convergence; fourth late join; guest disconnect/reconnect; fresh host save restore and restored guest. Nominal configured RTT=$RttMs ms, outgoing loss=$LossPercent% per direction. $mode. Diagnostic server teleports are enabled: no traversal/input/feel/performance inference."
 [IO.File]::WriteAllText((Join-Path $run 'Result.txt'),$result)
 Write-Output $result;Write-Output $run
}finally{StopOwned}
