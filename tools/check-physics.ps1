$ErrorActionPreference = "Stop"
& (Join-Path $PSScriptRoot "build.ps1") -Configuration Debug -Project "tests\PhysicsChecks.vcxproj"
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& (Join-Path (Split-Path -Parent $PSScriptRoot) "bin\Checks\PhysicsChecks.exe")
exit $LASTEXITCODE
