param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8')
$ErrorActionPreference='Stop'
$ProjectRoot=Split-Path $PSScriptRoot -Parent
$ReportDir=Join-Path $ProjectRoot 'Saved\Validation\Automation'
& "$EngineRoot\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" "$ProjectRoot\VoidAndThunder.uproject" '-ExecCmds=Automation RunTests VT.' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$ReportDir" -VTProbe=Automation -unattended -nop4 -nosplash -nullrhi -stdout -FullStdOutLogOutput
if($LASTEXITCODE -ne 0){throw "Automation failed: $LASTEXITCODE"}
if(-not(Test-Path "$ReportDir\index.json")){throw 'Automation did not produce a report'}
$Report=Get-Content "$ReportDir\index.json" -Raw|ConvertFrom-Json
if($Report.failed -gt 0 -or $Report.succeeded -eq 0){throw 'Tests failed or no tests executed'}
