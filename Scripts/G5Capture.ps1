param([ValidateRange(10,30)][int]$Seconds=18)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$package=Join-Path $root 'Builds/VF-G5-20261005-D/Windows/VeilFallG0'
$exe=Join-Path $package 'Binaries/Win64/VeilFallG0.exe'
if(!(Test-Path $exe)){throw 'Build G5 D first'}
New-Item "$package/Docs/Captures" -ItemType Directory -Force|Out-Null
New-Item "$root/Docs/Captures" -ItemType Directory -Force|Out-Null
$args='-G1CaptureView=HQ -G5Preview -G1SaveRoot="'+$root+'/Saved/G5CaptureIsolated" -dx12 -windowed -ResX=1280 -ResY=720 -ExecCmds="sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,t.MaxFPS 60" -ini:Engine:[TextureStreaming]:PoolSizeVRAMPercentage=0,[SystemSettings]:r.Streaming.PoolSize=768 -abslog="'+$root+'/Docs/Logs/G5-Capture.log"'
$captureStart=Get-Date
$p=Start-Process $exe -ArgumentList $args -WindowStyle Hidden -PassThru
try{Start-Sleep -Seconds $Seconds}finally{$p.Refresh();if(!$p.HasExited){Stop-Process -Id $p.Id}}
$source=Get-ChildItem -LiteralPath @("$package/Saved/Screenshots/Windows","$root/Saved/Screenshots/Windows","$env:LOCALAPPDATA/VeilFallG0/Saved/Screenshots/Windows") -Filter *.png -ErrorAction SilentlyContinue | Where-Object {$_.LastWriteTime -ge $captureStart} | Sort-Object LastWriteTime -Descending | Select-Object -First 1 -ExpandProperty FullName
if(!$source-or!(Test-Path $source)){throw 'Real gameplay screenshot missing'}
Copy-Item $source "$root/Docs/Captures/G5-Workshop.png"
if(!(Select-String -LiteralPath "$root/Docs/Logs/G5-Capture.log" -Pattern "Game class is 'BP_OperationDeadSignal_G5_C'" -Quiet)){throw 'Wrong default map/game mode'}
Write-Output "$root/Docs/Captures/G5-Workshop.png"





