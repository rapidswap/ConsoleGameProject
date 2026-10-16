$ErrorActionPreference = 'Stop'
$taskRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio\Installer\vswhere.exe'
$installation = & $vswhere -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (!$installation) { throw 'Visual Studio C++ build tools are required.' }
$devCommand = Join-Path $installation 'Common7\Tools\VsDevCmd.bat'
$buildDirectory = Join-Path $taskRoot 'Intermediate\PathfindingTests'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
Push-Location $taskRoot
try {
    & $env:ComSpec /d /c "call `"$devCommand`" -arch=x64 >nul && cl /nologo /std:c++20 /EHsc /O2 /W4 /I ServerCore Tests\PathfindingTests.cpp /Fo`"$buildDirectory\PathfindingTests.obj`" /Fe`"$buildDirectory\PathfindingTests.exe`""
    if ($LASTEXITCODE -ne 0) { throw 'Pathfinding test compilation failed.' }
    & (Join-Path $buildDirectory 'PathfindingTests.exe')
    if ($LASTEXITCODE -ne 0) { throw 'Pathfinding tests failed.' }
} finally {
    Pop-Location
}
