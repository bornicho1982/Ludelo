$WshShell = New-Object -ComObject WScript.Shell
$desktopPath = [System.Environment]::GetFolderPath('Desktop')
$rootPath = (Resolve-Path "$PSScriptRoot\..").Path
$Shortcut = $WshShell.CreateShortcut("$desktopPath\Ludelo.lnk")
$Shortcut.TargetPath = "$rootPath\bin\Ludelo.exe"
$Shortcut.WorkingDirectory = "$rootPath\bin"
$Shortcut.Description = "Ludelo - PlayStation Remote Play para PC"
$Shortcut.Save()

Write-Host "Acceso directo creado exitosamente en el Escritorio: $desktopPath\Ludelo.lnk"
