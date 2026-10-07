param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$Soak,[int]$SoakSeconds=7200)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$ReportDir=Join-Path $ProjectRoot ('Saved/Validation/NativeGate-'+(Get-Date -Format 'yyyyMMdd-HHmmss'))
New-Item -ItemType Directory -Path $ReportDir -Force|Out-Null
$Steps=@(
 @{name='source';script='SourceChecks';args=@{}},
 @{name='editor';script='Build';args=@{EngineRoot=$EngineRoot}},
 @{name='bootstrap';script='Bootstrap';args=@{EngineRoot=$EngineRoot}},
 @{name='assets';script='ValidateAssets';args=@{EngineRoot=$EngineRoot}},
 @{name='automation';script='Test';args=@{EngineRoot=$EngineRoot}},
 @{name='scale-500';script='Benchmark';args=@{EngineRoot=$EngineRoot;Population=500;Busy=$true;Armed=$true}},
 @{name='development';script='Package';args=@{EngineRoot=$EngineRoot}},
 @{name='gameplay-clean';script='Gameplay';args=@{Packaged=$true}},
 @{name='gameplay-latency';script='Gameplay';args=@{Packaged=$true;RoundTripMs=150;Loss=2}},
 @{name='gameplay-blackout';script='Gameplay';args=@{Packaged=$true;RoundTripMs=250;Loss=5;Blackout=$true}},
 @{name='reconnect';script='Multiplayer';args=@{Packaged=$true;Emulate=$true;Reconnect=$true}},
 @{name='create';script='SessionFlows';args=@{Packaged=$true}},
 @{name='continue';script='SessionFlows';args=@{Packaged=$true;Continue=$true}},
 @{name='host-departure';script='SessionFlows';args=@{Packaged=$true;HostDeparture=$true}},
 @{name='render';script='Render';args=@{Packaged=$true;Busy=$true;Armed=$true}},
 @{name='menu';script='Render';args=@{Packaged=$true;Menu=$true}},
 @{name='shipping';script='Package';args=@{EngineRoot=$EngineRoot;Configuration='Shipping'}},
 @{name='shipping-multiplayer';script='ShippingSmoke';args=@{}}
)
if($Soak){$Steps+=@{name='soak';script='Soak';args=@{Packaged=$true;Seconds=$SoakSeconds}}}
$Results=@()
foreach($Step in $Steps){$Started=Get-Date;$Parameters=$Step.args;try{& "$PSScriptRoot/$($Step.script).ps1" @Parameters *> "$ReportDir/$($Step.name).log";if(!$?){throw 'Native step failed'};$Results+=@{gate=$Step.name;passed=$true;seconds=((Get-Date)-$Started).TotalSeconds}}catch{$Results+=@{gate=$Step.name;passed=$false;error=$_.Exception.Message};$Results|ConvertTo-Json -Depth 4|Set-Content "$ReportDir/summary.json";throw}}
$Results|ConvertTo-Json -Depth 4|Set-Content "$ReportDir/summary.json"
Write-Output "Licensed native validation completed: $ReportDir"
