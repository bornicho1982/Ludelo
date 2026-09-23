param (
    [string]$TargetDir = "build/gui",
    [string]$ExeName = "Ludelo.exe",
    [int]$SoakDurationSeconds = 30
)

$ErrorActionPreference = "Stop"
$root = (Get-Item -Path "$PSScriptRoot\..").FullName
Set-Location $root

Write-Host "==========================================================" -ForegroundColor Cyan
Write-Host " Ludelo Unified QA Ritual (Fail-Fast Verification Pipeline)" -ForegroundColor Cyan
Write-Host "==========================================================" -ForegroundColor Cyan

$results = [ordered]@{
    "a) Clean Compilation (CMake/MinGW)" = "PENDING"
    "b) Static Gatekeeper (Chiaki Imports)" = "PENDING"
    "c) QML Validation (--validate-qml 23 items)" = "PENDING"
    "d) Unit Tests (CTest)" = "PENDING"
    "e) Windows Deployment & Isolated Smoke Test" = "PENDING"
    "f) Continuous Resize Soak Test" = "PENDING"
}

function Print-SummaryAndExit([int]$ExitCode) {
    Write-Host "`n==========================================================" -ForegroundColor Cyan
    Write-Host "                 QA RITUAL SUMMARY REPORT                 " -ForegroundColor Cyan
    Write-Host "==========================================================" -ForegroundColor Cyan
    foreach ($step in $results.Keys) {
        $status = $results[$step]
        $color = switch ($status) {
            "PASS" { "Green" }
            "FAIL" { "Red" }
            default { "DarkGray" }
        }
        Write-Host (" {0,-45} [{1}]" -f $step, $status) -ForegroundColor $color
    }
    Write-Host "==========================================================" -ForegroundColor Cyan
    if ($ExitCode -eq 0) {
        Write-Host " ALL CHECKS PASSED - READY FOR RELEASE / ROUND COMPLETION" -ForegroundColor Green
    } else {
        Write-Host " PIPELINE FAILED - FIX ISSUES BEFORE PROCEEDING" -ForegroundColor Red
    }
    exit $ExitCode
}

# Ensure MSYS2 MinGW64 is in PATH for compilation and ctest
$env:PATH = "C:\msys64\mingw64\share\qt6\bin;C:\msys64\mingw64\bin;C:\msys64\usr\bin;" + $env:PATH
$exePath = Join-Path $TargetDir $ExeName

# -------------------------------------------------------------------------
# Step a: Compilación limpia
# -------------------------------------------------------------------------
Write-Host "`n[STEP 1/6] Running clean compilation..." -ForegroundColor Yellow
try {
    $buildProc = Start-Process -FilePath "cmake.exe" -ArgumentList "--build build --target chiaki -j4" -NoNewWindow -PassThru -Wait
    if ($buildProc.ExitCode -ne 0) {
        Write-Error "Compilation FAILED with exit code $($buildProc.ExitCode)"
        $results["a) Clean Compilation (CMake/MinGW)"] = "FAIL"
        Print-SummaryAndExit 1
    }
    $results["a) Clean Compilation (CMake/MinGW)"] = "PASS"
    Write-Host "-> Compilation OK!" -ForegroundColor Green
} catch {
    $results["a) Clean Compilation (CMake/MinGW)"] = "FAIL"
    Print-SummaryAndExit 1
}

# -------------------------------------------------------------------------
# Step b: Gatekeeper estático de imports Chiaki
# -------------------------------------------------------------------------
Write-Host "`n[STEP 2/6] Running static gatekeeper for 'import org.streetpea.chiaking'..." -ForegroundColor Yellow
try {
    $qmlFiles = Get-ChildItem -Path "gui/src/qml" -Filter "*.qml" -Recurse
    $failedFiles = @()
    foreach ($file in $qmlFiles) {
        $content = Get-Content -Path $file.FullName -Raw
        if ($content -match "Chiaki\." -and $content -notmatch "import\s+org\.streetpea\.chiaking") {
            $failedFiles += $file.FullName
        }
    }
    if ($failedFiles.Count -gt 0) {
        Write-Error "Static gatekeeper FAILED! The following files reference 'Chiaki.' without importing 'org.streetpea.chiaking':`n$($failedFiles -join "`n")"
        $results["b) Static Gatekeeper (Chiaki Imports)"] = "FAIL"
        Print-SummaryAndExit 1
    }
    $results["b) Static Gatekeeper (Chiaki Imports)"] = "PASS"
    Write-Host "-> Static gatekeeper OK! (Checked $($qmlFiles.Count) QML files)" -ForegroundColor Green
} catch {
    $results["b) Static Gatekeeper (Chiaki Imports)"] = "FAIL"
    Print-SummaryAndExit 1
}

# -------------------------------------------------------------------------
# Step c: --validate-qml (23 componentes)
# -------------------------------------------------------------------------
Write-Host "`n[STEP 3/6] Running --validate-qml across all 23 components..." -ForegroundColor Yellow
try {
    $valProc = Start-Process -FilePath $exePath -ArgumentList "--validate-qml" -NoNewWindow -PassThru -Wait
    if ($valProc.ExitCode -ne 0) {
        Write-Error "QML validation FAILED with exit code $($valProc.ExitCode)"
        $results["c) QML Validation (--validate-qml 23 items)"] = "FAIL"
        Print-SummaryAndExit 1
    }
    $results["c) QML Validation (--validate-qml 23 items)"] = "PASS"
    Write-Host "-> QML validation OK!" -ForegroundColor Green
} catch {
    $results["c) QML Validation (--validate-qml 23 items)"] = "FAIL"
    Print-SummaryAndExit 1
}

# -------------------------------------------------------------------------
# Step d: ctest
# -------------------------------------------------------------------------
Write-Host "`n[STEP 4/6] Running unit tests via CTest..." -ForegroundColor Yellow
try {
    $ctestProc = Start-Process -FilePath "ctest.exe" -ArgumentList "--test-dir build --output-on-failure" -NoNewWindow -PassThru -Wait
    if ($ctestProc.ExitCode -ne 0) {
        Write-Error "CTest FAILED with exit code $($ctestProc.ExitCode)"
        $results["d) Unit Tests (CTest)"] = "FAIL"
        Print-SummaryAndExit 1
    }
    $results["d) Unit Tests (CTest)"] = "PASS"
    Write-Host "-> CTest OK!" -ForegroundColor Green
} catch {
    $results["d) Unit Tests (CTest)"] = "FAIL"
    Print-SummaryAndExit 1
}

# -------------------------------------------------------------------------
# Step e: deploy-windows.ps1 (smoke test PATH aislado)
# -------------------------------------------------------------------------
Write-Host "`n[STEP 5/6] Running deployment & isolated PATH smoke test..." -ForegroundColor Yellow
try {
    $deployProc = Start-Process -FilePath "powershell.exe" -ArgumentList "-ExecutionPolicy Bypass -File `"$PSScriptRoot\deploy-windows.ps1`"" -NoNewWindow -PassThru -Wait
    if ($deployProc.ExitCode -ne 0) {
        Write-Error "deploy-windows.ps1 FAILED with exit code $($deployProc.ExitCode)"
        $results["e) Windows Deployment & Isolated Smoke Test"] = "FAIL"
        Print-SummaryAndExit 1
    }
    $results["e) Windows Deployment & Isolated Smoke Test"] = "PASS"
    Write-Host "-> Deployment & Smoke Test OK!" -ForegroundColor Green
} catch {
    $results["e) Windows Deployment & Isolated Smoke Test"] = "FAIL"
    Print-SummaryAndExit 1
}

# -------------------------------------------------------------------------
# Step f: soak-test-resize.ps1 (RAM estable + responsiveness true)
# -------------------------------------------------------------------------
Write-Host "`n[STEP 6/6] Running resize soak test ($SoakDurationSeconds seconds)..." -ForegroundColor Yellow
try {
    $soakProc = Start-Process -FilePath "powershell.exe" -ArgumentList "-ExecutionPolicy Bypass -File `"$PSScriptRoot\soak-test-resize.ps1`" -DurationSeconds $SoakDurationSeconds -SampleIntervalSeconds 5" -NoNewWindow -PassThru -Wait
    if ($soakProc.ExitCode -ne 0) {
        Write-Error "soak-test-resize.ps1 FAILED with exit code $($soakProc.ExitCode)"
        $results["f) Continuous Resize Soak Test"] = "FAIL"
        Print-SummaryAndExit 1
    }
    $results["f) Continuous Resize Soak Test"] = "PASS"
    Write-Host "-> Soak test OK!" -ForegroundColor Green
} catch {
    $results["f) Continuous Resize Soak Test"] = "FAIL"
    Print-SummaryAndExit 1
}

Print-SummaryAndExit 0
