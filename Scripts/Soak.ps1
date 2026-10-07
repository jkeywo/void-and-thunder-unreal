param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$Packaged,[string]$PackagedRoot='',[ValidateRange(120,86400)][int]$Seconds=7200,[ValidateRange(45,3600)][int]$ReconnectSeconds=300,[int]$RoundTripMs=150,[int]$Loss=2)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$RunDir=Join-Path $ProjectRoot ('Saved\Validation\Soak-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $RunDir -Force|Out-Null
if(!$PackagedRoot){$PackagedRoot="$ProjectRoot/Artifacts/Development/Windows"}
$Exe=if($Packaged){"$PackagedRoot/VoidAndThunder/Binaries/Win64/VoidAndThunder.exe"}else{"$EngineRoot/Engine/Binaries/Win64/UnrealEditor-Cmd.exe"}
$Common=@('-game','-nullrhi','-nosound','-unattended','-nosplash','-stdout','-FullStdOutLogOutput',"-VTProbeDir=$RunDir","-PktLag=$([math]::Floor($RoundTripMs/2))","-PktLoss=$Loss",'-ExecCmds="t.MaxFPS 60"')
if(!$Packaged){$Common=@('"'+$ProjectRoot+'\VoidAndThunder.uproject"')+$Common}
$Processes=@();$Reports=@();$Rejoins=0;$PreviousIdentity=$null;$Guest0=$null
function Start-Guest([int]$Index,[int]$Lifetime,[string]$Report) {
 Start-Process $Exe -ArgumentList ($Common+@('127.0.0.1:7796',"-VTProbe=SoakClient$Index","-VTReportRole=$Report","-VTProbeSeconds=$Lifetime","-UserDir=$RunDir/Guest$Index/")) -PassThru -WindowStyle Hidden -RedirectStandardOutput "$RunDir/$Report.log"
}
try {
 $HostProcess=Start-Process $Exe -ArgumentList ($Common+@('/Game/Maps/Sandbox?listen','-port=7796','-VTProbe=SoakHost',"-VTProbeSeconds=$Seconds","-UserDir=$RunDir/Host/")) -PassThru -WindowStyle Hidden -RedirectStandardOutput "$RunDir/SoakHost.log"
 $Processes+=$HostProcess;$Start=Get-Date
 Start-Sleep -Seconds 8
 for($I=1;$I -le 2;$I++) {$Processes+=Start-Guest $I ($Seconds-30) "SoakClient$I";$Reports+="SoakClient$I"}
 $CurrentReport='SoakClient0-0';$Guest0=Start-Guest 0 ([math]::Min($ReconnectSeconds,$Seconds-30)) $CurrentReport;$Processes+=$Guest0;$Reports+=$CurrentReport
 while(!$HostProcess.HasExited -and (Get-Date) -lt $Start.AddSeconds($Seconds+90)) {
  if($Guest0.HasExited) {
   if($Guest0.ExitCode -ne 0){throw "Guest failed: $RunDir/$CurrentReport.log"}
   $Identity=Get-Content "$RunDir/$CurrentReport.json" -Raw|ConvertFrom-Json
   if(!$Identity.passed){throw "Guest report failed: $CurrentReport"}
   if($PreviousIdentity -and ($PreviousIdentity.profile -ne $Identity.profile -or $PreviousIdentity.ship_id -ne $Identity.ship_id)){throw 'Reconnect changed persistent captain or ship identity'}
   $PreviousIdentity=$Identity;$Remaining=[math]::Floor($Seconds-((Get-Date)-$Start).TotalSeconds-25)
   if($Remaining -gt 20) {Start-Sleep -Seconds 2;$Rejoins++;$CurrentReport="SoakClient0-$Rejoins";$Guest0=Start-Guest 0 ([math]::Min($ReconnectSeconds,$Remaining)) $CurrentReport;$Processes+=$Guest0;$Reports+=$CurrentReport}
  }
  Start-Sleep -Seconds 1
 }
 if(!$HostProcess.HasExited -or $HostProcess.ExitCode -ne 0){throw "Soak host failed or timed out: $RunDir"}
 $HostReport=Get-Content "$RunDir/SoakHost.json" -Raw|ConvertFrom-Json
 if(!$HostReport.passed){throw "Campaign soak failed: $RunDir"}
 foreach($Role in $Reports){$Report=Get-Content "$RunDir/$Role.json" -Raw|ConvertFrom-Json;if(!$Report.passed){throw "Soak client failed: $Role"}}
 if($Rejoins -lt [math]::Max(1,[math]::Floor($Seconds/$ReconnectSeconds)-1)){throw 'Too few completed reconnect cycles'}
 @{passed=$true;seconds=$Seconds;reconnects=$Rejoins;rtt_ms=$RoundTripMs;loss_percent=$Loss;normal_mortal_npcs=$true;invulnerable_captains_fixture=$true}|ConvertTo-Json|Set-Content "$RunDir/Run.json"
 Write-Output "Campaign soak passed: $RunDir"
} finally {foreach($P in $Processes){if(!$P.HasExited){Stop-Process -Id $P.Id -ErrorAction SilentlyContinue}}}
