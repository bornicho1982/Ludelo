// Archivo: src/UI/App.h
// Ludelo — Main Application (ImGui + Vulkan)
#pragma once

#include "PortalCore/Common.h"

#include <vulkan/vulkan.h>
#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>
#include <imgui.h>

#include <filesystem>
#include <functional>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "PortalCore/Auth/AccountManager.h"
#include "PortalCore/Discovery/ConsoleRegistry.h"
#include "PortalCore/Input/ControllerManager.h"
#include "PortalCore/Stream/SessionManager.h"
#include "PortalCore/Config/AppSettings.h"

namespace portal::ui {

struct AppConfig {
    std::filesystem::path app_data_dir;
    std::string window_title = "Ludelo";
    uint32_t window_width = 1280;
    uint32_t window_height = 720;
    bool fullscreen = false;
    bool vsync = true;
};

// Forward declarations
class HomeScreen;
class StreamingScreen;
class SettingsScreen;
class NavigationController;

enum class Screen {
    Onboarding,
    Home,
    ConsoleList,
    CloudGames,
    GameDetail,
    Streaming,
    Settings,
    Account,
    Registration,
    Login,
    Register,
};

class App {
public:
    explicit App(const AppConfig& config);
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    VoidResult init();
    void run();
    void request_quit();
    void navigate_to(Screen screen);

    [[nodiscard]] const AppConfig& config() const { return config_; }
    [[nodiscard]] SDL_Window* window() const { return window_; }
    [[nodiscard]] bool is_running() const { return running_; }

private:
    // Vulkan setup
    VoidResult init_vulkan();
    VoidResult create_instance();
    VoidResult select_physical_device();
    VoidResult create_logical_device();
    VoidResult create_surface();
    VoidResult create_swapchain();
    VoidResult create_render_pass();
    VoidResult create_framebuffers();
    VoidResult create_command_pool();
    VoidResult create_sync_objects();

    // ImGui setup
    VoidResult init_imgui();
    void setup_style();
    void load_fonts();

    // Frame rendering
    bool begin_frame();
    void render_ui();
    void end_frame();
    void present();

    // UI screens & widgets
    void draw_ambient_background();
    void draw_top_navigation_bar();
    void draw_home_screen();
    void draw_console_list();
    void draw_cloud_games();
    void draw_registration_screen();
    void draw_pin_modal();
    void draw_login_pin_modal();
    void draw_settings();
    void draw_streaming_overlay();
    void draw_status_bar();
    void draw_toast_notification();
    void draw_onboarding();
    void show_toast(const std::string& message, float duration = 3.5f);

    void probe_consoles_background();
    void wake_and_connect(const portal::discovery::RegisteredConsole& console);

    // Cleanup
    void cleanup_vulkan();
    void cleanup_imgui();

    AppConfig config_;
    bool running_ = false;
    Screen current_screen_ = Screen::Home;

    // SDL
    SDL_Window* window_ = nullptr;

    // Vulkan objects
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue graphics_queue_ = VK_NULL_HANDLE;
    VkQueue present_queue_ = VK_NULL_HANDLE;
    VkSurfaceKHR surface_ = VK_NULL_HANDLE;
    VkSwapchainKHR swapchain_ = VK_NULL_HANDLE;
    VkRenderPass render_pass_ = VK_NULL_HANDLE;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkDescriptorPool imgui_descriptor_pool_ = VK_NULL_HANDLE;

    uint32_t graphics_queue_family_ = 0;
    uint32_t present_queue_family_ = 0;

    struct FrameData {
        VkCommandBuffer command_buffer = VK_NULL_HANDLE;
        VkFence fence = VK_NULL_HANDLE;
        VkSemaphore image_available = VK_NULL_HANDLE;
        VkSemaphore render_finished = VK_NULL_HANDLE;
        VkFramebuffer framebuffer = VK_NULL_HANDLE;
    };

    std::vector<VkImage> swapchain_images_;
    std::vector<VkImageView> swapchain_image_views_;
    std::vector<FrameData> frames_;
    VkFormat swapchain_format_ = VK_FORMAT_B8G8R8A8_SRGB;
    VkExtent2D swapchain_extent_{};
    uint32_t current_frame_ = 0;
    uint32_t image_index_ = 0;
    bool swapchain_needs_rebuild_ = false;

    // Backend Managers
    std::shared_ptr<portal::auth::AccountManager> account_manager_;
    std::shared_ptr<portal::discovery::ConsoleRegistry> console_registry_;
    std::shared_ptr<portal::input::ControllerManager> controller_manager_;
    std::shared_ptr<portal::stream::SessionManager> session_manager_;

    // Vulkan texture management
    struct VulkanTexture {
        VkImage image = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkImageView view = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VkDescriptorSet descriptor_set = VK_NULL_HANDLE;
        int width = 0;
        int height = 0;
    };
    std::unordered_map<std::string, VulkanTexture> textures_;
    bool load_texture(const std::string& name, const std::string& filepath);
    void load_all_icons();
    void destroy_textures();
    ImTextureID get_texture(const std::string& name);

    // Dynamic Video Streaming Surface
    VulkanTexture stream_texture_{};
    VkBuffer stream_staging_buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory stream_staging_memory_ = VK_NULL_HANDLE;
    void* stream_staging_mapped_ = nullptr;
    VkCommandBuffer stream_upload_cmd_ = VK_NULL_HANDLE;
    VkFence stream_upload_fence_ = VK_NULL_HANDLE;
    bool stream_texture_initialized_ = false;
    int stream_w_ = 1920;
    int stream_h_ = 1080;
    std::mutex stream_frame_mutex_;
    std::vector<uint32_t> stream_pixel_cache_;
    bool stream_has_new_frame_ = false;
    // Audio output
    SDL_AudioStream* audio_stream_ = nullptr;

    bool init_stream_texture(int width, int height);
    void update_stream_texture(const uint32_t* pixels, int width, int height);
    void destroy_stream_texture();

    // Font pointers (NotoSansCJK / system fallback)
    ImFont* font_title_ = nullptr;
    ImFont* font_subtitle_ = nullptr;
    ImFont* font_body_ = nullptr;
    ImFont* font_small_ = nullptr;
    ImFont* font_pin_ = nullptr;

    // Navigation & UI State
    int current_tab_ = 0; // 0: Consolas, 1: PS Plus Cloud, 2: Ajustes
    int focused_card_ = 0;
    bool show_pin_modal_ = false;
    int active_pin_digit_ = 0;
    char pin_digits_[8] = {'\0'};
    char register_ip_[32] = "";
    char register_name_[64] = "";
    char account_id_b64_[64] = ""; // Added for PS5 Registration
    std::string toast_message_;
    float toast_timer_ = 0.0f;
    int cloud_selected_category_ = 0;
    int cloud_selected_game_ = -1;
    char cloud_search_[128] = "";

    // 4-Digit PS5 User Login PIN
    bool show_login_pin_modal_ = false;
    char login_pin_digits_[4] = {'\0', '\0', '\0', '\0'};
    int active_login_pin_digit_ = 0;
    bool remember_login_pin_ = true;
    std::string target_login_console_host_id_;

    // Background probing & Wakeup
    float last_probe_time_ = 0.0f;
    std::atomic<bool> is_probing_{false};
    std::atomic<bool> is_waking_{false};
    std::string waking_status_text_;
    std::vector<portal::discovery::DiscoveredConsole> unbound_consoles_;

    // Streaming Fullscreen & HUD Overlay Auto-hide
    float last_mouse_activity_time_ = 0.0f;
    bool stream_hud_visible_ = true;

    // Windows 11 Mica backdrop state
    bool mica_active_ = false;

    // Persistent application settings (loaded from disk at startup)
    portal::config::AppSettings settings_;
    std::filesystem::path settings_path_;

    // Background managed threads (jthread ensures clean join upon reassignment or exit)
    std::jthread probe_thread_;
    std::jthread connect_thread_;
    std::jthread register_thread_;
    std::jthread login_thread_;
    std::jthread avatar_thread_;
    std::atomic<bool> avatar_downloaded_{false};
    std::string avatar_path_;
    std::jthread suspend_thread_;
    std::jthread cloud_thread_;
};

}  // namespace portal::ui
