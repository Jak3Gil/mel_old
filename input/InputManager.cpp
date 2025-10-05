#include "InputManager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace melvin {

InputManager::InputManager() 
    : running_(true), input_count_(0), terminal_enabled_(true), 
      network_enabled_(false), network_port_(8080) {
    
    // Start terminal input thread
    terminal_thread_ = std::thread(&InputManager::terminal_input_loop, this);
}

InputManager::~InputManager() {
    running_ = false;
    
    // Notify all waiting threads
    queue_cv_.notify_all();
    
    // Join threads
    if (terminal_thread_.joinable()) {
        terminal_thread_.join();
    }
    if (file_thread_.joinable()) {
        file_thread_.join();
    }
    if (network_thread_.joinable()) {
        network_thread_.join();
    }
}

std::string InputManager::get_next_input() {
    std::unique_lock<std::mutex> lock(queue_mutex_);
    
    // Wait for input to be available
    queue_cv_.wait(lock, [this] { return !input_queue_.empty() || !running_; });
    
    if (!running_ && input_queue_.empty()) {
        return "";  // Shutdown signal
    }
    
    std::string input = input_queue_.front();
    input_queue_.pop();
    
    return input;
}

bool InputManager::has_input_available() const {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    return !input_queue_.empty();
}

void InputManager::add_input(const std::string& input) {
    process_and_queue_input(input);
}

void InputManager::enable_terminal_input(bool enable) {
    terminal_enabled_ = enable;
}

void InputManager::enable_file_input(const std::string& filename) {
    file_input_path_ = filename;
    
    // Stop existing file thread if running
    if (file_thread_.joinable()) {
        // Signal to stop and join
        file_thread_.join();
    }
    
    // Start new file input thread
    file_thread_ = std::thread(&InputManager::file_input_loop, this);
}

void InputManager::enable_network_input(int port) {
    network_port_ = port;
    network_enabled_ = true;
    
    // Stop existing network thread if running
    if (network_thread_.joinable()) {
        network_thread_.join();
    }
    
    // Start new network input thread
    network_thread_ = std::thread(&InputManager::network_input_loop, this);
}

void InputManager::set_input_preprocessor(std::function<std::string(const std::string&)> preprocessor) {
    preprocessor_ = preprocessor;
}

size_t InputManager::get_input_count() const {
    return input_count_.load();
}

void InputManager::reset_statistics() {
    input_count_.store(0);
}

void InputManager::terminal_input_loop() {
    std::string line;
    while (running_ && terminal_enabled_) {
        if (std::getline(std::cin, line)) {
            process_and_queue_input(line);
        } else {
            // EOF or error - check if we should continue
            if (!running_) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void InputManager::file_input_loop() {
    if (file_input_path_.empty()) return;
    
    std::ifstream file(file_input_path_);
    if (!file.is_open()) {
        std::cerr << "Warning: Could not open input file: " << file_input_path_ << std::endl;
        return;
    }
    
    std::string line;
    while (running_ && std::getline(file, line)) {
        process_and_queue_input(line);
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); // Rate limiting
    }
    
    file.close();
}

void InputManager::network_input_loop() {
    // Simplified network input - in a real implementation, this would use
    // a proper HTTP server or socket library
    while (running_ && network_enabled_) {
        // Placeholder for network input handling
        // This would typically involve:
        // 1. Setting up a socket server
        // 2. Accepting connections
        // 3. Reading input from clients
        // 4. Processing and queuing the input
        
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

void InputManager::process_and_queue_input(const std::string& raw_input) {
    std::string processed_input = raw_input;
    
    // Apply preprocessing if available
    if (preprocessor_) {
        processed_input = preprocessor_(raw_input);
    }
    
    // Basic preprocessing: trim whitespace
    processed_input.erase(0, processed_input.find_first_not_of(" \t\r\n"));
    processed_input.erase(processed_input.find_last_not_of(" \t\r\n") + 1);
    
    // Skip empty inputs
    if (processed_input.empty()) return;
    
    // Queue the processed input
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);
        input_queue_.push(processed_input);
        input_count_++;
    }
    
    // Notify waiting threads
    queue_cv_.notify_one();
}

} // namespace melvin
