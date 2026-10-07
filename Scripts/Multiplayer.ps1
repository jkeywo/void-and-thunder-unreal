param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$Emulate)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$RunDir=Join-Path $ProjectRoot ('Saved\Validation\Multiplayer-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $RunDir -Force|Out-Null
$Exe=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$Common=@("$ProjectRoot\VoidAndThunder.uproject",'-game','-nullrhi','-nosound','-unattended','-nop4','-nosplash','-NoCrashDialog','-NoErrorReport',"-VTProbeDir=$RunDir",'-ExecCmds="t.MaxFPS 60"')
if($Emulate){$Common+=@('-PktLag=75','-PktLoss=2')}
$Processes=@()
try {
 $Processes+=Start-Process -FilePath $Exe -ArgumentList ($Common+@('/Game/Maps/Sandbox?listen','-VTProbe=Host','-port=7789',"-abslog=$RunDir\Host.log")) -PassThru -WindowStyle Hidden
 Start-Sleep -Seconds 8
 $Destinations=@('meridian_gate','pale_meridian','vethara_seat')
 for($I=0;$I -lt 3;$I++){
  $Args=$Common+@('127.0.0.1:7789',"-VTProbe=Client$I","-VTProbeSystem=$($Destinations[$I])","-abslog=$RunDir\Client$I.log")
  $Processes+=Start-Process -FilePath $Exe -ArgumentList $Args -PassThru -WindowStyle Hidden
 }
 $Deadline=(Get-Date).AddSeconds(90)
 while((Get-Date) -lt $Deadline -and @($Processes|Where-Object {-not $_.HasExited}).Count -gt 0){Start-Sleep -Seconds 1}
 foreach($P in $Processes){if(-not $P.HasExited){throw "Multiplayer process timed out: $($P.Id)"}}
 foreach($Role in @('Host','Client0','Client1','Client2')){
  $Path=Join-Path $RunDir "$Role.json"
  if(-not(Test-Path $Path)){throw "Missing probe report: $Role"}
  $Report=Get-Content $Path -Raw|ConvertFrom-Json
  if(-not $Report.passed){throw "Probe failed: $Role; inspect $RunDir"}
 }
 Write-Output "Four-player scale, movement, replication and independent travel passed: $RunDir"
} finally {
 foreach($P in $Processes){if(-not $P.HasExited){Stop-Process -Id $P.Id -Force}}
}
