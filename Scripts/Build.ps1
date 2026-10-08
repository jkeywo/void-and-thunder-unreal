param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[ValidateSet('Editor','Game')][string]$Target='Editor',[ValidateSet('Development','Shipping')][string]$Configuration='Development')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$TargetName=if($Target -eq 'Editor'){'VoidAndThunderEditor'}else{'VoidAndThunder'}
& "$EngineRoot\Engine\Build\BatchFiles\Build.bat" $TargetName Win64 $Configuration "-Project=$ProjectRoot\VoidAndThunder.uproject" -WaitMutex -NoHotReloadFromIDE -NoUBTMakefiles
if($LASTEXITCODE -ne 0){throw "Unreal build failed: $LASTEXITCODE"}
