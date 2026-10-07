param([string]$SourceRoot='C:\Coding\void-and-thunder')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$Snapshot=Join-Path $ProjectRoot 'Intermediate\ParityReference'
New-Item -ItemType Directory -Path $Snapshot -Force|Out-Null
git -C $SourceRoot archive --format=tar "--output=$Snapshot/source.tar" c138f2c9caab77ed8288ddcb46d1622e471c2b15
if($LASTEXITCODE -ne 0){throw 'Source archive failed'}
tar -xf "$Snapshot/source.tar" -C $Snapshot
if($LASTEXITCODE -ne 0){throw 'Source extraction failed'}
$ManifestPath="$Snapshot/Cargo.toml";$Manifest=Get-Content $ManifestPath -Raw;$Manifest=$Manifest.Replace('members = ["crates/vt_sim", "crates/vt_client"]','members = ["crates/vt_sim", "crates/vt_client", "parity-export"]');[IO.File]::WriteAllText($ManifestPath,$Manifest)
New-Item -ItemType Directory -Path "$Snapshot/parity-export/src" -Force|Out-Null
@('[package]','name = "vt_parity"','version = "0.1.0"','edition = "2021"','[dependencies]','vt_sim = { path = "../crates/vt_sim" }','bevy_math = "0.19"','serde_json = "1"') | Set-Content "$Snapshot/parity-export/Cargo.toml"
Copy-Item -LiteralPath "$ProjectRoot/Tools/legacy-parity-main.rs" -Destination "$Snapshot/parity-export/src/main.rs" -Force
Push-Location $Snapshot
try {cargo run -p vt_parity --offline --target-dir "$ProjectRoot/Intermediate/ParityTarget" -- "$ProjectRoot/Migration/resolved-baseline.json" "$ProjectRoot/Migration/golden-rules.json";if($LASTEXITCODE -ne 0){throw 'Golden export failed'}} finally {Pop-Location}
