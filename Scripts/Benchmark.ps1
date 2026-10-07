param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[int]$Population=500,[switch]$Busy,[switch]$Armed,[switch]$RequireBudget)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
 $Extra=@(); if($Busy){$Extra+='-Busy'}; if($Armed){$Extra+='-Armed'}; if($RequireBudget){$Extra+='-RequireBudget'}
& "$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$ProjectRoot\VoidAndThunder.uproject" -run=VTBenchmark "-Population=$Population" -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput @Extra
if($LASTEXITCODE -ne 0){throw "Scale benchmark failed: $LASTEXITCODE"}
