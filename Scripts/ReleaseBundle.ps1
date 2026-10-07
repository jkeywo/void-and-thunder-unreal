param([ValidatePattern('^v[0-9]+\.[0-9]+\.[0-9]+[-a-zA-Z0-9.]*$')][string]$Version='v0.3.0-sandbox-preview')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$Package=Join-Path $ProjectRoot 'Artifacts/Shipping/Windows'
if(!(Test-Path "$Package/VoidAndThunder/Binaries/Win64/VoidAndThunder-Win64-Shipping.exe")){throw 'Build and validate Shipping before bundling'}
$Destination=Join-Path $ProjectRoot "Artifacts/Releases/$Version"
$Stage=Join-Path $Destination 'VoidAndThunder-Windows'
if(Test-Path $Destination){throw 'Release output exists; choose a new version to preserve the previous bundle'}
New-Item -ItemType Directory -Path $Stage -Force|Out-Null
foreach($Item in Get-ChildItem $Package -Recurse -File) {
 $Relative=[IO.Path]::GetRelativePath($Package,$Item.FullName)
 if($Relative -match '(?i)(^|[\/])Saved[\/]|\.pdb$|^Manifest_DebugFiles_|vc_redist\.arm64\.exe$'){continue}
 $Target=Join-Path $Stage $Relative;New-Item -ItemType Directory -Path (Split-Path $Target -Parent) -Force|Out-Null
 Copy-Item -LiteralPath $Item.FullName -Destination $Target
}
Copy-Item "$ProjectRoot/LICENSE" "$Stage/LICENSE.txt"
Copy-Item "$ProjectRoot/SourceAssets/CREDITS.md" "$Stage/ASSET-CREDITS.md"
Copy-Item "$ProjectRoot/docs/playing.md" "$Stage/CONTROLS-AND-HOSTING.md"
@('Void & Thunder — Windows sandbox preview','', 'Extract the entire folder, then start VoidAndThunder.exe.', 'Unreal Engine is not required. Keep Engine and VoidAndThunder folders beside the launcher.', 'If Windows reports missing runtime libraries, run Engine/Extras/Redist/en-us/vc_redist.x64.exe.', 'Read CONTROLS-AND-HOSTING.md for controls and LAN/direct-address hosting.', 'The host owns the world; closing the host ends the four-player session.', '', 'Validated reference: Windows 11, Core Ultra 9 275HX, 64 GB RAM, RTX 5090 Laptop GPU.', 'Lower-spec hardware and real WAN conditions have not been validated.', '500 active NPCs is the acceptance target; 1,000 NPCs exceeds the current simulation budget.', '', 'Source: https://github.com/jkeywo/void-and-thunder-unreal')|Set-Content "$Stage/START-HERE.txt"
$Binary=Get-FileHash "$Stage/VoidAndThunder/Binaries/Win64/VoidAndThunder-Win64-Shipping.exe" -Algorithm SHA256
$Commit=git -C $ProjectRoot rev-parse HEAD
@{version=$Version;source_commit=$Commit;engine='5.8.2';configuration='Shipping';platform='Win64';debug_symbols_included=$false;binary_sha256=$Binary.Hash.ToLower();created_utc=(Get-Date).ToUniversalTime().ToString('o')}|ConvertTo-Json|Set-Content "$Stage/BUILD.json"
$Archive=Join-Path $Destination "VoidAndThunder-$Version-Windows.zip"
Compress-Archive -Path $Stage -DestinationPath $Archive -CompressionLevel Optimal
$Hash=(Get-FileHash $Archive -Algorithm SHA256).Hash.ToLower()
"$Hash  $([IO.Path]::GetFileName($Archive))"|Set-Content "$Destination/SHA256SUMS.txt"
Write-Output "Release bundle: $Archive"
