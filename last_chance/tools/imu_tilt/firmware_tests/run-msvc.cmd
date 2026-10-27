@echo off
setlocal
cd /d "%~dp0\..\..\.."
if not defined VCINSTALLDIR call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
if not exist "build\imu-tilt-tests" mkdir "build\imu-tilt-tests"
cl /nologo /EHsc /std:c++14 /W4 /Itools\imu_tilt\firmware_tests /Isrc\imu_tilt /I..\libraries\ADXL335_IMU tools\imu_tilt\firmware_tests\test_firmware.cpp src\imu_tilt\ImuTiltMission.cpp ..\libraries\ADXL335_IMU\ADXL335_IMU.cpp /Fobuild\imu-tilt-tests\ /Febuild\imu-tilt-tests\test_firmware.exe
if errorlevel 1 exit /b 1
build\imu-tilt-tests\test_firmware.exe
exit /b %errorlevel%
