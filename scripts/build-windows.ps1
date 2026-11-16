$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '..')
cmake -S . -B build-windows -G 'Visual Studio 17 2022' -A x64
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
cmake --build build-windows --config Release --parallel 2
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
ctest --test-dir build-windows -C Release --output-on-failure
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
