param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$Broadside,[switch]$Menu,[switch]$Environment,[switch]$Packaged,[switch]$Busy,[switch]$Armed,[ValidateRange(640,7680)][int]$Width=1920,[ValidateRange(480,4320)][int]$Height=1080,[ValidateRange(0,3)][int]$Quality=2)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$Map=if($Menu){'/Game/Maps/Menu'}else{'/Game/Maps/Sandbox?listen'}
$Role=if($Menu){'RenderMenu'}elseif($Environment){'RenderEnvironment'}else{'Render'}
$Exe=if($Packaged){"$ProjectRoot\Artifacts\Development\Windows\VoidAndThunder\Binaries\Win64\VoidAndThunder.exe"}else{"$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"}
$Args=@(); if(!$Packaged){$Args+='"'+$ProjectRoot+'\VoidAndThunder.uproject"'}
$Extra=@();if($Broadside){$Extra+="-VTBroadsideProbe"};if($Busy){$Extra+="-Busy"};if($Armed){$Extra+="-Armed"};$Args+=$Extra
$Args+=@($Map,'-game',"-VTProbe=$Role",'-windowed','-ForceRes',"-ResX=$Width","-ResY=$Height",'-port=7787','-unattended','-nosplash','-nop4','-stdout','-FullStdOutLogOutput',"-ExecCmds=`"sg.ViewDistanceQuality $Quality,sg.AntiAliasingQuality $Quality,sg.ShadowQuality $Quality,sg.GlobalIlluminationQuality $Quality,sg.ReflectionQuality $Quality,sg.LandscapeQuality $Quality,sg.PostProcessQuality $Quality,sg.TextureQuality $Quality,sg.EffectsQuality $Quality,sg.FoliageQuality $Quality,sg.ShadingQuality $Quality,r.ScreenPercentage 100,t.MaxFPS 0`"")
$Started=Get-Date
$Process=Start-Process -FilePath $Exe -ArgumentList $Args -WindowStyle Hidden -PassThru -RedirectStandardOutput "$ProjectRoot\Saved\Validation\$Role.log"
$Process.WaitForExit(60000) | Out-Null
if(!$Process.HasExited){$Process.Kill();throw 'Render validation timed out.'}
if(Select-String -Path "$ProjectRoot/Saved/Validation/$Role.log" -Pattern 'Failed to compile Material|invalid ShaderMap|uncooked shader map' -Quiet){throw 'Rendered build used an invalid or missing cooked material shader'}
if($Process.ExitCode -ne 0){throw "Render gate failed: $($Process.ExitCode)"}

$OutputRoot=if($Packaged){"$ProjectRoot/Artifacts/Development/Windows/VoidAndThunder/Saved/Validation"}else{"$ProjectRoot/Saved/Validation"}
$ReportPath="$OutputRoot/$Role.json"
if(!(Test-Path $ReportPath) -or (Get-Item $ReportPath).LastWriteTime -lt $Started){throw 'Render did not produce a fresh report'}
$Report=Get-Content $ReportPath -Raw|ConvertFrom-Json
$Report|Add-Member -NotePropertyName requested_quality -NotePropertyValue $Quality
$Report|ConvertTo-Json -Depth 4|Set-Content $ReportPath
if(!$Report.passed -or $Report.width -ne $Width -or $Report.height -ne $Height){throw 'Render result or actual viewport dimensions did not match the requested gate'}
if(!(Test-Path "$OutputRoot/$Role.png") -or (Get-Item "$OutputRoot/$Role.png").LastWriteTime -lt $Started){throw 'Render did not produce a fresh screenshot'}
if($Packaged){Copy-Item $ReportPath "$ProjectRoot/Saved/Validation/$Role.json";Copy-Item "$OutputRoot/$Role.png" "$ProjectRoot/Saved/Validation/$Role.png"}
