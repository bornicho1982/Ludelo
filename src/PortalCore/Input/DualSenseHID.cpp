#include "PortalCore/Input/DualSenseHID.h"
#include <spdlog/spdlog.h>
#include <hidsdi.h>
#include <hidpi.h>
#include <SetupAPI.h>

#pragma comment(lib, "hid.lib")
#pragma comment(lib, "setupapi.lib")

namespace portal::input {

constexpr uint16_t SONY_VID = 0x054C;
constexpr uint16_t DUALSENSE_PID = 0x0CE6;
constexpr uint16_t DUALSENSE_EDGE_PID = 0x0DF2;

DualSenseHID::DualSenseHID() = default;

DualSenseHID::~DualSenseHID() {
    close();
}

std::vector<DualSenseInfo> DualSenseHID::enumerate() {
    std::vector<DualSenseInfo> controllers;
    GUID hid_guid;
    HidD_GetHidGuid(&hid_guid);

    HDEVINFO dev_info = SetupDiGetClassDevsA(&hid_guid, nullptr, nullptr, DIGCF_PRESENT | DIGCF_DEVICEINTERFACE);
    if (dev_info == INVALID_HANDLE_VALUE) {
        spdlog::get("portal")->error("Failed to get HID device info set");
        return controllers;
    }

    SP_DEVICE_INTERFACE_DATA interface_data{};
    interface_data.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

    for (DWORD i = 0; SetupDiEnumDeviceInterfaces(dev_info, nullptr, &hid_guid, i, &interface_data); ++i) {
        DWORD required_size = 0;
        SetupDiGetDeviceInterfaceDetailA(dev_info, &interface_data, nullptr, 0, &required_size, nullptr);

        std::vector<uint8_t> detail_buffer(required_size);
        auto detail_data = reinterpret_cast<PSP_DEVICE_INTERFACE_DETAIL_DATA_A>(detail_buffer.data());
        detail_data->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);

        if (SetupDiGetDeviceInterfaceDetailA(dev_info, &interface_data, detail_data, required_size, nullptr, nullptr)) {
            HANDLE device = CreateFileA(detail_data->DevicePath, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr);
            if (device != INVALID_HANDLE_VALUE) {
                HIDD_ATTRIBUTES attrib{};
                attrib.Size = sizeof(HIDD_ATTRIBUTES);
                if (HidD_GetAttributes(device, &attrib)) {
                    if (attrib.VendorID == SONY_VID && (attrib.ProductID == DUALSENSE_PID || attrib.ProductID == DUALSENSE_EDGE_PID)) {
                        DualSenseInfo info;
                        info.device_path = detail_data->DevicePath;
                        // For simplicity, determine Bluetooth by looking at string or just assume USB for now.
                        // Actually, Windows abstracts this. A real check might involve HID caps or strings.
                        info.is_bluetooth = (std::string(detail_data->DevicePath).find("ig_") == std::string::npos); // Dummy logic
                        
                        wchar_t serial_w[256]{};
                        if (HidD_GetSerialNumberString(device, serial_w, sizeof(serial_w))) {
                            char serial_a[256]{};
                            WideCharToMultiByte(CP_UTF8, 0, serial_w, -1, serial_a, sizeof(serial_a), nullptr, nullptr);
                            info.serial = serial_a;
                        }
                        controllers.push_back(info);
                        spdlog::get("portal")->info("Found DualSense: {}", info.device_path);
                    }
                }
                CloseHandle(device);
            }
        }
    }
    SetupDiDestroyDeviceInfoList(dev_info);
    return controllers;
}

portal::VoidResult DualSenseHID::open(const std::string& device_path) {
    if (m_device != INVALID_HANDLE_VALUE) close();
    
    m_device = CreateFileA(device_path.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    if (m_device == INVALID_HANDLE_VALUE) {
        m_device = CreateFileA(device_path.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, nullptr);
    }
    if (m_device == INVALID_HANDLE_VALUE) {
        spdlog::error("Failed to open DualSense device: {}", device_path);
        return std::unexpected(portal::Error{portal::ErrorCode::HIDOpenFailed, "Failed to open DualSense device"});
    }
    
    // A more precise Bluetooth detection could be reading the HID caps for max input report size (64 vs 78)
    PHIDP_PREPARSED_DATA preparsed;
    if (HidD_GetPreparsedData(m_device, &preparsed)) {
        HIDP_CAPS caps;
        HidP_GetCaps(preparsed, &caps);
        m_is_bluetooth = (caps.InputReportByteLength > 64);
        HidD_FreePreparsedData(preparsed);
    }

    spdlog::get("portal")->info("DualSense opened. Bluetooth: {}", m_is_bluetooth);
    return {};
}

void DualSenseHID::close() {
    if (m_device != INVALID_HANDLE_VALUE) {
        CloseHandle(m_device);
        m_device = INVALID_HANDLE_VALUE;
    }
}

bool DualSenseHID::is_bluetooth() const { return m_is_bluetooth; }
bool DualSenseHID::is_open() const { return m_device != INVALID_HANDLE_VALUE; }

portal::Result<DualSenseState> DualSenseHID::poll() {
    if (!is_open()) return std::unexpected(portal::Error{portal::ErrorCode::InputError, "Device not open"});

    std::vector<uint8_t> report(m_is_bluetooth ? 78 : 64, 0);
    DWORD bytes_read = 0;
    
    OVERLAPPED ol{};
    ol.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    
    if (!ReadFile(m_device, report.data(), (DWORD)report.size(), nullptr, &ol)) {
        if (GetLastError() != ERROR_IO_PENDING) {
            CloseHandle(ol.hEvent);
            return std::unexpected(portal::Error{portal::ErrorCode::InputError, "ReadFile failed"});
        }
        if (!GetOverlappedResult(m_device, &ol, &bytes_read, TRUE)) {
            CloseHandle(ol.hEvent);
            return std::unexpected(portal::Error{portal::ErrorCode::InputError, "GetOverlappedResult failed"});
        }
    } else {
        GetOverlappedResult(m_device, &ol, &bytes_read, FALSE);
    }
    CloseHandle(ol.hEvent);

    if (bytes_read == 0) return std::unexpected(portal::Error{portal::ErrorCode::InputError, "Zero bytes read"});

    size_t offset = 0;
    if (m_is_bluetooth) {
        if (report[0] != 0x31) return std::unexpected(portal::Error{portal::ErrorCode::InputError, "Invalid BT report ID"});
        offset = 1;
    } else {
        if (report[0] != 0x01) return std::unexpected(portal::Error{portal::ErrorCode::InputError, "Invalid USB report ID"});
    }

    DualSenseState state{};
    state.left_stick_x = report[offset + 1];
    state.left_stick_y = report[offset + 2];
    state.right_stick_x = report[offset + 3];
    state.right_stick_y = report[offset + 4];
    state.l2 = report[offset + 5];
    state.r2 = report[offset + 6];
    state.sequence = report[offset + 7];

    uint8_t dpad = report[offset + 8] & 0x0F;
    state.dpad_up = (dpad == 0 || dpad == 1 || dpad == 7);
    state.dpad_right = (dpad == 1 || dpad == 2 || dpad == 3);
    state.dpad_down = (dpad == 3 || dpad == 4 || dpad == 5);
    state.dpad_left = (dpad == 5 || dpad == 6 || dpad == 7);

    state.square = (report[offset + 8] & 0x10) != 0;
    state.cross = (report[offset + 8] & 0x20) != 0;
    state.circle = (report[offset + 8] & 0x40) != 0;
    state.triangle = (report[offset + 8] & 0x80) != 0;

    state.l1 = (report[offset + 9] & 0x01) != 0;
    state.r1 = (report[offset + 9] & 0x02) != 0;
    state.l2_btn = (report[offset + 9] & 0x04) != 0;
    state.r2_btn = (report[offset + 9] & 0x08) != 0;
    state.share = (report[offset + 9] & 0x10) != 0;
    state.options = (report[offset + 9] & 0x20) != 0;
    state.l3 = (report[offset + 9] & 0x40) != 0;
    state.r3 = (report[offset + 9] & 0x80) != 0;

    state.ps_btn = (report[offset + 10] & 0x01) != 0;
    state.touchpad_btn = (report[offset + 10] & 0x02) != 0;
    state.mute_btn = (report[offset + 10] & 0x04) != 0;

    auto read_int16 = [&](size_t idx) {
        return static_cast<int16_t>(report[idx] | (report[idx + 1] << 8));
    };
    
    state.gyro_x = read_int16(offset + 16);
    state.gyro_y = read_int16(offset + 18);
    state.gyro_z = read_int16(offset + 20);
    state.accel_x = read_int16(offset + 22);
    state.accel_y = read_int16(offset + 24);
    state.accel_z = read_int16(offset + 26);

    auto read_touch = [&](size_t idx, TouchPoint& tp) {
        tp.active = !(report[idx] & 0x80);
        tp.id = report[idx] & 0x7F;
        tp.x = report[idx + 1] | ((report[idx + 2] & 0x0F) << 8);
        tp.y = ((report[idx + 2] & 0xF0) >> 4) | (report[idx + 3] << 4);
    };

    read_touch(offset + 33, state.touch1);
    read_touch(offset + 37, state.touch2);

    state.battery_level = report[offset + 53] & 0x0F;
    state.is_charging = (report[offset + 53] & 0xF0) != 0;

    if (m_output_dirty) {
        send_output_report();
        m_output_dirty = false;
    }

    return state;
}

uint32_t DualSenseHID::calculate_crc32(const uint8_t* data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; ++i) {
        crc ^= data[i];
        for (int j = 0; j < 8; ++j) {
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : (crc >> 1);
        }
    }
    return ~crc;
}

portal::VoidResult DualSenseHID::send_output_report() {
    if (!is_open()) return std::unexpected(portal::Error{portal::ErrorCode::InputError, "Device not open"});

    std::vector<uint8_t> report(m_is_bluetooth ? 78 : 48, 0);
    
    size_t offset = 0;
    if (m_is_bluetooth) {
        report[0] = 0x31; // BT Report ID
        report[1] = 0x02; // Output report type
        offset = 2;
    } else {
        report[0] = 0x02; // USB Report ID
    }

    report[offset + 1] = 0x0F; // Valid flags: vibration, right_trigger, left_trigger, headphones? (0x0F includes 0x01, 0x02, 0x04, 0x08)
    report[offset + 2] = 0x03; // Valid flags: lightbar(0x01), player_leds(0x02)
    
    report[offset + 3] = m_motor_right;
    report[offset + 4] = m_motor_left;

    // Right trigger
    report[offset + 11] = static_cast<uint8_t>(m_right_trigger.mode);
    std::memcpy(&report[offset + 12], m_right_trigger.params, 10);
    
    // Left trigger
    report[offset + 22] = static_cast<uint8_t>(m_left_trigger.mode);
    std::memcpy(&report[offset + 23], m_left_trigger.params, 10);

    report[offset + 44] = m_lightbar_r;
    report[offset + 45] = m_lightbar_g;
    report[offset + 46] = m_lightbar_b;
    report[offset + 47] = m_player_leds;

    if (m_is_bluetooth) {
        uint32_t crc = calculate_crc32(report.data(), 74);
        report[74] = crc & 0xFF;
        report[75] = (crc >> 8) & 0xFF;
        report[76] = (crc >> 16) & 0xFF;
        report[77] = (crc >> 24) & 0xFF;
    }

    DWORD bytes_written = 0;
    OVERLAPPED ol{};
    ol.hEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    
    if (!WriteFile(m_device, report.data(), (DWORD)report.size(), nullptr, &ol)) {
        if (GetLastError() == ERROR_IO_PENDING) {
            GetOverlappedResult(m_device, &ol, &bytes_written, TRUE);
        } else {
            // Fallback directo a HidD_SetOutputReport
            HidD_SetOutputReport(m_device, report.data(), (ULONG)report.size());
        }
    } else {
        GetOverlappedResult(m_device, &ol, &bytes_written, FALSE);
    }
    CloseHandle(ol.hEvent);

    return {};
}

portal::VoidResult DualSenseHID::set_vibration(uint8_t left_low, uint8_t right_high) {
    m_motor_left = left_low;
    m_motor_right = right_high;
    m_output_dirty = true;
    return {};
}

portal::VoidResult DualSenseHID::set_lightbar(uint8_t r, uint8_t g, uint8_t b) {
    m_lightbar_r = r;
    m_lightbar_g = g;
    m_lightbar_b = b;
    m_output_dirty = true;
    return {};
}

portal::VoidResult DualSenseHID::set_trigger_effect(bool left_trigger, const TriggerEffect& effect) {
    if (left_trigger) m_left_trigger = effect;
    else m_right_trigger = effect;
    m_output_dirty = true;
    return {};
}

portal::VoidResult DualSenseHID::set_player_leds(uint8_t leds_mask) {
    m_player_leds = leds_mask;
    m_output_dirty = true;
    return {};
}

} // namespace portal::input
