@echo off
setlocal
title FlashRDR2 Hypercar config installer
echo.
echo Drag this folder somewhere, then enter your RDR2 directory below.
echo Example:
echo C:\Program Files (x86)\Steam\steamapps\common\Red Dead Redemption 2
echo.
set /p RDR2DIR=RDR2 folder: 
if not exist "%RDR2DIR%\RDR2.exe" (
  echo RDR2.exe not found.
  pause
  exit /b 1
)
copy /Y "%~dp0vehicleconfig.json" "%RDR2DIR%\vehicleconfig.json"
echo.
echo Hypercar vehicleconfig.json installed.
echo Make sure RDR2AddonVehicles + its LML vehicle assets are installed too.
pause
