# Build PigeonCrosshair (Qt 6.12 + MinGW + Ninja). Uso: .\build.ps1   (opzionale: -Run)
param([switch]$Run, [switch]$Clean)

$qt = "C:\Qt"
$env:PATH = "$qt\Tools\mingw1310_64\bin;$qt\Tools\Ninja;$qt\Tools\CMake_64\bin;$env:PATH"
Set-Location $PSScriptRoot

if ($Clean -and (Test-Path build)) { Remove-Item -Recurse -Force build }

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release `
    -DCMAKE_PREFIX_PATH="$qt/6.12.0/mingw_64" `
    -DCMAKE_CXX_COMPILER="$qt/Tools/mingw1310_64/bin/g++.exe"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

cmake --build build
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "`nOK: build\PigeonCrosshair.exe"
if ($Run) { Start-Process build\PigeonCrosshair.exe }
