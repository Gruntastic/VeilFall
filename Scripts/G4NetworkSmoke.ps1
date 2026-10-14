param([ValidateSet(0,100,200)][int]$RttMs=0,[ValidateRange(0,100)][int]$LossPercent=0,[switch]$Rendered,[int]$Port=7790)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$exe=Join-Path $root 'Builds/VF-G4-20261005-E/Windows/VeilFallG0/Binaries/Win64/VeilFallG0.exe'
$run=Join-Path $root ('Docs/Logs/G4-Network-'+(Get-Date -Format yyyyMMdd-HHmmss));New-Item $run -ItemType Directory|Out-Null
$owned=@();$samples=@()
function Launch($url,$name){$mode='-nullrhi';if($Rendered-and $name-eq 'Host'){$mode='-dx12 -windowed -ResX=1280 -ResY=720 -ini:Engine:[TextureStreaming]:PoolSizeVRAMPercentage=0,[SystemSettings]:r.Streaming.PoolSize=768 -ExecCmds="sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,t.MaxFPS 60"'}else{$mode+=' -ExecCmds="t.MaxFPS 60"'};$args=$url+" -G4Smoke -unattended $mode -PktLag=$([int]($RttMs/2)) -PktLoss=$LossPercent -G1SaveRoot=`"$run/IsolatedSave`" -abslog=`"$run/$name.log`"";Start-Process $exe -ArgumentList $args -WindowStyle Hidden -PassThru}
try{
 $owned+=Launch "/Game/DeadSignal/L_DeadSignal_G4?listen?port=$Port" Host
 Start-Sleep -Seconds 5
 $owned+=Launch "127.0.0.1:$Port" Client
 $deadline=(Get-Date).AddSeconds(45)
 while((Get-Date)-lt $deadline){
  $ws=0;$commit=0;foreach($p in $owned){$p.Refresh();if(!$p.HasExited){$ws+=$p.WorkingSet64;$commit+=$p.PrivateMemorySize64}}
  $samples+=[pscustomobject]@{UTC=[DateTime]::UtcNow.ToString('o');WorkingSetMB=[math]::Round($ws/1MB,2);PrivateCommitMB=[math]::Round($commit/1MB,2)}
  $h=if(Test-Path "$run/Host.log"){Get-Content "$run/Host.log" -Raw}else{''};$c=if(Test-Path "$run/Client.log"){Get-Content "$run/Client.log" -Raw}else{''}
  if(([regex]::Matches($h,'G4_SERVER_SPRINT_PASS')).Count-ge 2-and([regex]::Matches($h,'G4_SERVER_COVER_PASS')).Count-ge 2-and $c.Contains('G4_CLIENT_FATIGUE_PASS')-and([regex]::Matches($h,'G4_SERVER_TOP_PEEK_PASS')).Count-ge 2-and([regex]::Matches($h,'G4_SERVER_PEEK_RELEASE_PASS')).Count-ge 2-and([regex]::Matches($h,'G4_SERVER_OPEN_EDGE_PASS')).Count-ge 2-and $c.Contains('G4_CLIENT_TOP_PEEK_PASS')){break}
  Start-Sleep -Seconds 1
 }
 if(([regex]::Matches($h,'G4_SERVER_SPRINT_PASS')).Count-lt 2-or([regex]::Matches($h,'G4_SERVER_COVER_PASS')).Count-lt 2-or !$c.Contains('G4_CLIENT_FATIGUE_PASS')-or([regex]::Matches($h,'G4_SERVER_TOP_PEEK_PASS')).Count-lt 2-or([regex]::Matches($h,'G4_SERVER_PEEK_RELEASE_PASS')).Count-lt 2-or([regex]::Matches($h,'G4_SERVER_OPEN_EDGE_PASS')).Count-lt 2-or !$c.Contains('G4_CLIENT_TOP_PEEK_PASS')){throw 'Missing actual host/client sprint top-peek release or open-edge evidence'}
 if(($h+$c)-match 'Fatal error|Reliable buffer overflow|NetworkFailure|Assertion failed'){throw 'Engine/network error detected'}
 "PASS two-process G4; RTT target=$RttMs; PktLag per outbound process=$([int]($RttMs/2))ms; PktLoss each direction=$LossPercent%; opt-in server fixture placement and automated client movement. Rendered host=$Rendered" | Set-Content "$run/Result.txt"
 Write-Output "G4_NETWORK_PASS $run"
}finally{
 $samples|Export-Csv "$run/ProcessMemory.csv" -NoTypeInformation
 foreach($p in $owned){$p.Refresh();if(!$p.HasExited){Stop-Process -Id $p.Id}}
}



