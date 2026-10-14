param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$Package)
$ErrorActionPreference='Stop'
& "$PSScriptRoot/G0.ps1" Build -EngineRoot $EngineRoot
& "$PSScriptRoot/G0.ps1" Map -EngineRoot $EngineRoot
& "$PSScriptRoot/Test.ps1" -EngineRoot $EngineRoot
if($Package){& "$PSScriptRoot/G0.ps1" Package -EngineRoot $EngineRoot}
