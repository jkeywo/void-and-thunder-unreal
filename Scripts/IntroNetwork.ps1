param([switch]$Packaged,[int]$RoundTripMs=0,[int]$Loss=0,[string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$RunDir=Join-Path $ProjectRoot ('Saved/Validation/IntroNetwork-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $RunDir -Force | Out-Null
$Exe=if($Packaged){"$ProjectRoot/Artifacts/Development/Windows/VoidAndThunder/Binaries/Win64/VoidAndThunder.exe"}else{"$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"}
$Common=@('-game','-nullrhi','-nosound','-unattended','-nosplash','-stdout','-FullStdOutLogOutput',"-VTProbeDir=$RunDir","-PktLag=$([math]::Floor($RoundTripMs/2))","-PktLoss=$Loss",'-ExecCmds="t.MaxFPS 60"')
if(!$Packaged){$Common=@('"'+$ProjectRoot+'\VoidAndThunder.uproject"')+$Common}
$Processes=@()
try {
 $Processes+=Start-Process $Exe -ArgumentList ($Common+@('/Game/Maps/Sandbox?listen','-port=7798','-VTProbe=IntroHost',"-UserDir=$RunDir/Host/")) -PassThru -WindowStyle Hidden -RedirectStandardOutput "$RunDir/IntroHost.log"
 Start-Sleep -Seconds 8
 for($I=0;$I -lt 3;$I++){$Processes+=Start-Process $Exe -ArgumentList ($Common+@('127.0.0.1:7798',"-VTProbe=IntroClient$I","-UserDir=$RunDir/Guest$I/")) -PassThru -WindowStyle Hidden -RedirectStandardOutput "$RunDir/IntroClient$I.log";Start-Sleep -Seconds 2}
 if(!$Processes[3].WaitForExit(65000)){throw 'Intro guest timed out'}
 if($Processes[3].ExitCode -ne 0){throw "Intro guest failed: $RunDir"}
 $Processes+=Start-Process $Exe -ArgumentList ($Common+@('127.0.0.1:7798','-VTProbe=IntroClient2','-VTIntroReconnect',"-UserDir=$RunDir/Guest2/")) -PassThru -WindowStyle Hidden -RedirectStandardOutput "$RunDir/IntroClient2-Reconnect.log"
 foreach($P in $Processes){if(!$P.WaitForExit(55000)){throw 'Intro network timeout'};if($P.ExitCode -ne 0){throw "Intro network process failed: $RunDir"}}
 foreach($Role in @('IntroHost','IntroClient0','IntroClient1','IntroClient2','IntroClient2-Reconnect')){$R=Get-Content "$RunDir/$Role.json" -Raw|ConvertFrom-Json;if(!$R.passed){throw "Intro network failed $Role"}}
 $Before=Get-Content "$RunDir/IntroClient2.json" -Raw|ConvertFrom-Json;$After=Get-Content "$RunDir/IntroClient2-Reconnect.json" -Raw|ConvertFrom-Json
 if($Before.stage -ne $After.stage){throw 'Reconnect lost its checkpoint'}
 Write-Output "Four-player intro and reconnect passed: $RunDir"
} finally {foreach($P in $Processes){if(!$P.HasExited){Stop-Process -Id $P.Id -ErrorAction SilentlyContinue}}}
