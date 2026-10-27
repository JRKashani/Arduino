@echo off
setlocal
cd /d "%~dp0\..\..\.."
if not defined VCINSTALLDIR call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
if not exist "build\manual-drive-tests" mkdir "build\manual-drive-tests"
cl /nologo /EHsc /std:c++14 /W4 /WX /Ilib\ManualCommand tools\manual_drive\firmware_tests\test_manual_command.cpp lib\ManualCommand\ManualCommand.cpp /Fobuild\manual-drive-tests\ /Febuild\manual-drive-tests\test_manual_command.exe
if errorlevel 1 exit /b 1
build\manual-drive-tests\test_manual_command.exe
exit /b %errorlevel%
