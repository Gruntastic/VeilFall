param([ValidateRange(10,30)][int]$Seconds=18)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$package=Join-Path $root 'Builds/VF-G4-20261005-E/Windows/VeilFallG0'
$exe=Join-Path $package 'Binaries/Win64/VeilFallG0.exe'
if(!(Test-Path $exe)){throw 'Build G4 C first'}
New-Item "$package/Docs/Captures" -ItemType Directory -Force|Out-Null
New-Item "$root/Docs/Captures" -ItemType Directory -Force|Out-Null
$args='-G4Capture -G1SaveRoot="'+$root+'/Saved/G4CaptureIsolated" -dx12 -windowed -ResX=1280 -ResY=720 -ExecCmds="sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,t.MaxFPS 60" -ini:Engine:[TextureStreaming]:PoolSizeVRAMPercentage=0,[SystemSettings]:r.Streaming.PoolSize=768 -abslog="'+$root+'/Docs/Logs/G4-Capture.log"'
$p=Start-Process $exe -ArgumentList $args -WindowStyle Hidden -PassThru
try{Start-Sleep -Seconds $Seconds}finally{$p.Refresh();if(!$p.HasExited){Stop-Process -Id $p.Id}}
$source="$package/Docs/Captures/G4-Benchmarks.png"
if(!(Test-Path $source)){throw 'Real gameplay screenshot missing'}
Copy-Item $source "$root/Docs/Captures/G4-Benchmarks.png"
if(!(Select-String -LiteralPath "$root/Docs/Logs/G4-Capture.log" -Pattern "Game class is 'TacticalGameMode'" -Quiet)){throw 'Wrong default map/game mode'}
Write-Output "$root/Docs/Captures/G4-Benchmarks.png"

