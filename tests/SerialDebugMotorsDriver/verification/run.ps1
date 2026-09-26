$ErrorActionPreference = 'Stop'
$sketchRoot = Split-Path -Parent $PSScriptRoot
$sketchbookRoot = Split-Path -Parent (Split-Path -Parent $sketchRoot)
$buildRoot = Join-Path $sketchRoot 'build\host'
New-Item -ItemType Directory -Force -Path $buildRoot | Out-Null
$vswherePath = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$vsRoot = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$vsRoot) { throw 'Visual Studio C++ Build Tools are required for host simulation.' }
$devShell = Join-Path $vsRoot 'Common7\Tools\Launch-VsDevShell.ps1'
& $devShell -Arch amd64 -HostArch amd64 -SkipAutomaticLocation | Out-Null
$motorRoot = Join-Path $sketchbookRoot 'libraries\Motor'
$steeringRoot = Join-Path $sketchbookRoot 'libraries\SteeringDualH'
Push-Location $buildRoot
try {
  & cl.exe /nologo /EHsc /std:c++14 /W4 /I $PSScriptRoot /I $motorRoot /I $steeringRoot (Join-Path $PSScriptRoot 'simulation.cpp') (Join-Path $motorRoot 'Motor.cpp') (Join-Path $steeringRoot 'SteeringDualH.cpp') /Fe:simulation.exe
  if ($LASTEXITCODE -ne 0) { throw 'Host simulation compilation failed.' }
  & .\simulation.exe
  if ($LASTEXITCODE -ne 0) { throw 'Host simulation failed.' }
} finally {
  Pop-Location
}
