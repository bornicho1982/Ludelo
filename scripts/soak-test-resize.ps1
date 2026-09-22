param (
    [string]$TargetDir = "build/gui",
    [string]$ExeName = "Ludelo.exe",
    [int]$DurationSeconds = 120,
    [int]$SampleIntervalSeconds = 5
)

$ErrorActionPreference = "Stop"
$root = (Get-Item -Path "$PSScriptRoot\..").FullName
Set-Location $root

$exePath = Join-Path $TargetDir $ExeName
if (-not (Test-Path $exePath)) {
    Write-Error "Executable not found at: $exePath"
    exit 1
}

Write-Host "=========================================================="
Write-Host " Ludelo Continuous Resize Soak Test ($DurationSeconds seconds)"
Write-Host "=========================================================="

Add-Type @"
using System;
using System.Runtime.InteropServices;

public class Win32Window {
    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool SetWindowPos(IntPtr hWnd, IntPtr hWndInsertAfter, int X, int Y, int cx, int cy, uint uFlags);

    [DllImport("user32.dll")]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool GetWindowRect(IntPtr hWnd, out RECT lpRect);

    [StructLayout(LayoutKind.Sequential)]
    public struct RECT {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }

    public const uint SWP_NOZORDER = 0x0004;
    public const uint SWP_NOACTIVATE = 0x0010;
}
"@

# Prepare environment with deployed DLLs
$env:PATH = "$root\$TargetDir;" + ($env:PATH -split ';' | Where-Object { $_ -notmatch 'msys64' }) -join ';'

Write-Host "1. Launching $ExeName..."
$proc = Start-Process -FilePath $exePath -ArgumentList "--onboarding" -PassThru

try {
    # Wait for window to be created and obtain handle
    $hwnd = [IntPtr]::Zero
    $waited = 0
    while ($hwnd -eq [IntPtr]::Zero -and $waited -lt 15) {
        Start-Sleep -Milliseconds 500
        $waited += 0.5
        $proc.Refresh()
        if ($proc.HasExited) {
            Write-Error "Process exited prematurely with exit code: $($proc.ExitCode)"
            exit 1
        }
        $hwnd = $proc.MainWindowHandle
    }

    if ($hwnd -eq [IntPtr]::Zero) {
        Write-Error "Could not find main window handle after 15 seconds!"
        exit 1
    }

    Write-Host "2. Target window found (HWND: $hwnd). Initializing soak test..."
    Start-Sleep -Seconds 2
    $proc.Refresh()
    
    $rect = New-Object Win32Window+RECT
    [void][Win32Window]::GetWindowRect($hwnd, [ref]$rect)
    $startX = $rect.Left
    $startY = $rect.Top

    $sw = [System.Diagnostics.Stopwatch]::StartNew()
    $lastSample = 0
    $initialRamMb = [Math]::Round($proc.WorkingSet64 / 1MB, 2)
    $maxRamMb = $initialRamMb
    $resizeCount = 0
    $baseWidth = 1080
    $baseHeight = 640

    Write-Host "Initial Working Set RAM: $initialRamMb MB"
    Write-Host "Running continuous edge/corner resize oscillation at ~40 Hz..."
    Write-Host "----------------------------------------------------------"

    while ($sw.Elapsed.TotalSeconds -lt $DurationSeconds) {
        if ($proc.HasExited) {
            Write-Error "Process crashed or closed during soak test! Exit Code: $($proc.ExitCode)"
            exit 1
        }

        $elapsed = $sw.Elapsed.TotalSeconds
        
        # Smooth oscillation simulating user dragging resize borders back and forth
        $sineVal = [Math]::Sin($elapsed * 3.5)
        $curW = $baseWidth + [int](220 * $sineVal)
        $curH = $baseHeight + [int](140 * $sineVal)

        [void][Win32Window]::SetWindowPos($hwnd, [IntPtr]::Zero, $startX, $startY, $curW, $curH, [Win32Window]::SWP_NOZORDER -bor [Win32Window]::SWP_NOACTIVATE)
        $resizeCount++

        # Sample every $SampleIntervalSeconds
        if (($elapsed - $lastSample) -ge $SampleIntervalSeconds) {
            $lastSample = $elapsed
            $proc.Refresh()
            $curRamMb = [Math]::Round($proc.WorkingSet64 / 1MB, 2)
            if ($curRamMb -gt $maxRamMb) { $maxRamMb = $curRamMb }
            $responding = $proc.Responding

            Write-Host (("[{0:D3}s / {1:D3}s] Resizes: {2:D5} | Size: {3}x{4} | RAM: {5:F2} MB (Peak: {6:F2} MB) | Responsive: {7}") -f `
                [int]$elapsed, $DurationSeconds, $resizeCount, $curW, $curH, $curRamMb, $maxRamMb, $responding)

            if (-not $responding) {
                Write-Warning "App marked as NOT responding by Windows!"
            }
        }

        # ~25ms delay -> ~40 resizes/sec
        Start-Sleep -Milliseconds 25
    }

    $sw.Stop()
    $proc.Refresh()
    $finalRamMb = [Math]::Round($proc.WorkingSet64 / 1MB, 2)
    $ramDiff = [Math]::Round($finalRamMb - $initialRamMb, 2)

    Write-Host "----------------------------------------------------------"
    Write-Host "Soak Test Complete!"
    Write-Host "Total Elapsed Time  : $([Math]::Round($sw.Elapsed.TotalSeconds, 1)) s"
    Write-Host "Total Resize Events : $resizeCount"
    Write-Host "Initial RAM         : $initialRamMb MB"
    Write-Host "Peak RAM            : $maxRamMb MB"
    Write-Host "Final RAM           : $finalRamMb MB"
    Write-Host "RAM Difference      : $ramDiff MB"
    Write-Host "Process Responding  : $($proc.Responding)"
    Write-Host "=========================================================="

    if (-not $proc.Responding) {
        Write-Error "Soak test FAILED: Process is unresponsive!"
        exit 1
    }

    # RAM should not grow unbounded (e.g. runaway leak > 150MB increase)
    if ($ramDiff -gt 150) {
        Write-Error "Soak test FAILED: Potential memory leak detected (RAM grew by $ramDiff MB)!"
        exit 1
    }

    Write-Host "Soak test PASSED: Memory and responsiveness remain stable under continuous resize."
}
finally {
    if ($proc -and -not $proc.HasExited) {
        Write-Host "Stopping Ludelo process..."
        $proc.CloseMainWindow() | Out-Null
        Start-Sleep -Seconds 1
        if (-not $proc.HasExited) {
            $proc.Kill()
        }
    }
}
