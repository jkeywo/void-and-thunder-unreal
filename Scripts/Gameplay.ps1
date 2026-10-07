param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$Packaged,[switch]$Blackout,[int]$RoundTripMs=0,[int]$Loss=0)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$RunDir=Join-Path $ProjectRoot ('Saved\Validation\Gameplay-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $RunDir -Force|Out-Null
$Exe=if($Packaged){"$ProjectRoot/Artifacts/Development/Windows/VoidAndThunder/Binaries/Win64/VoidAndThunder.exe"}else{"$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"}
$Common=@('-game','-nullrhi','-nosound','-unattended','-nosplash','-stdout','-FullStdOutLogOutput',"-VTProbeDir=$RunDir","-PktLag=$([math]::Floor($RoundTripMs/2))","-PktLoss=$Loss",'-ExecCmds="t.MaxFPS 60"')
if(!$Packaged){$Common=@('"'+$ProjectRoot+'\VoidAndThunder.uproject"')+$Common}
if($Blackout){$Common+='-VTBlackout'}
$Processes=@()
try {
 $Processes+=Start-Process $Exe -ArgumentList ($Common+@('/Game/Maps/Sandbox?listen','-port=7795','-VTProbe=GameplayHost',"-UserDir=$RunDir/Host/")) -PassThru -WindowStyle Hidden -RedirectStandardOutput "$RunDir/GameplayHost.log"
 Start-Sleep -Seconds 8
 for($I=0;$I -lt 3;$I++) {$Processes+=Start-Process $Exe -ArgumentList ($Common+@('127.0.0.1:7795',"-VTProbe=GameplayClient$I","-UserDir=$RunDir/Guest$I/")) -PassThru -WindowStyle Hidden -RedirectStandardOutput "$RunDir/GameplayClient$I.log";Start-Sleep -Seconds 2}
 $Deadline=(Get-Date).AddSeconds(220)
 while((Get-Date) -lt $Deadline -and @($Processes|Where-Object {!$_.HasExited}).Count -gt 0){Start-Sleep -Seconds 1}
 foreach($P in $Processes){if(!$P.HasExited){throw "Gameplay process timeout: $($P.Id)"};if($P.ExitCode -ne 0){throw "Gameplay process failed: $($P.ExitCode); $RunDir"}}
 foreach($Role in @('GameplayHost','GameplayClient0','GameplayClient1','GameplayClient2')) {$Report=Get-Content "$RunDir/$Role.json" -Raw|ConvertFrom-Json;if(!$Report.passed){throw "Failed $Role; $RunDir"}}
 Write-Output "Four-player gameplay acceptance passed: $RunDir"
} finally {foreach($P in $Processes){if(!$P.HasExited){Stop-Process -Id $P.Id -ErrorAction SilentlyContinue}}}
