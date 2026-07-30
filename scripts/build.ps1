$ErrorActionPreference = "Stop"

$repoRoot = Split-Path $PSScriptRoot -Parent

$projects = @(Get-ChildItem -Path $repoRoot -Filter *.vcxproj -File)

switch ($projects.Count) {
    0 { throw "No .vcxproj found in '$repoRoot'." }
    1 { $project = $projects[0] }
    default { throw "Expected one .vcxproj in '$repoRoot', found $($projects.Count)." }
}

$msbuild = Join-Path $env:MSBUILD_PATH "MSBuild.exe"

if (!(Test-Path $msbuild)) {
    throw "MSBuild.exe not found at '$msbuild'."
}

Write-Host "Building $($project.Name)..."

& $msbuild `
    $project.FullName `
    /p:Configuration=Release `
    /p:Platform=x64

exit $LASTEXITCODE