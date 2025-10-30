/**
 * @file can_motor_controller.h
 * @brief CAN Motor Controller for Robstride O2/O3 Motors
 * 
 * Supports motors with IDs 13 and 14 via CAN bus
 */

#ifndef MELVIN_CAN_MOTOR_CONTROLLER_H
#define MELVIN_CAN_MOTOR_CONTROLLER_H

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <string>
#include <mutex>

namespace melvin {
namespace motor {

// Motor IDs
constexpr int MOTOR_13 = 13;  // 0x0D
constexpr int MOTOR_14 = 14;  // 0x0E

// Robstride CAN Commands
enum MotorCommand {
    DISABLE_MOTOR = 0xA0,
    ENABLE_MOTOR = 0xA1,
    POSITION_MODE = 0xA1,
    VELOCITY_MODE = 0xA2,
    TORQUE_MODE = 0xA3,
    READ_STATE = 0x92,
    ZERO_POSITION = 0x19
};

// Motor state structure
struct MotorState {
    float position;      // radians
    float velocity;      // rad/s
    float torque;        // Nm
    float temperature;   // °C
    uint8_t error_code;
    bool is_enabled;
    bool has_error;
    uint64_t timestamp_us;
};

/**
 * @brief CAN Motor Controller for Robstride motors
 */
class CANMotorController {
public:
    /**
     * @brief Constructor
     * @param can_interface CAN interface name (default: "can0")
     */
    explicit CANMotorController(const std::string& can_interface = "can0");
    
    /**
     * @brief Destructor - disables motors and closes socket
     */
    ~CANMotorController();
    
    // ═══════════════════════════════════════════════════════════
    // INITIALIZATION
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Initialize CAN bus connection
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Check if controller is initialized
     */
    bool is_initialized() const { return initialized_; }
    
    // ═══════════════════════════════════════════════════════════
    // MOTOR CONTROL
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Enable motor
     * @param motor_id Motor ID (13 or 14)
     * @return true if successful
     */
    bool enable_motor(int motor_id);
    
    /**
     * @brief Disable motor
     * @param motor_id Motor ID (13 or 14)
     * @return true if successful
     */
    bool disable_motor(int motor_id);
    
    /**
     * @brief Set motor position
     * @param motor_id Motor ID (13 or 14)
     * @param position Target position in radians
     * @param velocity Max velocity in rad/s (default: 2.0)
     * @param kp Position gain (default: 50.0)
     * @return true if successful
     */
    bool set_position(int motor_id, float position, float velocity = 2.0f, float kp = 50.0f);
    
    /**
     * @brief Set motor velocity
     * @param motor_id Motor ID (13 or 14)
     * @param velocity Target velocity in rad/s
     * @return true if successful
     */
    bool set_velocity(int motor_id, float velocity);
    
    /**
     * @brief Set motor torque
     * @param motor_id Motor ID (13 or 14)
     * @param torque Target torque in Nm
     * @return true if successful
     */
    bool set_torque(int motor_id, float torque);
    
    /**
     * @brief Zero motor position (set current position as 0)
     * @param motor_id Motor ID (13 or 14)
     * @return true if successful
     */
    bool zero_position(int motor_id);
    
    // ═══════════════════════════════════════════════════════════
    // MOTOR FEEDBACK
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Read motor state
     * @param motor_id Motor ID (13 or 14)
     * @return MotorState structure with current state
     */
    MotorState read_motor_state(int motor_id);
    
    /**
     * @brief Check if motor is enabled
     * @param motor_id Motor ID (13 or 14)
     * @return true if enabled
     */
    bool is_motor_enabled(int motor_id);
    
    /**
     * @brief Get last known state (from cache)
     * @param motor_id Motor ID (13 or 14)
     * @return Cached MotorState
     */
    MotorState get_cached_state(int motor_id) const;
    
    // ═══════════════════════════════════════════════════════════
    // SAFETY
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Emergency stop - immediately disable all motors
     */
    void emergency_stop();
    
    /**
     * @brief Set torque limit for motor
     * @param motor_id Motor ID (13 or 14)
     * @param max_torque Maximum torque in Nm
     */
    void set_torque_limit(int motor_id, float max_torque);
    
    /**
     * @brief Get torque limit for motor
     * @param motor_id Motor ID (13 or 14)
     * @return Current torque limit in Nm
     */
    float get_torque_limit(int motor_id) const;
    
    // ═══════════════════════════════════════════════════════════
    // UTILITIES
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Get interface name
     */
    std::string get_interface_name() const { return interface_name_; }
    
    /**
     * @brief Check if motor ID is valid (13 or 14)
     */
    static bool is_valid_motor_id(int motor_id) {
        return motor_id == MOTOR_13 || motor_id == MOTOR_14;
    }

private:
    std::string interface_name_;
    int can_socket_;
    bool initialized_;
    
    // Motor state cache
    MotorState motor_13_state_;
    MotorState motor_14_state_;
    
    // Safety limits
    float motor_13_torque_limit_;
    float motor_14_torque_limit_;
    
    // Thread safety
    mutable std::mutex socket_mutex_;
    mutable std::mutex state_mutex_;
    
    // ═══════════════════════════════════════════════════════════
    // INTERNAL FUNCTIONS
    // ═══════════════════════════════════════════════════════════
    
    /**
     * @brief Send CAN frame
     * @param motor_id Motor ID
     * @param command Command byte
     * @param data Data bytes (8 bytes)
     * @return true if successful
     */
    bool send_can_frame(int motor_id, uint8_t command, const uint8_t* data, size_t len);
    
    /**
     * @brief Receive CAN frame
     * @param frame Output frame
     * @param timeout_ms Timeout in milliseconds
     * @return true if frame received
     */
    bool receive_can_frame(struct can_frame* frame, int timeout_ms = 100);
    
    /**
     * @brief Convert float to CAN bytes (Robstride protocol)
     * @param value Float value
     * @param bytes Output bytes (2 bytes)
     * @param min Minimum value
     * @param max Maximum value
     */
    void float_to_can_bytes(float value, uint8_t* bytes, float min, float max);
    
    /**
     * @brief Convert CAN bytes to float (Robstride protocol)
     * @param bytes Input bytes (2 bytes)
     * @param min Minimum value
     * @param max Maximum value
     * @return Float value
     */
    float can_bytes_to_float(const uint8_t* bytes, float min, float max);
    
    /**
     * @brief Update cached motor state
     * @param motor_id Motor ID
     * @param state New state
     */
    void update_cached_state(int motor_id, const MotorState& state);
    
    /**
     * @brief Get current timestamp in microseconds
     */
    uint64_t get_timestamp_us() const;
};

} // namespace motor
} // namespace melvin

#endif // MELVIN_CAN_MOTOR_CONTROLLER_H

