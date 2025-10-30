#include "field_architecture.h"
#include <iostream>
#include <thread>

namespace melvin {
namespace cognitive_field {

// ============================================================================
// CognitiveField Implementation
// ============================================================================

CognitiveField::CognitiveField(int field_id, float tick_rate_hz)
    : field_id_(field_id), tick_rate_hz_(tick_rate_hz), running_(false) {
    stats_ = {0, 0, 0, 0.0f, 0.0f};
    FieldRegistry::instance().register_field(field_id, this);
}

CognitiveField::~CognitiveField() {
    stop();
    FieldRegistry::instance().unregister_field(field_id_);
}

void CognitiveField::start() {
    if (running_.load()) return;
    running_.store(true);
    processing_thread_ = std::thread(&CognitiveField::processing_loop, this);
}

void CognitiveField::stop() {
    if (!running_.load()) return;
    running_.store(false);
    if (processing_thread_.joinable()) {
        processing_thread_.join();
    }
}

void CognitiveField::send_message(const FieldMessage& msg) {
    stats_.messages_sent++;
    FieldRegistry::instance().route_message(msg);
}

void CognitiveField::receive_message(const FieldMessage& msg) {
    std::lock_guard<std::mutex> lock(inbox_mutex_);
    inbox_.push(msg);
    stats_.messages_received++;
}

void CognitiveField::broadcast(const FieldMessage& msg) {
    stats_.messages_sent++;
    FieldRegistry::instance().broadcast_message(msg);
}

void CognitiveField::processing_loop() {
    const auto tick_duration = std::chrono::microseconds(
        static_cast<int>(1'000'000.0f / tick_rate_hz_)
    );
    
    while (running_.load()) {
        auto start_time = std::chrono::high_resolution_clock::now();
        
        // Call derived class's tick function
        tick();
        
        stats_.total_ticks++;
        
        // Calculate timing
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::microseconds>(
            end_time - start_time
        );
        
        // Update stats
        float duration_ms = duration.count() / 1000.0f;
        stats_.avg_tick_duration_ms = (stats_.avg_tick_duration_ms * 0.95f) + 
                                      (duration_ms * 0.05f);
        
        // Sleep to maintain tick rate
        if (duration < tick_duration) {
            std::this_thread::sleep_for(tick_duration - duration);
        }
    }
}

// ============================================================================
// FieldRegistry Implementation
// ============================================================================

FieldRegistry& FieldRegistry::instance() {
    static FieldRegistry registry;
    return registry;
}

void FieldRegistry::register_field(int field_id, CognitiveField* field) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    fields_[field_id] = field;
}

void FieldRegistry::unregister_field(int field_id) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    fields_.erase(field_id);
}

void FieldRegistry::route_message(const FieldMessage& msg) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    auto it = fields_.find(msg.target_field_id);
    if (it != fields_.end()) {
        it->second->receive_message(msg);
    }
}

void FieldRegistry::broadcast_message(const FieldMessage& msg) {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    for (auto& pair : fields_) {
        if (pair.first != msg.source_field_id) {
            pair.second->receive_message(msg);
        }
    }
}

std::vector<int> FieldRegistry::get_all_field_ids() const {
    std::lock_guard<std::mutex> lock(registry_mutex_);
    std::vector<int> ids;
    ids.reserve(fields_.size());
    for (const auto& pair : fields_) {
        ids.push_back(pair.first);
    }
    return ids;
}

} // namespace cognitive_field
} // namespace melvin

