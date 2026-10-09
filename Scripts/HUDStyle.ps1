param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$MenuReadability)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$Extra=@();if($MenuReadability){$Extra+='-MenuReadability'}
& "$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$ProjectRoot\VoidAndThunder.uproject" -run=VTHUDStyle -Apply -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput @Extra
if($LASTEXITCODE -ne 0){throw "HUD style upgrade failed: $LASTEXITCODE"}
