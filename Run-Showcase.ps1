param(
    [ValidateSet("Story", "Manual", "Mounted")]
    [string] $Mode = "Story",
    [switch] $Raster
)
$ErrorActionPreference = "Stop"
$executable = Join-Path $PSScriptRoot "bin\Release\HauntedToyRoom.exe"
if (-not (Test-Path -LiteralPath $executable)) {
    & (Join-Path $PSScriptRoot "tools\build.ps1") -Configuration Release
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
$launchArguments = @()
if ($Mode -eq "Manual") { $launchArguments += @("--no-intro", "--manual") }
if ($Mode -eq "Mounted") { $launchArguments += @("--no-intro", "--manual", "--mount", "--select", "1", "--focus") }
if ($Raster) { $launchArguments += "--no-raytrace" }
Push-Location -LiteralPath $PSScriptRoot
try { & $executable @launchArguments } finally { Pop-Location }
