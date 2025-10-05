#pragma once

#include <string>
#include <memory>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <atomic>

namespace melvin {

/**
 * InputManager - Handles sensory and text inputs from various sources
 * 
 * Supports multiple input modalities:
 * - Terminal/stdin text input
 * - File input
 * - Network/API input
 * - Sensor data (future extension)
 */
class InputManager {
public:
    InputManager();
    ~InputManager();

    // Core input methods
    std::string get_next_input();  // blocks until input arrives
    bool has_input_available() const;
    void add_input(const std::string& input);
    
    // Input source management
    void enable_terminal_input(bool enable = true);
    void enable_file_input(const std::string& filename);
    void enable_network_input(int port = 8080);
    
    // Input preprocessing
    void set_input_preprocessor(std::function<std::string(const std::string&)> preprocessor);
    
    // Statistics
    size_t get_input_count() const;
    void reset_statistics();

private:
    std::queue<std::string> input_queue_;
    mutable std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    
    std::atomic<bool> running_;
    std::atomic<size_t> input_count_;
    
    // Input sources
    std::atomic<bool> terminal_enabled_;
    std::string file_input_path_;
    std::atomic<bool> network_enabled_;
    int network_port_;
    
    // Preprocessing
    std::function<std::string(const std::string&)> preprocessor_;
    
    // Background threads
    std::thread terminal_thread_;
    std::thread file_thread_;
    std::thread network_thread_;
    
    void terminal_input_loop();
    void file_input_loop();
    void network_input_loop();
    
    void process_and_queue_input(const std::string& raw_input);
};

} // namespace melvin
