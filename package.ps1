# Crea dist\PigeonCrosshair-win64.zip pronto da caricare su GitHub Releases.
# Uso: .\build.ps1 -Clean ; .\package.ps1
Set-Location $PSScriptRoot
if (-not (Test-Path build\PigeonCrosshair.exe)) { throw "Compila prima: .\build.ps1" }
$stage = "dist\PigeonCrosshair"
if (Test-Path dist) { Remove-Item -Recurse -Force dist }
New-Item -ItemType Directory -Force $stage | Out-Null
Get-ChildItem build -File | Where-Object { $_.Extension -in ".exe", ".dll" } | Copy-Item -Destination $stage
foreach ($d in "platforms","styles","tls","imageformats","iconengines","generic","networkinformation") {
    if (Test-Path "build\$d") { Copy-Item "build\$d" "$stage\$d" -Recurse }
}
Copy-Item LICENSE, THIRD-PARTY-NOTICES.md $stage
Compress-Archive -Path "$stage\*" -DestinationPath dist\PigeonCrosshair-win64.zip -Force
Write-Host "OK: dist\PigeonCrosshair-win64.zip"
