@echo off
setlocal
title FlashRDR2 - experimental Story Mode mod

if not exist "%~dp0Install.ps1" (
  echo Extract the ENTIRE ZIP first. Do not run inside WinRAR.
  pause
  exit /b 1
)

powershell.exe -NoLogo -NoProfile -STA -ExecutionPolicy Bypass -File "%~dp0Install.ps1"

set "result=%errorlevel%"
pause
exit /b %result%
