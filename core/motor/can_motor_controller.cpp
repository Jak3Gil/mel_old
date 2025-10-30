/**
 * @file can_motor_controller.cpp
 * @brief Implementation of CAN Motor Controller for Robstride motors
 */

#include "can_motor_controller.h"
#include <chrono>
#include <cmath>
#include <fcntl.h>
#include <poll.h>

namespace melvin {
namespace motor {

// Robstride protocol constants
constexpr float POS_MIN = -12.5f;   // radians
constexpr float POS_MAX = 12.5f;
constexpr float VEL_MIN = -65.0f;   // rad/s
constexpr float VEL_MAX = 65.0f;
constexpr float TORQUE_MIN = -18.0f; // Nm
constexpr float TORQUE_MAX = 18.0f;
constexpr float KP_MIN = 0.0f;
constexpr float KP_MAX = 500.0f;

CANMotorController::CANMotorController(const std::string& can_interface)
    : interface_name_(can_interface)
    , can_socket_(-1)
    , initialized_(false)
    , motor_13_torque_limit_(10.0f)  // Default 10 Nm limit
    , motor_14_torque_limit_(10.0f)
{
    // Initialize motor states
    motor_13_state_ = {0};
    motor_14_state_ = {0};
}

CANMotorController::~CANMotorController() {
    if (initialized_) {
        std::cout << "Disabling motors before shutdown...\n";
        disable_motor(MOTOR_13);
        disable_motor(MOTOR_14);
        
        if (can_socket_ >= 0) {
            close(can_socket_);
        }
    }
}

bool CANMotorController::initialize() {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    
    if (initialized_) {
        return true;
    }
    
    // Create CAN socket
    can_socket_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (can_socket_ < 0) {
        std::cerr << "Failed to create CAN socket: " << strerror(errno) << "\n";
        return false;
    }
    
    // Get interface index
    struct ifreq ifr;
    strcpy(ifr.ifr_name, interface_name_.c_str());
    if (ioctl(can_socket_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "Failed to get interface index for " << interface_name_ 
                  << ": " << strerror(errno) << "\n";
        close(can_socket_);
        can_socket_ = -1;
        return false;
    }
    
    // Bind socket to CAN interface
    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    
    if (bind(can_socket_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Failed to bind CAN socket: " << strerror(errno) << "\n";
        close(can_socket_);
        can_socket_ = -1;
        return false;
    }
    
    // Set non-blocking mode
    int flags = fcntl(can_socket_, F_GETFL, 0);
    fcntl(can_socket_, F_SETFL, flags | O_NONBLOCK);
    
    initialized_ = true;
    std::cout << "✅ CAN Motor Controller initialized on " << interface_name_ << "\n";
    std::cout << "   Motors: 13 (0x0D), 14 (0x0E)\n";
    
    return true;
}

bool CANMotorController::enable_motor(int motor_id) {
    if (!initialized_ || !is_valid_motor_id(motor_id)) {
        return false;
    }
    
    uint8_t data[8] = {0};
    
    if (!send_can_frame(motor_id, ENABLE_MOTOR, data, 8)) {
        std::cerr << "Failed to enable motor " << motor_id << "\n";
        return false;
    }
    
    std::cout << "✅ Motor " << motor_id << " enabled\n";
    return true;
}

bool CANMotorController::disable_motor(int motor_id) {
    if (!initialized_ || !is_valid_motor_id(motor_id)) {
        return false;
    }
    
    uint8_t data[8] = {0};
    
    if (!send_can_frame(motor_id, DISABLE_MOTOR, data, 8)) {
        std::cerr << "Failed to disable motor " << motor_id << "\n";
        return false;
    }
    
    std::cout << "Motor " << motor_id << " disabled\n";
    return true;
}

bool CANMotorController::set_position(int motor_id, float position, float velocity, float kp) {
    if (!initialized_ || !is_valid_motor_id(motor_id)) {
        return false;
    }
    
    // Clamp values to safe ranges
    position = std::max(POS_MIN, std::min(POS_MAX, position));
    velocity = std::max(0.0f, std::min(VEL_MAX, velocity));
    kp = std::max(KP_MIN, std::min(KP_MAX, kp));
    
    // Encode data
    uint8_t data[8] = {0};
    float_to_can_bytes(position, &data[0], POS_MIN, POS_MAX);
    float_to_can_bytes(velocity, &data[2], 0.0f, VEL_MAX);
    float_to_can_bytes(kp, &data[4], KP_MIN, KP_MAX);
    data[6] = 0;  // Kd low byte
    data[7] = 0;  // Kd high byte
    
    return send_can_frame(motor_id, POSITION_MODE, data, 8);
}

bool CANMotorController::set_velocity(int motor_id, float velocity) {
    if (!initialized_ || !is_valid_motor_id(motor_id)) {
        return false;
    }
    
    // Clamp velocity
    velocity = std::max(VEL_MIN, std::min(VEL_MAX, velocity));
    
    uint8_t data[8] = {0};
    float_to_can_bytes(velocity, &data[0], VEL_MIN, VEL_MAX);
    
    return send_can_frame(motor_id, VELOCITY_MODE, data, 8);
}

bool CANMotorController::set_torque(int motor_id, float torque) {
    if (!initialized_ || !is_valid_motor_id(motor_id)) {
        return false;
    }
    
    // Apply torque limit
    float limit = (motor_id == MOTOR_13) ? motor_13_torque_limit_ : motor_14_torque_limit_;
    torque = std::max(-limit, std::min(limit, torque));
    
    // Clamp to motor max
    torque = std::max(TORQUE_MIN, std::min(TORQUE_MAX, torque));
    
    uint8_t data[8] = {0};
    float_to_can_bytes(torque, &data[0], TORQUE_MIN, TORQUE_MAX);
    
    return send_can_frame(motor_id, TORQUE_MODE, data, 8);
}

bool CANMotorController::zero_position(int motor_id) {
    if (!initialized_ || !is_valid_motor_id(motor_id)) {
        return false;
    }
    
    uint8_t data[8] = {0};
    
    if (!send_can_frame(motor_id, ZERO_POSITION, data, 8)) {
        std::cerr << "Failed to zero motor " << motor_id << "\n";
        return false;
    }
    
    std::cout << "✅ Motor " << motor_id << " position zeroed\n";
    return true;
}

MotorState CANMotorController::read_motor_state(int motor_id) {
    if (!initialized_ || !is_valid_motor_id(motor_id)) {
        return get_cached_state(motor_id);
    }
    
    uint8_t data[8] = {0};
    
    if (!send_can_frame(motor_id, READ_STATE, data, 8)) {
        return get_cached_state(motor_id);
    }
    
    // Wait for response
    struct can_frame frame;
    if (receive_can_frame(&frame, 50)) {
        if (frame.can_id == motor_id && frame.can_dlc == 8) {
            MotorState state;
            state.position = can_bytes_to_float(&frame.data[0], POS_MIN, POS_MAX);
            state.velocity = can_bytes_to_float(&frame.data[2], VEL_MIN, VEL_MAX);
            state.torque = can_bytes_to_float(&frame.data[4], TORQUE_MIN, TORQUE_MAX);
            state.temperature = frame.data[6];
            state.error_code = frame.data[7];
            state.is_enabled = (state.error_code & 0x01) == 0;
            state.has_error = (state.error_code & 0xFE) != 0;
            state.timestamp_us = get_timestamp_us();
            
            update_cached_state(motor_id, state);
            return state;
        }
    }
    
    return get_cached_state(motor_id);
}

bool CANMotorController::is_motor_enabled(int motor_id) {
    MotorState state = read_motor_state(motor_id);
    return state.is_enabled;
}

MotorState CANMotorController::get_cached_state(int motor_id) const {
    std::lock_guard<std::mutex> lock(state_mutex_);
    return (motor_id == MOTOR_13) ? motor_13_state_ : motor_14_state_;
}

void CANMotorController::emergency_stop() {
    std::cout << "🛑 EMERGENCY STOP - Disabling all motors!\n";
    disable_motor(MOTOR_13);
    disable_motor(MOTOR_14);
}

void CANMotorController::set_torque_limit(int motor_id, float max_torque) {
    if (motor_id == MOTOR_13) {
        motor_13_torque_limit_ = max_torque;
        std::cout << "Motor 13 torque limit: " << max_torque << " Nm\n";
    } else if (motor_id == MOTOR_14) {
        motor_14_torque_limit_ = max_torque;
        std::cout << "Motor 14 torque limit: " << max_torque << " Nm\n";
    }
}

float CANMotorController::get_torque_limit(int motor_id) const {
    return (motor_id == MOTOR_13) ? motor_13_torque_limit_ : motor_14_torque_limit_;
}

// ═══════════════════════════════════════════════════════════
// INTERNAL FUNCTIONS
// ═══════════════════════════════════════════════════════════

bool CANMotorController::send_can_frame(int motor_id, uint8_t command, const uint8_t* data, size_t len) {
    std::lock_guard<std::mutex> lock(socket_mutex_);
    
    struct can_frame frame;
    frame.can_id = motor_id;
    frame.can_dlc = len;
    frame.data[0] = command;
    
    for (size_t i = 0; i < len - 1 && i < 7; i++) {
        frame.data[i + 1] = data[i];
    }
    
    ssize_t bytes_sent = write(can_socket_, &frame, sizeof(frame));
    if (bytes_sent != sizeof(frame)) {
        std::cerr << "Failed to send CAN frame: " << strerror(errno) << "\n";
        return false;
    }
    
    return true;
}

bool CANMotorController::receive_can_frame(struct can_frame* frame, int timeout_ms) {
    struct pollfd fds;
    fds.fd = can_socket_;
    fds.events = POLLIN;
    
    int ret = poll(&fds, 1, timeout_ms);
    if (ret <= 0) {
        return false;  // Timeout or error
    }
    
    ssize_t bytes_read = read(can_socket_, frame, sizeof(*frame));
    return (bytes_read == sizeof(*frame));
}

void CANMotorController::float_to_can_bytes(float value, uint8_t* bytes, float min, float max) {
    // Normalize to 0-65535 range
    float normalized = (value - min) / (max - min);
    uint16_t encoded = static_cast<uint16_t>(normalized * 65535.0f);
    
    bytes[0] = (encoded >> 8) & 0xFF;  // High byte
    bytes[1] = encoded & 0xFF;          // Low byte
}

float CANMotorController::can_bytes_to_float(const uint8_t* bytes, float min, float max) {
    uint16_t encoded = (bytes[0] << 8) | bytes[1];
    float normalized = static_cast<float>(encoded) / 65535.0f;
    return min + normalized * (max - min);
}

void CANMotorController::update_cached_state(int motor_id, const MotorState& state) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    
    if (motor_id == MOTOR_13) {
        motor_13_state_ = state;
    } else if (motor_id == MOTOR_14) {
        motor_14_state_ = state;
    }
}

uint64_t CANMotorController::get_timestamp_us() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count();
}

} // namespace motor
} // namespace melvin

