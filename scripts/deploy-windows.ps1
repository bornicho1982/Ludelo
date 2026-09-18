param (
    [string]$TargetDir = "build/gui",
    [string]$ExeName = "Ludelo.exe"
)

$ErrorActionPreference = "Stop"
$root = (Get-Item -Path "$PSScriptRoot\..").FullName
Set-Location $root

$exePath = Join-Path $TargetDir $ExeName
if (-not (Test-Path $exePath)) {
    Write-Error "Executable not found at: $exePath"
    exit 1
}

Write-Host "=== 1. Deploying Qt6 dependencies with windeployqt6 ==="
$env:PATH = "C:\msys64\mingw64\share\qt6\bin;C:\msys64\mingw64\bin;C:\msys64\usr\bin;" + $env:PATH
& "C:\msys64\mingw64\bin\windeployqt6.exe" --no-translations --qmldir "gui/src/qml" $exePath

Write-Host "=== 2. Deploying MinGW runtime & transitive DLLs ==="
$mingwBin = "C:\msys64\mingw64\bin"
$added = $true
$round = 1
while ($added) {
    $added = $false
    $filesToCheck = Get-ChildItem -Path $TargetDir -Include "*.exe","*.dll" -Recurse | Where-Object { $_.FullName -notmatch "\\qml\\" }
    foreach ($file in $filesToCheck) {
        $ldd = & "C:\msys64\usr\bin\ldd.exe" $file.FullName
        foreach ($line in $ldd) {
            if ($line -match "=> /mingw64/bin/([^\s]+)") {
                $dllName = $matches[1]
                $dest = Join-Path $TargetDir $dllName
                if (-not (Test-Path $dest)) {
                    $src = Join-Path $mingwBin $dllName
                    if (Test-Path $src) {
                        Copy-Item $src $dest -Force
                        Write-Host "Copied: $dllName"
                        $added = $true
                    }
                }
            }
        }
    }
    $round++
}

Write-Host "=== 3. QML Validation & Smoke Test (isolated PATH without MSYS2) ==="
$origPath = $env:PATH
try {
    $env:PATH = ($env:PATH -split ';' | Where-Object { $_ -notmatch 'msys64' }) -join ';'
    
    Write-Host "--- Validating Root QML & All UI Screens ---"
    $qmlTest = Start-Process -FilePath $exePath -ArgumentList "--validate-qml" -NoNewWindow -PassThru -Wait
    if ($qmlTest.ExitCode -ne 0) {
        Write-Error "QML Validation FAILED! Exit Code: $($qmlTest.ExitCode)"
        exit $qmlTest.ExitCode
    }
    Write-Host "QML Validation: ALL SCREENS READY AND COMPILED!"

    Write-Host "--- Checking CLI Interface ---"
    $p = Start-Process -FilePath $exePath -ArgumentList "--help" -NoNewWindow -PassThru -Wait
    if ($p.ExitCode -ne 0) {
        Write-Error "Smoke test FAILED with Exit Code: $($p.ExitCode)"
        exit $p.ExitCode
    }
} finally {
    $env:PATH = $origPath
}
Write-Host "=== Deploy, QML Validation & Smoke Test SUCCESSFUL (Exit Code 0) ==="
