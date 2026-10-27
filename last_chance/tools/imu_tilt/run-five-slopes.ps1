$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath (Join-Path $PSScriptRoot '../..')
& .\.venv\Scripts\python.exe -u tools\imu_tilt\tilt_mission.py record --port COM7 --surfaces 5 --settle 5 --duration 10 --calibration-info docs\imu-calibration-replacement-2026-10-11.json
Write-Host "`nFive-slope session ended. You can close this window."
