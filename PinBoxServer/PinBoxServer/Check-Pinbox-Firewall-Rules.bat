@echo off
title = Pinbox Firewall Setup
echo Checking PinBox Server executable...
IF NOT EXIST %~dp0PinBoxServer.exe GOTO pinnotdetected
echo PinBox Executable found!
echo Elevating privilegies...
if not "%1"=="am_admin" (powershell start -verb runas '%0' am_admin & exit /b)

echo Admin detected.
echo Adding rules...
rem REMEMBER TO CHANGE THE PROGRAM NAME IF THE BINARY FILE NAME CHANGES
netsh advfirewall firewall add rule name="PinboxOut" dir=out action=allow  profile=any program= "%~dp0PinBoxServer.exe" enable=yes
netsh advfirewall firewall add rule name="PinBoxIn" dir=in action=allow  profile=any program= "%~dp0PinBoxServer.exe" enable=yes
echo Done
pause
exit

:pinnotdetected
echo PinBoxServer.exe doesn't exists. Remember to run this script in THE SAME FOLDER AS THE EXECUTABLE.
pause exit