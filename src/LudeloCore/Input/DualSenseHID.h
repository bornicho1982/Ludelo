// Archivo: src/LudeloCore/Input/DualSenseHID.h
#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <expected>
#include <memory>
#include "LudeloCore/Common.h"

namespace ludelo::input {

struct DualSenseInfo {
    std::string device_path;
    std::string serial;
    bool is_bluetooth;
};

struct TouchPoint {
    bool active;
    uint8_t id;
    uint16_t x;
    uint16_t y;
};

struct DualSenseState {
    uint8_t left_stick_x;
    uint8_t left_stick_y;
    uint8_t right_stick_x;
    uint8_t right_stick_y;
    uint8_t l2;
    uint8_t r2;
    uint8_t sequence;
    
    // Buttons
    bool dpad_up, dpad_down, dpad_left, dpad_right;
    bool square, cross, circle, triangle;
    bool l1, r1, l2_btn, r2_btn;
    bool share, options, l3, r3;
    bool ps_btn, touchpad_btn, mute_btn;

    int16_t gyro_x, gyro_y, gyro_z;
    int16_t accel_x, accel_y, accel_z;

    TouchPoint touch1;
    TouchPoint touch2;

    uint8_t battery_level;
    bool is_charging;
};

enum class TriggerMode : uint8_t {
    Off = 0x00,
    Feedback = 0x01,
    Weapon = 0x02,
    Vibration = 0x06
};

struct TriggerEffect {
    TriggerMode mode{TriggerMode::Off};
    uint8_t params[10]{};
};

class DualSenseHID {
public:
    DualSenseHID();
    ~DualSenseHID();

    static std::vector<DualSenseInfo> enumerate();
    
    ludelo::VoidResult open(const std::string& device_path);
    void close();
    
    ludelo::Result<DualSenseState> poll();
    
    ludelo::VoidResult set_vibration(uint8_t left_low, uint8_t right_high);
    ludelo::VoidResult set_lightbar(uint8_t r, uint8_t g, uint8_t b);
    ludelo::VoidResult set_trigger_effect(bool left_trigger, const TriggerEffect& effect);
    ludelo::VoidResult set_player_leds(uint8_t leds_mask);
    
    bool is_bluetooth() const;
    bool is_open() const;

private:
    ludelo::VoidResult send_output_report();
    uint32_t calculate_crc32(const uint8_t* data, size_t length);

    HANDLE m_device{INVALID_HANDLE_VALUE};
    bool m_is_bluetooth{false};
    
    // Output state
    uint8_t m_motor_left{0};
    uint8_t m_motor_right{0};
    uint8_t m_lightbar_r{0};
    uint8_t m_lightbar_g{0};
    uint8_t m_lightbar_b{0};
    uint8_t m_player_leds{0};
    TriggerEffect m_left_trigger;
    TriggerEffect m_right_trigger;
    
    bool m_output_dirty{false};
};

} // namespace ludelo::input
