param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$Continue,[switch]$Packaged,[switch]$HostDeparture)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$RunDir=Join-Path $ProjectRoot ('Saved\Validation\Sessions-'+(Get-Date -Format 'yyyyMMdd-HHmmss')); New-Item -ItemType Directory -Path $RunDir -Force|Out-Null
$Exe=if($Packaged){"$ProjectRoot\Artifacts\Development\Windows\VoidAndThunder\Binaries\Win64\VoidAndThunder.exe"}else{"$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"}
$Common=@();if(!$Packaged){$Common+='"'+$ProjectRoot+'\VoidAndThunder.uproject"'}
$Common+=@('/Game/Maps/Menu','-game','-nullrhi','-nosound','-unattended','-nosplash',"-VTProbeDir=$RunDir",'-ExecCmds="t.MaxFPS 60"')
$Extra=@(); if($HostDeparture){$Extra+=@("-VTProbeSeconds=20")}; if($Continue){$Extra+='-VTContinue'}
$Processes=@()
try {
 $Processes+=Start-Process -FilePath $Exe -ArgumentList ($Common+@('-VTProbe=FlowHost','-port=7790',"-abslog=$RunDir\Host.log")+$Extra) -PassThru -WindowStyle Hidden
 Start-Sleep -Seconds 8
 $GuestExtra=@();if($HostDeparture){$GuestExtra+='-VTExpectHostDeparture'}
 $Processes+=Start-Process -FilePath $Exe -ArgumentList ($Common+$GuestExtra+@('-VTProbe=FlowGuest',"-abslog=$RunDir\Guest.log")) -PassThru -WindowStyle Hidden
 $Deadline=(Get-Date).AddSeconds(90)
 while((Get-Date) -lt $Deadline -and @($Processes|Where-Object {!$_.HasExited}).Count -gt 0){Start-Sleep -Seconds 1}
 $Roles=if($HostDeparture){@('FlowHost','FlowDepartureGuest')}else{@('FlowHost','FlowGuest')}; foreach($Role in $Roles) {if(!(Test-Path "$RunDir\$Role.json")){throw "No $Role result in $RunDir"};$Report=Get-Content "$RunDir\$Role.json" -Raw|ConvertFrom-Json;if(!$Report.passed){throw "$Role failed in $RunDir"}}
 Write-Output "Frontend session flow passed: $RunDir"
} finally {foreach($Process in $Processes){if(!$Process.HasExited){Stop-Process -Id $Process.Id -ErrorAction SilentlyContinue}}}
