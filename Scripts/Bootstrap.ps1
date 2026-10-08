param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$Regenerate,[switch]$UpgradeNative)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$MigrationArgs=@(); if($Regenerate){$MigrationArgs+='-Regenerate'}; if($UpgradeNative){$MigrationArgs+='-UpgradeNative'}
& "$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$ProjectRoot\VoidAndThunder.uproject" -run=VTBootstrap @MigrationArgs -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput
if($LASTEXITCODE -ne 0){throw "Content bootstrap failed: $LASTEXITCODE"}
