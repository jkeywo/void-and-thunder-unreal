param()
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
Push-Location $ProjectRoot
try {
 $Project=Get-Content VoidAndThunder.uproject -Raw|ConvertFrom-Json
 if($Project.EngineAssociation -ne '5.8'){throw 'Wrong Unreal engine association'}
 foreach($Module in $Project.Modules){if(!(Test-Path "Source/$($Module.Name)/$($Module.Name).Build.cs")){throw "Missing module: $($Module.Name)"}}
 $Pinned='c138f2c9caab77ed8288ddcb46d1622e471c2b15'
 foreach($Path in @('Migration/source-inventory.json','Migration/resolved-baseline.json','Migration/factions.json','Migration/golden-rules.json')){$Data=Get-Content $Path -Raw|ConvertFrom-Json;if($Data.source_commit -ne $Pinned){throw "Wrong baseline attribution: $Path"}}
 $Golden=Get-Content Migration/golden-rules.json -Raw|ConvertFrom-Json
 if($Golden.flight.Count -ne 40 -or $Golden.broadside.Count -ne 210 -or $Golden.shield.Count -ne 105 -or $Golden.ai.Count -ne 48){throw 'Incomplete golden rule corpus'}
 foreach($File in Get-ChildItem Scripts -Filter '*.ps1'){$Tokens=$null;$Errors=$null;[System.Management.Automation.Language.Parser]::ParseFile($File.FullName,[ref]$Tokens,[ref]$Errors)|Out-Null;if($Errors.Count){throw "Invalid PowerShell: $($File.Name): $Errors"}}
 foreach($Path in @('LICENSE','SourceAssets/CREDITS.md','design/setting/SOURCE.md','docs/decisions.md')){if(!(Test-Path $Path)){throw "Missing attribution: $Path"}}
 $Tracked=git ls-files
 foreach($Path in $Tracked){if($Path -match '\.(uasset|umap|glb|png|wav)$'){$Size=git cat-file -s "HEAD:$Path";if($LASTEXITCODE -eq 0){if([int]$Size -gt 1024){throw "Native binary is not stored through LFS: $Path"};$Pointer=(git show "HEAD:$Path") -join "`n";if($Pointer -notmatch '^version https://git-lfs.github.com/spec/v1' -or $Pointer -notmatch 'oid sha256:[0-9a-f]{64}' -or $Pointer -notmatch 'size [0-9]+'){throw "Invalid LFS pointer: $Path"}}}}
 Write-Output 'Source metadata, baseline attribution, golden corpus, script syntax and LFS pointers passed. Engine validation is separate.'
} finally {Pop-Location}
