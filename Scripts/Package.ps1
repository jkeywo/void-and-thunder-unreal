param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[ValidateSet('Development','Shipping')][string]$Configuration='Development')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
& "$EngineRoot\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$ProjectRoot\VoidAndThunder.uproject" -nop4 -unattended -build -cook -stage -pak -archive -platform=Win64 "-clientconfig=$Configuration" "-archivedirectory=$ProjectRoot\Artifacts\$Configuration" -nocompileeditor
if($LASTEXITCODE -ne 0){throw "Packaging failed: $LASTEXITCODE"}
