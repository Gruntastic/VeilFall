param([switch]$Rendered,[ValidateSet(0,100,200)][int]$RttMs=0,[ValidateRange(0,100)][int]$LossPercent=0,[ValidateSet(3,4)][int]$Players=3)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Builds/VF-G0-20261003-C/Windows/VeilFallG0/Binaries/Win64/VeilFallG0.exe'
if(!(Test-Path -LiteralPath $exe)){throw "Packaged binary missing: $exe"}
$run=Join-Path $root ('Docs/Logs/Network-'+(Get-Date -Format yyyyMMdd-HHmmss))
New-Item -ItemType Directory -Path $run | Out-Null
$hostLog=Join-Path $run 'Host.log'; $clientLog=Join-Path $run 'Client.log'; $lateLog=Join-Path $run 'LateJoin.log'
$extra='-nullrhi'
if($Rendered){$extra='-dx12 -windowed -ResX=1280 -ResY=720'}
$extra += " -PktLag=$([int]($RttMs/2)) -PktLoss=$LossPercent"
$client2Log=Join-Path $run "Client2.log"
$owned=@()
function Launch($url,$log,$diagnostic){
 $args=$url+' -unattended -nosound -nosplash '+$extra+' -abslog="'+$log+'" '+$diagnostic
 $p=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru
 return $p
}
function WaitMarker($log,$pattern){
 $deadline=(Get-Date).AddSeconds(60)
 while((Get-Date) -lt $deadline){
  if((Test-Path -LiteralPath $log) -and (Select-String -LiteralPath $log -Pattern $pattern -Quiet)){return}
  Start-Sleep -Seconds 1
 }
 throw "Timed out: $pattern in $log"
}
try{
 $owned+=Launch '/Game/Foundation/L_G0?listen' $hostLog '-G0Smoke'
 WaitMarker $hostLog 'G0_PAWN_BEGIN'
 $owned+=Launch '127.0.0.1' $clientLog '-G0Smoke'
 WaitMarker $clientLog 'G0_PAWN_BEGIN'
 if($Players -eq 4){$owned+=Launch '127.0.0.1' $client2Log '-G0Smoke'; WaitMarker $client2Log 'G0_PAWN_BEGIN'}
 Start-Sleep -Seconds 12
 $owned+=Launch '127.0.0.1' $lateLog ''
 WaitMarker $lateLog 'G0_PAWN_BEGIN'
 Start-Sleep -Seconds 5
 $h=Get-Content -LiteralPath $hostLog -Raw; $c=Get-Content -LiteralPath $clientLog -Raw; $l=Get-Content -LiteralPath $lateLog -Raw
 $hostCounts=[regex]::Matches($h,'G0_SWITCH_STATE count=(\d+) authority=1')
 if($hostCounts.Count -eq 0){throw 'No authoritative activation'}
 foreach($taskLogText in @($h,$c,$l)){ if($taskLogText -notmatch ('PktLag set to '+[int]($RttMs/2)) -or $taskLogText -notmatch ('PktLoss set to '+$LossPercent)){throw 'Emulation settings were not confirmed in every process'} }
 $expected=$hostCounts[$hostCounts.Count-1].Groups[1].Value
 foreach($pair in @(@('client',$c),@('late join',$l))){
  $counts=[regex]::Matches($pair[1],'G0_SWITCH_STATE count=(\d+) authority=0')
  if($counts.Count -eq 0 -or $counts[$counts.Count-1].Groups[1].Value -ne $expected){throw ($pair[0]+' disagrees with host')}
 }
 if($Players -eq 4){$client2=Get-Content -LiteralPath $client2Log -Raw; $counts=[regex]::Matches($client2,'G0_SWITCH_STATE count=(\d+) authority=0'); if($counts.Count -eq 0 -or $counts[$counts.Count-1].Groups[1].Value -ne $expected){throw 'Second initial client disagrees with host'}; if($client2 -notmatch ('PktLag set to '+[int]($RttMs/2)) -or $client2 -notmatch ('PktLoss set to '+$LossPercent)){throw 'Second client emulation unconfirmed'} }
 $users=[regex]::Matches($h,'G0_INTERACTION_ACCEPTED user=([^ ]+)') | ForEach-Object {$_.Groups[1].Value} | Sort-Object -Unique
 if(@($users).Count -lt ($Players-1)){throw 'Remote client interaction not evidenced; host alone is insufficient'}
 $result="PASS packaged separate-process host/client interaction and late join: players=$Players count=$expected users=$($users -join ','). Mode=$extra. No keyboard/visual/performance inference."
 [IO.File]::WriteAllText((Join-Path $run 'Result.txt'),$result)
 Write-Output $result
 Write-Output $run
} finally {
 foreach($p in $owned){if(!$p.HasExited){Stop-Process -Id $p.Id}}
}
