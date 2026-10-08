param([switch]$TorpedoOnly,[string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$Extra=@();if($TorpedoOnly){$Extra+="-TorpedoOnly"}
& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$ProjectRoot/VoidAndThunder.uproject" -run=VTFlightFixes -Apply -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput @Extra
if($LASTEXITCODE -ne 0){throw "Flight assets failed: $LASTEXITCODE"}
