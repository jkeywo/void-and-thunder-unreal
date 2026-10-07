param()
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$RunDir=Join-Path $ProjectRoot ('Saved\Validation\Shipping-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $RunDir -Force|Out-Null
$Exe="$ProjectRoot\Artifacts\Shipping\Windows\VoidAndThunder\Binaries\Win64\VoidAndThunder-Win64-Shipping.exe"
$Processes=@()
try {
 $Processes+=Start-Process -FilePath $Exe -ArgumentList @('-VTHostWorld=ShippingSmoke','-port=7793',"-UserDir=$RunDir/Host/",'-nullrhi','-unattended','-nosplash','-nosound') -WindowStyle Hidden -PassThru
 Start-Sleep -Seconds 8
 for($I=0;$I -lt 3;$I++) {$Processes+=Start-Process -FilePath $Exe -ArgumentList @('-VTJoinAddress=127.0.0.1:7793',"-UserDir=$RunDir/Guest$I/",'-nullrhi','-nosound','-unattended','-nosplash') -WindowStyle Hidden -PassThru}
 $Snapshot="$RunDir/Host/Saved/SaveGames/ShippingSmoke.vts"
 $Deadline=(Get-Date).AddSeconds(110)
 while(!(Test-Path $Snapshot) -and (Get-Date) -lt $Deadline) {foreach($Process in $Processes){if($Process.HasExited){throw "Shipping process exited early: $($Process.Id)"}}; Start-Sleep -Seconds 1}
 if(!(Test-Path $Snapshot)){throw "No Shipping autosave at $Snapshot"}
 $Metadata=& python "$ProjectRoot/Tools/inspect-snapshot.py" $Snapshot
 if($LASTEXITCODE -ne 0){throw 'Shipping snapshot inspection failed.'}
 $Parsed=$Metadata|ConvertFrom-Json
 if($Parsed.players -ne 4){throw "Shipping host saved $($Parsed.players) captains instead of four."}
 @{passed=$true;four_captains_saved=$true;autosave_observed=$true;snapshot_checksum_valid=$Parsed.checksum_valid}|ConvertTo-Json|Set-Content "$RunDir/Shipping.json"
 Write-Output "Shipping four-player possession and autosave passed: $RunDir"
} finally {foreach($Process in $Processes){if(!$Process.HasExited){Stop-Process -Id $Process.Id -ErrorAction SilentlyContinue}}}
