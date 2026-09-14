// Archivo: src/PortalCore/Input/ControllerManager.h
#pragma once

#include "PortalCore/Input/DualSenseHID.h"
#include <string>
#include <memory>
#include <expected>
#include <mutex>
#include <cstdint>

// Forward declarations to avoid hard dependency here
struct SDL_Gamepad;
union SDL_Event;

namespace portal::input {

struct ControllerState {
    int16_t left_stick_x{0}, left_stick_y{0};
    int16_t right_stick_x{0}, right_stick_y{0};
    uint8_t l2{0}, r2{0};

    bool dpad_up{false}, dpad_down{false}, dpad_left{false}, dpad_right{false};
    bool square{false}, cross{false}, circle{false}, triangle{false};
    bool l1{false}, r1{false}, l3{false}, r3{false};
    bool share{false}, options{false}, ps_btn{false}, touchpad_btn{false};
    float gyro_x{0.0f}, gyro_y{0.0f}, gyro_z{0.0f};
    bool has_gyro{false};
};

struct HapticEvent {
    float left_intensity{0.0f};
    float right_intensity{0.0f};
    uint32_t duration_ms{0};
};

class ControllerManager {
public:
    ControllerManager();
    ~ControllerManager();

    portal::VoidResult init();
    ControllerState poll();
    
    void handle_sdl_event(const SDL_Event& event);
    
    void on_haptic(const HapticEvent& event);
    void on_trigger_effect(bool left_trigger, const TriggerEffect& effect);
    
    std::string get_controller_name() const;
    bool is_dualsense() const;
    bool is_connected() const;

    void set_stick_deadzone(float deadzone);
    float get_stick_deadzone() const;

    void set_enable_gyro(bool enable);
    bool get_enable_gyro() const;

    void set_window_focused(bool focused);
    bool is_window_focused() const;

private:
    void check_and_open_controller();
    void close_controller();
    void fallback_poll_sdl(ControllerState& state);
    void apply_stick_deadzone(int16_t& x, int16_t& y) const;

    mutable std::mutex m_mutex;
    SDL_Gamepad* m_sdl_gamepad{nullptr};
    std::unique_ptr<DualSenseHID> m_dualsense;
    bool m_is_dualsense{false};
    std::string m_controller_name{"Sin mando"};
    float m_stick_deadzone{0.15f};
    bool m_enable_gyro{false};
    bool m_window_focused{true};
};

} // namespace portal::input
