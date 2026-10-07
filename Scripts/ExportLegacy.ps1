param([string]$SourceRoot='C:\Coding\void-and-thunder',[string]$Commit='c138f2c9caab77ed8288ddcb46d1622e471c2b15')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$Snapshot=Join-Path $ProjectRoot 'Intermediate\LegacyExport'
New-Item -ItemType Directory -Path $Snapshot -Force|Out-Null
git -C $SourceRoot archive --format=tar "--output=$Snapshot\source.tar" $Commit
if($LASTEXITCODE -ne 0){throw 'Source archive failed'}
tar -xf "$Snapshot\source.tar" -C $Snapshot
if($LASTEXITCODE -ne 0){throw 'Source extraction failed'}
$CargoPath=Join-Path $Snapshot 'crates\vt_client\Cargo.toml'
$Cargo=Get-Content $CargoPath -Raw
$Cargo=$Cargo.Replace('[dependencies]',"[dependencies]`nserde_json = `"1`"")
[IO.File]::WriteAllText($CargoPath,$Cargo)
$MainPath=Join-Path $Snapshot 'crates\vt_client\src\main.rs'
$Main=(Get-Content $MainPath -Raw).Replace('fn main() {','fn legacy_game_main() {')
$Main+="`n"+(Get-Content "$ProjectRoot\Tools\legacy-export-main.rs" -Raw)
[IO.File]::WriteAllText($MainPath,$Main)
Push-Location $Snapshot
try {
 cargo run -p vt_client --no-default-features --offline --target-dir "$ProjectRoot\Intermediate\LegacyTarget" -- "$ProjectRoot\Migration\resolved-baseline.json"
 if($LASTEXITCODE -ne 0){throw 'Typed export failed'}
} finally {Pop-Location}
