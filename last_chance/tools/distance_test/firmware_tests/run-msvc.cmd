@echo off
setlocal
cd /d "%~dp0\..\..\.."
if not defined VCINSTALLDIR call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist "build\distance-tests" mkdir "build\distance-tests"
cl /nologo /EHsc /std:c++14 /W4 /Itools\distance_test\firmware_tests tools\distance_test\firmware_tests\test_firmware.cpp /Fobuild\distance-tests\ /Febuild\distance-tests\test_firmware.exe
if errorlevel 1 exit /b 1
build\distance-tests\test_firmware.exe
exit /b %errorlevel%
