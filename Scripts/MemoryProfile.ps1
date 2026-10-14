param([ValidateSet('Editor','Host','Join')][string]$Action='Host',[switch]$FullQuality,[string]$EngineRoot='C:/Program Files/Epic Games/UE_5.8')
$ErrorActionPreference='Stop'
$root=Split-Path -Parent $PSScriptRoot
$quality='sg.ViewDistanceQuality 1,sg.AntiAliasingQuality 2,sg.ShadowQuality 1,sg.GlobalIlluminationQuality 1,sg.ReflectionQuality 1,sg.PostProcessQuality 1,sg.TextureQuality 1,sg.EffectsQuality 1,sg.FoliageQuality 1,sg.ShadingQuality 1,r.ScreenPercentage 85,r.Streaming.PoolSize 768,r.RenderTargetPoolMin 200,t.MaxFPS 60'
if($FullQuality){$quality='t.MaxFPS 60'}
$exe=Join-Path $root 'Builds/VF-G3-20261003-A/Windows/VeilFallG0/Binaries/Win64/VeilFallG0.exe'
$url='/Game/DeadSignal/L_DeadSignal_G3?listen'
if($Action -eq 'Join'){$url='127.0.0.1'}
$args=$url+' -dx12 -windowed -ResX=1280 -ResY=720 -ExecCmds="'+$quality+'"'
if($Action -eq 'Editor'){
 $exe=Join-Path $EngineRoot 'Engine/Binaries/Win64/UnrealEditor.exe'
 $args='"'+$root+'/VeilFallG0.uproject" /Game/DeadSignal/L_DeadSignal_G3 -dx12 -ExecCmds="'+$quality+'"'
}
if(!$FullQuality){$args+=' -ini:Engine:[TextureStreaming]:PoolSizeVRAMPercentage=0,[SystemSettings]:r.Streaming.PoolSize=768,[SystemSettings]:r.RenderTargetPoolMin=200,[SystemSettings]:r.ScreenPercentage=85'}
if(!(Test-Path $exe)){throw "Missing executable: $exe"}
Start-Process $exe -ArgumentList $args -WindowStyle Hidden


