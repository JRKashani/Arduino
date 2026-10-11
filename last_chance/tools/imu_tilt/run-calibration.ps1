$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath (Join-Path $PSScriptRoot '../..')
& .\.venv\Scripts\python.exe -u tools\imu_tilt\calibrate.py --port COM7
Write-Host "`nCalibration session ended. You can close this window."
