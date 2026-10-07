param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[switch]$Menu,[switch]$Packaged,[switch]$Busy,[switch]$Armed)
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$Map=if($Menu){'/Game/Maps/Menu'}else{'/Game/Maps/Sandbox?listen'}
$Role=if($Menu){'RenderMenu'}else{'Render'}
$Exe=if($Packaged){"$ProjectRoot\Artifacts\Development\Windows\VoidAndThunder\Binaries\Win64\VoidAndThunder.exe"}else{"$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe"}
$Args=@(); if(!$Packaged){$Args+='"'+$ProjectRoot+'\VoidAndThunder.uproject"'}
$Extra=@();if($Busy){$Extra+="-Busy"};if($Armed){$Extra+="-Armed"};$Args+=$Extra
$Args+=@($Map,'-game',"-VTProbe=$Role",'-windowed','-ResX=1920','-ResY=1080','-port=7787','-unattended','-nosplash','-nop4','-stdout','-FullStdOutLogOutput','-ExecCmds="sg.ViewDistanceQuality 2,sg.AntiAliasingQuality 2,sg.ShadowQuality 2,sg.PostProcessQuality 2,sg.TextureQuality 2,sg.EffectsQuality 2,sg.FoliageQuality 2,sg.ShadingQuality 2,r.ScreenPercentage 100,t.MaxFPS 0"')
$Process=Start-Process -FilePath $Exe -ArgumentList $Args -WindowStyle Hidden -PassThru -RedirectStandardOutput "$ProjectRoot\Saved\Validation\$Role.log"
$Process.WaitForExit(60000) | Out-Null
if(!$Process.HasExited){$Process.Kill();throw 'Render validation timed out.'}
if($Process.ExitCode -ne 0){throw "Render gate failed: $($Process.ExitCode)"}
