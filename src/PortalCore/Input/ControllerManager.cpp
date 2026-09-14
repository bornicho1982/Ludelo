// Archivo: src/PortalCore/Input/ControllerManager.cpp
#include "ControllerManager.h"
#include <spdlog/spdlog.h>
#include <SDL3/SDL.h>
#include <cctype>
#include <cmath>
#include <algorithm>

namespace portal::input {

ControllerManager::ControllerManager() = default;

ControllerManager::~ControllerManager() {
    std::lock_guard<std::mutex> lock(m_mutex);
    close_controller();
    SDL_QuitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC);
}

portal::VoidResult ControllerManager::init() {
    SDL_SetHint(SDL_HINT_JOYSTICK_ENHANCED_REPORTS, "1");
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");

    if (!SDL_InitSubSystem(SDL_INIT_GAMEPAD | SDL_INIT_HAPTIC)) {
        spdlog::warn("[ControllerManager] SDL_InitSubSystem Gamepad: {}", SDL_GetError());
    }

    int mappings_count = SDL_AddGamepadMappingsFromFile("assets/gamecontrollerdb.txt");
    if (mappings_count < 0) {
        mappings_count = SDL_AddGamepadMappingsFromFile("bin/assets/gamecontrollerdb.txt");
    }
    if (mappings_count >= 0) {
        spdlog::info("[ControllerManager] Loaded {} mappings from gamecontrollerdb.txt", mappings_count);
    } else {
        spdlog::warn("[ControllerManager] gamecontrollerdb.txt could not be loaded");
    }

    std::lock_guard<std::mutex> lock(m_mutex);
    check_and_open_controller();
    return {};
}

void ControllerManager::check_and_open_controller() {
    if ((m_dualsense && m_dualsense->is_open()) || m_sdl_gamepad != nullptr) {
        return;
    }

    // 1. Try to find a DualSense via native HID for advanced adaptive triggers & haptics
    auto ds_list = DualSenseHID::enumerate();
    if (!ds_list.empty()) {
        m_dualsense = std::make_unique<DualSenseHID>();
        if (m_dualsense->open(ds_list[0].device_path)) {
            m_is_dualsense = true;
            m_controller_name = "DualSense Wireless Controller (Native)";
            spdlog::info("[ControllerManager] Inicializado DualSense nativo USB/BT");
            return;
        }
        m_dualsense.reset();
    }

    // 2. Open any connected gamepad via SDL3 (Xbox, DualSense, DualShock 4, Switch Pro, Generic)
    int count = 0;
    SDL_JoystickID* joysticks = SDL_GetGamepads(&count);
    if (count > 0 && joysticks != nullptr) {
        m_sdl_gamepad = SDL_OpenGamepad(joysticks[0]);
        if (m_sdl_gamepad) {
            const char* name = SDL_GetGamepadName(m_sdl_gamepad);
            m_controller_name = name ? name : "Mando Compatible";
            std::string name_lower = m_controller_name;
            for (auto& c : name_lower) c = static_cast<char>(std::tolower(c));
            m_is_dualsense = (name_lower.find("dualsense") != std::string::npos);
            if (m_enable_gyro && SDL_GamepadHasSensor(m_sdl_gamepad, SDL_SENSOR_GYRO)) {
                SDL_SetGamepadSensorEnabled(m_sdl_gamepad, SDL_SENSOR_GYRO, true);
                spdlog::info("[ControllerManager] Sensor giroscopio activado en mando");
            }
            spdlog::info("[ControllerManager] Mando conectado (SDL3): {} [ID: {}]", m_controller_name, joysticks[0]);
            SDL_free(joysticks);
            return;
        }
    }
    if (joysticks) SDL_free(joysticks);

    m_controller_name = "Sin mando";
    m_is_dualsense = false;
}

void ControllerManager::close_controller() {
    if (m_sdl_gamepad) {
        SDL_CloseGamepad(m_sdl_gamepad);
        m_sdl_gamepad = nullptr;
    }
    if (m_dualsense) {
        m_dualsense->close();
        m_dualsense.reset();
    }
    m_is_dualsense = false;
    m_controller_name = "Sin mando";
}

void ControllerManager::handle_sdl_event(const SDL_Event& event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
        spdlog::info("[ControllerManager] Evento GAMEPAD_ADDED detectado (ID: {})", event.gdevice.which);
        if (!m_dualsense && !m_sdl_gamepad) {
            check_and_open_controller();
        }
    } else if (event.type == SDL_EVENT_GAMEPAD_REMOVED) {
        spdlog::info("[ControllerManager] Evento GAMEPAD_REMOVED detectado (ID: {})", event.gdevice.which);
        if (m_sdl_gamepad && SDL_GetGamepadID(m_sdl_gamepad) == event.gdevice.which) {
            close_controller();
            check_and_open_controller();
        }
    }
}

ControllerState ControllerManager::poll() {
    std::lock_guard<std::mutex> lock(m_mutex);
    ControllerState state{};

    if (!m_window_focused) {
        return state;
    }

    if (!m_dualsense && !m_sdl_gamepad) {
        check_and_open_controller();
    }

    if (m_dualsense && m_dualsense->is_open()) {
        auto ds_state = m_dualsense->poll();
        if (ds_state) {
            state.left_stick_x  = static_cast<int16_t>((static_cast<int32_t>(ds_state->left_stick_x) - 128) * 256);
            state.left_stick_y  = static_cast<int16_t>((static_cast<int32_t>(ds_state->left_stick_y) - 128) * 256);
            state.right_stick_x = static_cast<int16_t>((static_cast<int32_t>(ds_state->right_stick_x) - 128) * 256);
            state.right_stick_y = static_cast<int16_t>((static_cast<int32_t>(ds_state->right_stick_y) - 128) * 256);
            
            state.l2 = ds_state->l2;
            state.r2 = ds_state->r2;

            state.dpad_up    = ds_state->dpad_up;
            state.dpad_down  = ds_state->dpad_down;
            state.dpad_left  = ds_state->dpad_left;
            state.dpad_right = ds_state->dpad_right;

            state.square   = ds_state->square;
            state.cross    = ds_state->cross;
            state.circle   = ds_state->circle;
            state.triangle = ds_state->triangle;
            state.l1       = ds_state->l1;
            state.r1       = ds_state->r1;
            state.l3       = ds_state->l3;
            state.r3       = ds_state->r3;
            
            state.share        = ds_state->share;
            state.options      = ds_state->options;
            state.ps_btn       = ds_state->ps_btn;
            state.touchpad_btn = ds_state->touchpad_btn;
        }
    } else if (m_sdl_gamepad) {
        fallback_poll_sdl(state);
    }

    // Apply radial deadzone to eliminate stick drift before returning state
    apply_stick_deadzone(state.left_stick_x, state.left_stick_y);
    apply_stick_deadzone(state.right_stick_x, state.right_stick_y);

    // Teclado fallback para control inmediato con teclado de PC
    const bool* keys = SDL_GetKeyboardState(nullptr);
    if (keys) {
        if (keys[SDL_SCANCODE_W]) state.left_stick_y = -32767;
        if (keys[SDL_SCANCODE_S]) state.left_stick_y = 32767;
        if (keys[SDL_SCANCODE_A]) state.left_stick_x = -32767;
        if (keys[SDL_SCANCODE_D]) state.left_stick_x = 32767;

        if (keys[SDL_SCANCODE_UP])    state.dpad_up = true;
        if (keys[SDL_SCANCODE_DOWN])  state.dpad_down = true;
        if (keys[SDL_SCANCODE_LEFT])  state.dpad_left = true;
        if (keys[SDL_SCANCODE_RIGHT]) state.dpad_right = true;

        if (keys[SDL_SCANCODE_SPACE] || keys[SDL_SCANCODE_RETURN]) state.cross = true;
        if (keys[SDL_SCANCODE_BACKSPACE]) state.circle = true;
        if (keys[SDL_SCANCODE_F]) state.square = true;
        if (keys[SDL_SCANCODE_R]) state.triangle = true;
        if (keys[SDL_SCANCODE_Q]) state.l1 = true;
        if (keys[SDL_SCANCODE_E]) state.r1 = true;
        if (keys[SDL_SCANCODE_TAB]) state.options = true;
        if (keys[SDL_SCANCODE_C]) state.share = true;
        if (keys[SDL_SCANCODE_T]) state.touchpad_btn = true;
        if (keys[SDL_SCANCODE_F1] || keys[SDL_SCANCODE_HOME] || keys[SDL_SCANCODE_P] || keys[SDL_SCANCODE_GRAVE]) {
            state.ps_btn = true;
        }
    }

    return state;
}

void ControllerManager::fallback_poll_sdl(ControllerState& state) {
    if (!m_sdl_gamepad) return;
    
    // Sticks directos 16-bit (-32768 a 32767) idéntico a Chiaki/Pylux
    state.left_stick_x  = SDL_GetGamepadAxis(m_sdl_gamepad, SDL_GAMEPAD_AXIS_LEFTX);
    state.left_stick_y  = SDL_GetGamepadAxis(m_sdl_gamepad, SDL_GAMEPAD_AXIS_LEFTY);
    state.right_stick_x = SDL_GetGamepadAxis(m_sdl_gamepad, SDL_GAMEPAD_AXIS_RIGHTX);
    state.right_stick_y = SDL_GetGamepadAxis(m_sdl_gamepad, SDL_GAMEPAD_AXIS_RIGHTY);

    // Gatillos directos 8-bit (0 a 255) mediante shift de 7 bits (0..32767 >> 7)
    int16_t l2_raw = SDL_GetGamepadAxis(m_sdl_gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER);
    state.l2 = (l2_raw > 0) ? static_cast<uint8_t>(l2_raw >> 7) : 0;

    int16_t r2_raw = SDL_GetGamepadAxis(m_sdl_gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER);
    state.r2 = (r2_raw > 0) ? static_cast<uint8_t>(r2_raw >> 7) : 0;

    // Botones digitales estándar compatibles con cualquier mando (Xbox, DualSense, Switch, Genérico)
    auto get_btn = [&](SDL_GamepadButton btn) -> bool {
        return SDL_GetGamepadButton(m_sdl_gamepad, btn) != 0;
    };

    state.cross    = get_btn(SDL_GAMEPAD_BUTTON_SOUTH);
    state.circle   = get_btn(SDL_GAMEPAD_BUTTON_EAST);
    state.square   = get_btn(SDL_GAMEPAD_BUTTON_WEST);
    state.triangle = get_btn(SDL_GAMEPAD_BUTTON_NORTH);

    state.dpad_up    = get_btn(SDL_GAMEPAD_BUTTON_DPAD_UP);
    state.dpad_down  = get_btn(SDL_GAMEPAD_BUTTON_DPAD_DOWN);
    state.dpad_left  = get_btn(SDL_GAMEPAD_BUTTON_DPAD_LEFT);
    state.dpad_right = get_btn(SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    
    state.l1 = get_btn(SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
    state.r1 = get_btn(SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
    state.l3 = get_btn(SDL_GAMEPAD_BUTTON_LEFT_STICK);
    state.r3 = get_btn(SDL_GAMEPAD_BUTTON_RIGHT_STICK);
    
    state.share        = get_btn(SDL_GAMEPAD_BUTTON_BACK);
    state.options      = get_btn(SDL_GAMEPAD_BUTTON_START);
    // PS Button: Guide button OR Options+Share chord (for Xbox and third-party gamepads)
    state.ps_btn       = get_btn(SDL_GAMEPAD_BUTTON_GUIDE) || 
                         (get_btn(SDL_GAMEPAD_BUTTON_START) && get_btn(SDL_GAMEPAD_BUTTON_BACK));
    
    bool has_tp = get_btn(SDL_GAMEPAD_BUTTON_TOUCHPAD);
    // On controllers without physical touchpad button (e.g. Xbox), touchpad click maps to Back/Share button
    state.touchpad_btn = has_tp || (!m_is_dualsense && get_btn(SDL_GAMEPAD_BUTTON_BACK));

    if (m_enable_gyro && SDL_GamepadHasSensor(m_sdl_gamepad, SDL_SENSOR_GYRO)) {
        float gyro[3] = {0.0f, 0.0f, 0.0f};
        if (SDL_GetGamepadSensorData(m_sdl_gamepad, SDL_SENSOR_GYRO, gyro, 3)) {
            state.has_gyro = true;
            state.gyro_x = gyro[0];
            state.gyro_y = gyro[1];
            state.gyro_z = gyro[2];
        }
    }
}

void ControllerManager::on_haptic(const HapticEvent& event) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_dualsense && m_dualsense->is_open()) {
        uint8_t left = static_cast<uint8_t>(event.left_intensity * 255.0f);
        uint8_t right = static_cast<uint8_t>(event.right_intensity * 255.0f);
        (void)m_dualsense->set_vibration(left, right);
    } else if (m_sdl_gamepad) {
        uint16_t left = static_cast<uint16_t>(event.left_intensity * 65535.0f);
        uint16_t right = static_cast<uint16_t>(event.right_intensity * 65535.0f);
        SDL_RumbleGamepad(m_sdl_gamepad, left, right, event.duration_ms);
    }
}

void ControllerManager::on_trigger_effect(bool left_trigger, const TriggerEffect& effect) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_dualsense && m_dualsense->is_open()) {
        (void)m_dualsense->set_trigger_effect(left_trigger, effect);
    } else if (m_sdl_gamepad && m_is_dualsense) {
        uint8_t data[11];
        data[0] = static_cast<uint8_t>(effect.mode);
        std::memcpy(&data[1], effect.params, 10);
        SDL_SendGamepadEffect(m_sdl_gamepad, data, sizeof(data));
    }
}

std::string ControllerManager::get_controller_name() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_controller_name;
}

bool ControllerManager::is_dualsense() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_is_dualsense;
}

bool ControllerManager::is_connected() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return (m_dualsense && m_dualsense->is_open()) || (m_sdl_gamepad != nullptr);
}

void ControllerManager::set_stick_deadzone(float deadzone) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_stick_deadzone = std::clamp(deadzone, 0.0f, 0.95f);
    spdlog::info("[Input] Applied stick radial deadzone: {:.2f}", m_stick_deadzone);
}

float ControllerManager::get_stick_deadzone() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_stick_deadzone;
}

void ControllerManager::apply_stick_deadzone(int16_t& x, int16_t& y) const {
    if (m_stick_deadzone <= 0.001f) return;

    float nx = static_cast<float>(x) / 32767.0f;
    float ny = static_cast<float>(y) / 32767.0f;

    float mag = std::sqrt(nx * nx + ny * ny);
    if (mag <= m_stick_deadzone) {
        x = 0;
        y = 0;
    } else {
        float factor = (mag - m_stick_deadzone) / (1.0f - m_stick_deadzone);
        factor = std::clamp(factor, 0.0f, 1.0f);

        float scaled_x = (nx / mag) * factor;
        float scaled_y = (ny / mag) * factor;

        x = static_cast<int16_t>(std::clamp(scaled_x * 32767.0f, -32767.0f, 32767.0f));
        y = static_cast<int16_t>(std::clamp(scaled_y * 32767.0f, -32767.0f, 32767.0f));
    }
}

void ControllerManager::set_enable_gyro(bool enable) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_enable_gyro = enable;
    if (m_sdl_gamepad && SDL_GamepadHasSensor(m_sdl_gamepad, SDL_SENSOR_GYRO)) {
        SDL_SetGamepadSensorEnabled(m_sdl_gamepad, SDL_SENSOR_GYRO, enable);
        spdlog::info("[ControllerManager] Gyroscope sensor set to: {}", enable);
    }
}

bool ControllerManager::get_enable_gyro() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_enable_gyro;
}

void ControllerManager::set_window_focused(bool focused) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_window_focused = focused;
}

bool ControllerManager::is_window_focused() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_window_focused;
}

} // namespace portal::input
