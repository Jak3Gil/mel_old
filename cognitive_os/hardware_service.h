/**
 * @file hardware_service.h
 * @brief Hardware I/O services for Jetson (cameras, audio, motors)
 */

#ifndef MELVIN_HARDWARE_SERVICE_H
#define MELVIN_HARDWARE_SERVICE_H

#include "event_bus.h"
#include "service_base.h"
#include <opencv2/opencv.hpp>
#include <thread>
#include <atomic>
#include <string>
#include <vector>

namespace melvin {
namespace cognitive_os {

/**
 * @brief USB Camera capture service
 */
class VisionService : public ServiceBase {
public:
    VisionService(EventBus* bus, const std::vector<std::string>& camera_devices);
    ~VisionService();
    
    void start() override;
    void stop() override;
    
private:
    void capture_loop();
    
    EventBus* bus_;
    std::vector<cv::VideoCapture> cameras_;
    std::vector<std::string> devices_;
    std::thread capture_thread_;
    std::atomic<bool> running_{false};
    double last_frame_time_{0.0};
    static constexpr double FRAME_RATE = 10.0;  // 10 FPS for vision processing
};

/**
 * @brief USB Microphone capture service
 */
class AudioInputService : public ServiceBase {
public:
    AudioInputService(EventBus* bus, const std::string& alsa_device = "default");
    ~AudioInputService();
    
    void start() override;
    void stop() override;
    
private:
    void capture_loop();
    std::vector<float> process_audio_chunk(const std::vector<int16_t>& samples);
    
    EventBus* bus_;
    std::string device_;
    std::thread capture_thread_;
    std::atomic<bool> running_{false};
    static constexpr int SAMPLE_RATE = 16000;
    static constexpr int CHUNK_SIZE = 3200;  // 200ms chunks
};

/**
 * @brief USB Speaker output service
 */
class AudioOutputService : public ServiceBase {
public:
    AudioOutputService(EventBus* bus, const std::string& alsa_device = "default");
    ~AudioOutputService();
    
    void start() override;
    void stop() override;
    
    void speak(const std::string& text);
    
private:
    void output_loop();
    std::vector<int16_t> synthesize_speech(const std::string& text);
    
    EventBus* bus_;
    std::string device_;
    std::thread output_thread_;
    std::atomic<bool> running_{false};
    std::vector<std::string> speech_queue_;
    std::mutex queue_mutex_;
};

/**
 * @brief CAN bus motor control service
 */
class MotorService : public ServiceBase {
public:
    MotorService(EventBus* bus, const std::string& can_interface = "can0");
    ~MotorService();
    
    void start() override;
    void stop() override;
    
private:
    void control_loop();
    void send_motor_command(int motor_id, float position, float velocity);
    void read_motor_feedback();
    
    EventBus* bus_;
    std::string can_interface_;
    int can_socket_{-1};
    std::thread control_thread_;
    std::atomic<bool> running_{false};
};

} // namespace cognitive_os
} // namespace melvin

#endif // MELVIN_HARDWARE_SERVICE_H

