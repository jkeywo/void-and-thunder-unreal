param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[int]$Population=500)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
& "$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$ProjectRoot\VoidAndThunder.uproject" -run=VTBenchmark "-Population=$Population" -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput
if($LASTEXITCODE -ne 0){throw "Scale benchmark failed: $LASTEXITCODE"}
