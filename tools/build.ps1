param(
    [ValidateSet("Debug", "Release")]
    [string] $Configuration = "Debug",
    [string] $Project = "HauntedToyRoom.vcxproj"
)

$ErrorActionPreference = "Stop"
$projectFile = Join-Path (Split-Path -Parent $PSScriptRoot) $Project

# Prefer the MSBuild installation that comes with Visual Studio or Build Tools.
$vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
$msbuild = $null

if (Test-Path -LiteralPath $vswhere) {
    $installationPath = & $vswhere -latest -products '*' -requires Microsoft.Component.MSBuild -property installationPath
    if ($LASTEXITCODE -eq 0 -and $installationPath) {
        $candidate = Join-Path $installationPath "MSBuild\Current\Bin\MSBuild.exe"
        if (Test-Path -LiteralPath $candidate) {
            $msbuild = $candidate
        }
    }
}

# Also support a Developer PowerShell or a shell with MSBuild already on PATH.
if (-not $msbuild) {
    $command = Get-Command "MSBuild.exe" -ErrorAction SilentlyContinue
    if ($command) {
        $msbuild = $command.Source
    }
}

if (-not $msbuild) {
    throw "MSBuild was not found. Install Visual Studio or Visual Studio Build Tools with the Desktop development with C++ workload, then reopen VS Code."
}

& $msbuild $projectFile /m /v:minimal "/p:Configuration=$Configuration" /p:Platform=x64
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
