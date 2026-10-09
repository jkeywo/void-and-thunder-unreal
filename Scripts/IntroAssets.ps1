param([switch]$RepairChoices,[switch]$RefreshVisuals,[string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
 $Extra=@();if($RepairChoices){$Extra+='-RepairChoices'};if($RefreshVisuals){$Extra+='-RefreshVisuals'}
& "$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe" "$ProjectRoot/VoidAndThunder.uproject" -run=VTIntroAssets -Apply -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput @Extra
if($LASTEXITCODE -ne 0){throw "Intro authoring failed: $LASTEXITCODE"}
