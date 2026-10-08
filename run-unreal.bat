@echo off
setlocal
set "vt_game=%~dp0Artifacts\Development\Windows\VoidAndThunder.exe"
if not exist "%vt_game%" (
 echo Package the game first with PowerShell: .\Scripts\Package.ps1
 pause
 exit /b 1
)
"%vt_game%" %*
