@echo off
REM Cree un raccourci "WOTOL - Demo" sur le Bureau, pointant vers WOTOL.exe
REM situe dans le MEME dossier que ce script (place ce .bat a cote du .exe).
setlocal
set "TARGET=%~dp0WOTOL.exe"
set "WORKDIR=%~dp0"
set "LINK=%USERPROFILE%\Desktop\WOTOL - Demo.lnk"
powershell -NoProfile -Command ^
  "$s=(New-Object -ComObject WScript.Shell).CreateShortcut('%LINK%');" ^
  "$s.TargetPath='%TARGET%';" ^
  "$s.WorkingDirectory='%WORKDIR%';" ^
  "$s.IconLocation='%TARGET%,0';" ^
  "$s.Description='WOTOL - War of the Ocean''s Legacy (Demo)';" ^
  "$s.Save()"
echo Raccourci cree sur le Bureau : "WOTOL - Demo"
pause
