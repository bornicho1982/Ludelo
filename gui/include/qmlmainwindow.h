#pragma once

#include "streamsession.h"
#include "settings.h"

#include <QMutex>
#include <QWindow>
#include <QQuickWindow>
#include <QLoggingCategory>
#include <QElapsedTimer>
#include <QTimer>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext_vulkan.h>
#include <libplacebo/options.h>
#include <libplacebo/vulkan.h>
#include <libplacebo/renderer.h>
#include <libplacebo/log.h>
#include <libplacebo/cache.h>
}

#include <vulkan/vulkan.h>
#if defined(Q_OS_LINUX)
#include <xcb/xcb.h>
#include <vulkan/vulkan_xcb.h>
#include <vulkan/vulkan_wayland.h>
#elif defined(Q_OS_MACOS)
#include <vulkan/vulkan_metal.h>
#elif defined(Q_OS_WIN)
#include <vulkan/vulkan_win32.h>
#endif

Q_DECLARE_LOGGING_CATEGORY(chiakiGui);

class Settings;
class StreamSession;
class QmlBackend;
class SteamworksWrapper;  // Always forward-declare for pointer parameters

class QmlMainWindow : public QWindow
{
    Q_OBJECT
    Q_PROPERTY(bool hasVideo READ hasVideo NOTIFY hasVideoChanged)
    Q_PROPERTY(int droppedFrames READ droppedFrames NOTIFY droppedFramesChanged)
    Q_PROPERTY(bool keepVideo READ keepVideo WRITE setKeepVideo NOTIFY keepVideoChanged)
    Q_PROPERTY(VideoMode videoMode READ videoMode WRITE setVideoMode NOTIFY videoModeChanged)
    Q_PROPERTY(float ZoomFactor READ zoomFactor WRITE setZoomFactor NOTIFY zoomFactorChanged)
    Q_PROPERTY(VideoPreset videoPreset READ videoPreset WRITE setVideoPreset NOTIFY videoPresetChanged)
    Q_PROPERTY(bool directStream READ directStream NOTIFY directStreamChanged)

public:
    enum class VideoMode {
        Normal,
        Stretch,
        Zoom
    };
    Q_ENUM(VideoMode);

    enum class VideoPreset {
        Fast,
        Default,
        HighQuality,
        Custom
    };
    Q_ENUM(VideoPreset);

    QmlMainWindow(Settings *settings,  bool exit_app_on_stream_exit = false, SteamworksWrapper *steamworks = nullptr);
    QmlMainWindow(const StreamSessionConnectInfo &connect_info, SteamworksWrapper *steamworks = nullptr);
    QmlMainWindow(Settings *settings, const QString &serviceType, const QString &gameIdentifier, bool exit_app_on_stream_exit = true, SteamworksWrapper *steamworks = nullptr);
    ~QmlMainWindow();
    void updateWindowType(WindowType type);
    void setSettings(Settings *new_settings);

    bool hasVideo() const;
    int droppedFrames() const;

    bool directStream() const;

    bool keepVideo() const;
    void setKeepVideo(bool keep);

    VideoMode videoMode() const;
    void setVideoMode(VideoMode mode);

    float zoomFactor() const;
    void setZoomFactor(float factor);

    bool amdCard() const;
    bool wasMaximized() const { return was_maximized; };
    bool isWindowAdjustable() const { return is_window_adjustable; }
    void setWindowAdjustable(bool adjustable) { is_window_adjustable = adjustable; }

    void fullscreenTime();
    void normalTime();

    bool isStreamWindowAdjustable() { return is_stream_window_adjustable; }
    void setStreamWindowAdjustable(bool adjustable) { is_stream_window_adjustable = adjustable; }

    VideoPreset videoPreset() const;
    void setVideoPreset(VideoPreset mode);

    Q_INVOKABLE void grabInput();
    Q_INVOKABLE void releaseInput();
    Q_INVOKABLE void releaseMouseCapture();
    Q_INVOKABLE void captureMouse();
    Q_INVOKABLE bool isMouseCaptured() const { return mouse_captured; }
    Q_INVOKABLE bool startDrag();
    Q_INVOKABLE void toggleMaximize();
    Q_INVOKABLE bool startResize(int edges);

    void updatePlacebo();
    void show();
    QQmlEngine *getQmlEngine() const { return qml_engine; }
    void presentFrame(AVFrame *frame, int32_t frames_lost);
    void startCloudStreaming(const QString &serviceType, const QString &gameIdentifier);

    AVBufferRef *vulkanHwDeviceCtx();

signals:
    void hasVideoChanged();
    void droppedFramesChanged();
    void keepVideoChanged();
    void videoModeChanged();
    void zoomFactorChanged();
    void videoPresetChanged();
    void menuRequested();
    void directStreamChanged();
    void userActivity();
    void mouseCapturedChanged();

private:
    void init(Settings *settings, bool exit_app_on_stream_exit = false, SteamworksWrapper *steamworks = nullptr);
    void update();
    void scheduleUpdate();
    void createSwapchain();
    void destroySwapchain();
    void resizeSwapchain();
    void updateSwapchain();
    void sync();
    void beginFrame();
    void endFrame();
    void render();
    bool handleShortcut(QKeyEvent *event);
    bool event(QEvent *event) override;
    bool nativeEvent(const QByteArray &eventType, void *message, qintptr *result) override;
    QObject *focusObject() const override;
    void applyNativeWin32FramelessStyles();

    bool m_isResizing = false;
    bool m_inSizeMove = false;
    QElapsedTimer m_resizeThrottleTimer;
    QTimer *m_resizeDebounceTimer = nullptr;

    bool has_video = false;
    bool was_maximized = false;
    bool amd_card = false;
    bool direct_stream = false;
    bool keep_video = false;
    bool mouse_captured = true;
    int grab_input = 0;
    int dropped_frames = 0;
    bool is_window_adjustable = false;
    bool is_stream_window_adjustable = false;
    int dropped_frames_current = 0;
    bool going_full = false;
    QTimer *mouse_hide_timer = nullptr;
    VideoMode video_mode = VideoMode::Normal;
    float zoom_factor = 0;
    VideoPreset video_preset = VideoPreset::HighQuality;
    Settings *settings = {};

    QPoint m_lastMousePos;
    bool m_lastMousePosSet = false;
    bool m_wasGamepadActive = false;

    QmlBackend *backend = {};
    StreamSession *session = {};
    AVBufferRef *vulkan_hw_dev_ctx = nullptr;

    pl_cache placebo_cache = {};
    pl_log placebo_log = {};
    pl_vk_inst placebo_vk_inst = {};
    pl_vulkan placebo_vulkan = {};
    pl_swapchain placebo_swapchain = {};
    pl_renderer placebo_renderer = {};
    std::array<pl_tex, 8> placebo_tex{};
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    int vk_decode_queue_index = -1;
    QSize swapchain_size;
    QMutex frame_mutex;
    QThread *render_thread = {};
    AVFrame *av_frame = {};
    pl_frame current_frame = {};
    pl_frame previous_frame = {};
    std::atomic<bool> render_scheduled = {false};

    QVulkanInstance *qt_vk_inst = {};
    QQmlEngine *qml_engine = {};
    QQuickWindow *quick_window = {};
    QQuickRenderControl *quick_render = {};
    QQuickItem *quick_item = {};
    pl_tex quick_tex = {};
    VkSemaphore quick_sem = VK_NULL_HANDLE;
    uint64_t quick_sem_value = 0;
    QTimer *update_timer = {};
    QTimer *geometry_save_timer = {};
    bool quick_frame = false;
    bool quick_need_sync = false;
    std::atomic<bool> quick_need_render = {false};
    pl_options renderparams_opts = {};
    bool renderparams_changed = false;

    struct {
        PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;
#if defined(Q_OS_LINUX)
        PFN_vkCreateXcbSurfaceKHR vkCreateXcbSurfaceKHR;
        PFN_vkCreateWaylandSurfaceKHR vkCreateWaylandSurfaceKHR;
#elif defined(Q_OS_MACOS)
        PFN_vkCreateMetalSurfaceEXT vkCreateMetalSurfaceEXT;
#elif defined(Q_OS_WIN32)
        PFN_vkCreateWin32SurfaceKHR vkCreateWin32SurfaceKHR;
#endif
        PFN_vkDestroySurfaceKHR vkDestroySurfaceKHR;
        PFN_vkWaitSemaphores vkWaitSemaphores;
        PFN_vkGetPhysicalDeviceQueueFamilyProperties vkGetPhysicalDeviceQueueFamilyProperties;
        PFN_vkGetPhysicalDeviceProperties vkGetPhysicalDeviceProperties;
    } vk_funcs;

    friend class QmlBackend;
};
