param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
& "$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$ProjectRoot\VoidAndThunder.uproject" -run=VTHUDStyle -Apply -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput
if($LASTEXITCODE -ne 0){throw "HUD style upgrade failed: $LASTEXITCODE"}
