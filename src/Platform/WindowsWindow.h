// Archivo: src/Platform/WindowsWindow.h
#pragma once

#include "PortalCore/Common.h"
#include <string>

namespace portal::platform {

/// Platform-specific window utilities for Windows
class WindowsWindow {
public:
    /// Get the executable directory
    static std::string get_exe_directory();
    
    /// Get system DPI scaling factor
    static float get_dpi_scale();
    
    /// Show a native message box
    static void show_error_dialog(const std::string& title, const std::string& message);
    
    /// Open a URL in the default browser
    static void open_url(const std::string& url);
    
    /// Check if running under a game compositor (e.g., Steam Big Picture)
    static bool is_compositor_mode();
    
    /// Get system GPU name
    static std::string get_gpu_name();
    
    /// Check for HDR display support
    static bool is_hdr_display_available();

    /// Apply Windows 11 Mica backdrop material to a Win32 window.
    /// Returns true if Mica was applied, false if OS doesn't support it (pre-Win11 build 22000).
    /// Must be called after the window is shown.
    static bool apply_mica_backdrop(void* hwnd);

    /// Remove Mica/Acrylic and restore default (opaque) backdrop.
    static void remove_mica_backdrop(void* hwnd);
};

}  // namespace portal::platform
