@echo off
rem COOP s47: the check of the maps of RESOURCE\Downloads. Double click = check every map.
rem map_check.bat MyMap -Fix   = check MyMap and copy the textures it needs.  map_check.bat MyMap -New land = a map.ini template.
chcp 65001 >nul
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0map_check.ps1" %*
pause
