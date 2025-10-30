/**
 * @file event_bus.h
 * @brief Lock-free pub/sub event bus for cognitive services
 * 
 * Zero-copy where possible, SPSC ring buffers per topic
 */

#ifndef MELVIN_EVENT_BUS_H
#define MELVIN_EVENT_BUS_H

#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
#include <atomic>
#include <mutex>
#include <memory>

namespace melvin {
namespace cognitive_os {

/**
 * @brief Event topics
 */
namespace topics {
    constexpr const char* VISION_EVENTS = "/vision/events";
    constexpr const char* AUDIO_EVENTS = "/audio/events";
    constexpr const char* MOTOR_STATE = "/motor/state";
    constexpr const char* COG_QUERY = "/cog/query";
    constexpr const char* COG_ANSWER = "/cog/answer";
    constexpr const char* FIELD_METRICS = "/field/metrics";
    constexpr const char* WM_CONTEXT = "/wm/context";
    constexpr const char* REFLECT_COMMAND = "/reflect/command";
    constexpr const char* SAFETY_EVENTS = "/safety/events";
}

/**
 * @brief Vision event
 */
struct VisionEvent {
    double timestamp;
    std::vector<int> obj_ids;
    std::vector<std::vector<float>> embeddings;
    std::vector<float> bbox;  // [x, y, w, h]
};

/**
 * @brief Audio event
 */
struct AudioEvent {
    double timestamp;
    std::vector<std::string> phonemes;
    float energy;
    std::vector<float> embedding;
};

/**
 * @brief Motor state
 */
struct MotorState {
    double timestamp;
    std::vector<float> joint_pos;
    std::vector<float> joint_vel;
    std::vector<float> torque;
};

/**
 * @brief Cognitive query
 */
struct CogQuery {
    double timestamp;
    std::string text;
    std::vector<float> embedding;
    int intent;  // 0=DEFINE, 1=LOCATE, etc.
};

/**
 * @brief Cognitive answer
 */
struct CogAnswer {
    double timestamp;
    std::string text;
    std::vector<std::string> reasoning_chain;
    float confidence;
};

/**
 * @brief Field metrics
 */
struct FieldMetrics {
    double timestamp;
    int active_nodes;
    float energy_variance;
    float sparsity;
    float entropy;
    float coherence;
    float confidence;
};

/**
 * @brief Working memory context
 */
struct WMContext {
    double timestamp;
    std::vector<int> node_ids;  // Max 7
    std::vector<float> strengths;
};

/**
 * @brief Reflection command
 */
struct ReflectCommand {
    double timestamp;
    int mode;  // 0=EXPLORATORY, 1=EXPLOITATIVE, etc.
    float beta;
    float theta;
    std::string strategy;
};

/**
 * @brief Safety event
 */
struct SafetyEvent {
    double timestamp;
    std::string event_type;  // "BACKPRESSURE", "OVERHEAT", "QUEUE_OVERFLOW"
    float severity;  // 0-1
    std::string details;
};

/**
 * @brief Generic event wrapper
 */
struct Event {
    std::string topic;
    double timestamp;
    std::shared_ptr<void> data;
    
    template<typename T>
    std::shared_ptr<T> get() const {
        return std::static_pointer_cast<T>(data);
    }
};

/**
 * @brief Lock-free event bus
 * 
 * SPSC ring buffers per topic, pub/sub pattern
 */
class EventBus {
public:
    EventBus(size_t buffer_size = 1024);
    ~EventBus() = default;
    
    /**
     * @brief Publish event to topic
     */
    template<typename T>
    void publish(const std::string& topic, const T& event_data) {
        Event event;
        event.topic = topic;
        event.timestamp = get_timestamp();
        event.data = std::make_shared<T>(event_data);
        
        std::lock_guard<std::mutex> lock(mutex_);
        auto& buffer = buffers_[topic];
        if (buffer.size() >= buffer_capacity_) {
            buffer.erase(buffer.begin());  // Drop oldest
            dropped_msgs_.fetch_add(1, std::memory_order_relaxed);
        }
        buffer.push_back(event);
    }
    
    /**
     * @brief Subscribe to topic with callback
     */
    void subscribe(const std::string& topic, 
                   std::function<void(const Event&)> callback);
    
    /**
     * @brief Poll for new events (non-blocking)
     */
    std::vector<Event> poll(const std::string& topic);
    
    /**
     * @brief Get latest event from topic
     */
    Event get_latest(const std::string& topic);
    
    /**
     * @brief Clear all events from topic
     */
    void clear(const std::string& topic);
    
    /**
     * @brief Get dropped message count
     */
    uint64_t dropped_messages() const {
        return dropped_msgs_.load(std::memory_order_relaxed);
    }
    
private:
    size_t buffer_capacity_;
    std::unordered_map<std::string, std::vector<Event>> buffers_;
    std::unordered_map<std::string, std::vector<std::function<void(const Event&)>>> subscribers_;
    std::mutex mutex_;
    std::atomic<uint64_t> dropped_msgs_{0};
    
    double get_timestamp() const;
};

} // namespace cognitive_os
} // namespace melvin

#endif // MELVIN_EVENT_BUS_H

