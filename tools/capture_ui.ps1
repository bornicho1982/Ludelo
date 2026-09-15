Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing

$proc = Start-Process -FilePath 'bin\Ludelo.exe' -WorkingDirectory 'bin' -PassThru
Start-Sleep -Seconds 3

$bounds = [System.Windows.Forms.Screen]::PrimaryScreen.Bounds
$bmp = New-Object System.Drawing.Bitmap $bounds.Width, $bounds.Height
$graphics = [System.Drawing.Graphics]::FromImage($bmp)
$graphics.CopyFromScreen($bounds.Location, [System.Drawing.Point]::Empty, $bounds.Size)
$screenshotPath = 'C:\Users\borni\.gemini\antigravity\brain\0745dfce-c004-4e55-bb4c-48471980fe85\portal_new_ui.png'
$bmp.Save($screenshotPath, [System.Drawing.Imaging.ImageFormat]::Png)
$graphics.Dispose()
$bmp.Dispose()

Write-Host "Screenshot saved successfully to: $screenshotPath"
Stop-Process -Id $proc.Id -Force
