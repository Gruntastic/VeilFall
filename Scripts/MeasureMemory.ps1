param([ValidateSet('Packaged','EditorGame','Editor')][string]$Target='Packaged',[ValidateRange(10,60)][int]$Seconds=20)
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$run=Join-Path $root ('Docs/Logs/Memory-'+$Target+'-'+(Get-Date -Format yyyyMMdd-HHmmss))
New-Item -ItemType Directory -Path $run | Out-Null
$exe=Join-Path $root 'Builds/VF-G3-20261003-A/Windows/VeilFallG0/Binaries/Win64/VeilFallG0.exe'
$prefix=''
if($Target -ne 'Packaged'){$exe='C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe';$prefix='"'+$root+'/VeilFallG0.uproject" '}
$quality='sg.ViewDistanceQuality 1,sg.AntiAliasingQuality 2,sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,sg.PostProcessQuality 1,sg.TextureQuality 1,sg.EffectsQuality 1,sg.FoliageQuality 1,sg.ShadingQuality 1,r.ScreenPercentage 85,r.Streaming.PoolSize 768,r.RenderTargetPoolMin 200,t.MaxFPS 60'
$results=@()
foreach($profile in @('Baseline','Reduced')){
 $commands='t.MaxFPS 60';if($profile -eq 'Reduced'){$commands=$quality}
 $args=$prefix+'/Game/DeadSignal/L_DeadSignal_G3 -game -G3Smoke -G3Mode=AI -dx12 -windowed -ResX=1280 -ResY=720 -unattended -ExecCmds="'+$commands+'" -G1SaveRoot="'+$run+'/'+$profile+'/Save" -abslog="'+$run+'/'+$profile+'.log"'
 if($profile -eq 'Reduced'){$args+=' -ini:Engine:[TextureStreaming]:PoolSizeVRAMPercentage=0,[SystemSettings]:r.Streaming.PoolSize=768,[SystemSettings]:r.RenderTargetPoolMin=200,[SystemSettings]:r.ScreenPercentage=85'}
 if($Target -eq 'Editor'){$args=$args.Replace(' -game -G3Smoke -G3Mode=AI','')}
 $p=Start-Process $exe -ArgumentList $args -WindowStyle Hidden -PassThru
 $samples=@();$clock=[Diagnostics.Stopwatch]::StartNew()
 try{
  while($clock.Elapsed.TotalSeconds -lt $Seconds){Start-Sleep -Milliseconds 500;$p.Refresh();if($p.HasExited){throw "Unexpected process exit: $profile"};$samples+=[pscustomobject]@{Seconds=[math]::Round($clock.Elapsed.TotalSeconds,2);WorkingSetMB=[math]::Round($p.WorkingSet64/1MB,2);PrivateCommitMB=[math]::Round($p.PrivateMemorySize64/1MB,2)}}
  $samples | Export-Csv "$run/$profile-Samples.csv" -NoTypeInformation
  $p.Refresh();$results+=[pscustomobject]@{Profile=$profile;Target=$Target;PeakWorkingSetMB=[math]::Round($p.PeakWorkingSet64/1MB,2);PeakSampledPrivateCommitMB=($samples | Measure-Object PrivateCommitMB -Maximum).Maximum;EndWorkingSetMB=$samples[-1].WorkingSetMB;EndPrivateCommitMB=$samples[-1].PrivateCommitMB}
  if($Target -eq 'Editor' -and !(Select-String -LiteralPath "$run/$profile.log" -SimpleMatch -Pattern '(Engine Initialization) Total time:' -Quiet)){throw "Editor initialization was incomplete"}
  if($Target -ne 'Editor'){foreach($marker in @('G3_AI_NAV_PASS','G3_AI_MEMORY_PASS')){if(!(Select-String -LiteralPath "$run/$profile.log" -SimpleMatch -Pattern $marker -Quiet)){throw "Missing $profile $marker"}}}
 }catch{$_ | Out-String | Set-Content "$run/Failure.txt";throw}finally{if(!$p.HasExited){Stop-Process -Id $p.Id}}
}
$results | Export-Csv "$run/Results.csv" -NoTypeInformation
$results | Format-Table | Out-String | Set-Content "$run/Result.txt"
"Duration=$Seconds seconds per process; target=$Target. Game targets include scripted relay combat at 1280x720 DX12. Full Editor measures map/viewport startup without playing; its window size is not controlled. Owned test processes stopped after sampling. Shader/DDC warm-up, background pressure and fixed run order can affect results. Working set/private commitment are CPU/process measurements; GPU bytes and full multiplayer route budgets NOT MEASURED. Reduced profile lowers visual quality and internal resolution to 85%; not the 1080p High acceptance target." | Add-Content "$run/Result.txt"
Get-Content "$run/Result.txt"



