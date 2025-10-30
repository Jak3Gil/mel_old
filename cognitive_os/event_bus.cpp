/**
 * @file event_bus.cpp
 * @brief Implementation of event bus
 */

#include "event_bus.h"
#include <chrono>
#include <algorithm>

namespace melvin {
namespace cognitive_os {

EventBus::EventBus(size_t buffer_size) : buffer_capacity_(buffer_size) {}

void EventBus::subscribe(const std::string& topic, 
                         std::function<void(const Event&)> callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_[topic].push_back(callback);
}

std::vector<Event> EventBus::poll(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = buffers_.find(topic);
    if (it == buffers_.end() || it->second.empty()) {
        return {};
    }
    
    std::vector<Event> events = it->second;
    it->second.clear();
    
    return events;
}

Event EventBus::get_latest(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = buffers_.find(topic);
    if (it == buffers_.end() || it->second.empty()) {
        return Event{};
    }
    
    return it->second.back();
}

void EventBus::clear(const std::string& topic) {
    std::lock_guard<std::mutex> lock(mutex_);
    buffers_[topic].clear();
}

double EventBus::get_timestamp() const {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration<double>(duration).count();
}

} // namespace cognitive_os
} // namespace melvin

