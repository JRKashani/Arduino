param(
    [ValidateSet('us', 'laser', 'ir')][string]$Sensor = 'us',
    [string]$Port = 'COM7'
)
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath (Join-Path $PSScriptRoot '../..')
& .\.venv\Scripts\python.exe -u tools\distance_test\distance_mission.py record --sensor $Sensor --port $Port
if ($LASTEXITCODE -ne 0) { throw 'Distance recorder stopped with an error; saved records remain in distance_sessions.' }
