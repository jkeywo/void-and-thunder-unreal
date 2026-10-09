param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
if(!(Test-Path "$ProjectRoot/Content/Data/DA_GameData.uasset")){throw 'Seed native content explicitly before validating authoring protection.'}
function Get-ContentHashes {
 $Result=@{};Get-ChildItem "$ProjectRoot/Content" -Recurse -File | Where-Object {$_.Extension -in '.uasset','.umap'} | ForEach-Object {$Result[$_.FullName]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash};return $Result
}
$Before=Get-ContentHashes
& "$PSScriptRoot/Bootstrap.ps1" -EngineRoot $EngineRoot
$After=Get-ContentHashes
if($Before.Count -ne $After.Count){throw 'Normal bootstrap changed the authored package set.'}
foreach($Path in $Before.Keys){if($After[$Path] -ne $Before[$Path]){throw "Normal bootstrap modified authored content: $Path"}}
$Report=@{passed=$true;packages=$Before.Count;mode='seed-preserves-existing';checkedAt=(Get-Date).ToString('o')}
$Report | ConvertTo-Json | Set-Content "$ProjectRoot/Saved/Validation/authoring-protection.json"
Write-Output "Authoring protection passed for $($Before.Count) native packages."
