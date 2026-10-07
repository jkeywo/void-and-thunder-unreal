param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$Emulate,[switch]$Reconnect,[switch]$Packaged,[int]$RoundTripMs=150,[int]$Loss=2,[switch]$SameSystem)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$RunDir=Join-Path $ProjectRoot ('Saved\Validation\Multiplayer-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $RunDir -Force|Out-Null
$Exe=if($Packaged){Join-Path $ProjectRoot 'Artifacts\Development\Windows\VoidAndThunder\Binaries\Win64\VoidAndThunder.exe'}else{Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'}
if(-not(Test-Path $Exe)){throw "Missing executable: $Exe"}
$Common=@("$ProjectRoot\VoidAndThunder.uproject",'-game','-nullrhi','-nosound','-unattended','-nop4','-nosplash','-NoCrashDialog','-NoErrorReport',"-VTProbeDir=$RunDir",'-ExecCmds="t.MaxFPS 60"')
if($Packaged){$Common=$Common[1..($Common.Length-1)]}
if($Emulate){$Common+=@("-PktLag=$([math]::Floor($RoundTripMs/2))","-PktLoss=$Loss")}
$Processes=@()
$HostExtra=if($Reconnect){@('-VTProbeSeconds=80')}else{@()}
try {
 $Processes+=Start-Process -FilePath $Exe -ArgumentList ($Common+@('/Game/Maps/Sandbox?listen','-VTProbe=Host','-port=7789',"-abslog=$RunDir\Host.log")+$HostExtra) -PassThru -WindowStyle Hidden
 Start-Sleep -Seconds 8
 $Destinations=@('meridian_gate','pale_meridian','vethara_seat')
 if($SameSystem){$Destinations=@("","","")}
 for($I=0;$I -lt 3;$I++){
  $Travel=@();if($Destinations[$I]){$Travel+="-VTProbeSystem=$($Destinations[$I])"}
  $Args=$Common+@('127.0.0.1:7789',"-VTProbe=Client$I","-abslog=$RunDir\Client$I.log")+$Travel
  $Processes+=Start-Process -FilePath $Exe -ArgumentList $Args -PassThru -WindowStyle Hidden
 }
 if($Reconnect){
  $Guest=$Processes[1]
  $GuestDeadline=(Get-Date).AddSeconds(60)
  while(-not $Guest.HasExited -and (Get-Date) -lt $GuestDeadline){Start-Sleep -Seconds 1}
  if(-not $Guest.HasExited){throw 'Initial guest did not exit'}
  if(-not(Test-Path "$RunDir\Client0.json")){throw 'Initial guest did not report'}
  Start-Sleep -Seconds 2
  $Args=$Common+@('127.0.0.1:7789','-VTProbe=Client0','-VTReportRole=Client0Rejoined','-VTRejoined','-VTProbeSystem=meridian_gate',"-abslog=$RunDir\Client0Rejoined.log")
  $Processes+=Start-Process -FilePath $Exe -ArgumentList $Args -PassThru -WindowStyle Hidden
 }
 $Deadline=(Get-Date).AddSeconds(120)
 while((Get-Date) -lt $Deadline -and @($Processes|Where-Object {-not $_.HasExited}).Count -gt 0){Start-Sleep -Seconds 1}
 foreach($P in $Processes){if(-not $P.HasExited){throw "Multiplayer process timed out: $($P.Id)"}}
 foreach($Role in @('Host','Client0','Client1','Client2')){
  $Path=Join-Path $RunDir "$Role.json"
  if(-not(Test-Path $Path)){throw "Missing probe report: $Role"}
  $Report=Get-Content $Path -Raw|ConvertFrom-Json
  if(-not $Report.passed){throw "Probe failed: $Role; inspect $RunDir"}
 }
 if($Reconnect){
  $Before=Get-Content "$RunDir\Client0.json" -Raw|ConvertFrom-Json
  $After=Get-Content "$RunDir\Client0Rejoined.json" -Raw|ConvertFrom-Json
  $Distance=[math]::Sqrt([math]::Pow($After.origin_x-$Before.position_x,2)+[math]::Pow($After.origin_y-$Before.position_y,2))
  if($Distance -gt 60 -or -not $After.passed -or $Before.ship_id -ne $After.ship_id -or $Before.profile -ne $After.profile){throw "Reconnect lost ship or profile identity: $RunDir"}
 }
 Write-Output "Four-player scale, movement, replication and independent travel passed: $RunDir"
} finally {
 foreach($P in $Processes){if(-not $P.HasExited){Stop-Process -Id $P.Id -Force}}
}
