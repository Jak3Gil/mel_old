#ifndef FIELD_ARCHITECTURE_H
#define FIELD_ARCHITECTURE_H

#include <vector>
#include <unordered_map>
#include <queue>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <functional>

namespace melvin {
namespace cognitive_field {

/**
 * Message types for inter-field communication
 * Fields communicate asynchronously via message passing
 */
enum class MessageType {
    SENSORY_INPUT,      // Raw sensor data
    ACTIVATION_SPIKE,   // Node activation change
    ATTENTION_SHIFT,    // Attention focus change
    PREDICTION_ERROR,   // Mismatch between prediction and reality
    CONTEXT_UPDATE,     // Global context vector update
    GOAL_REQUEST,       // Request to achieve a goal
    MOTOR_COMMAND,      // Output motor action
    REFLECTION_QUERY,   // Meta-cognitive question
    CONSOLIDATION_HINT  // Suggestion for memory consolidation
};

/**
 * Message structure for field communication
 */
struct FieldMessage {
    MessageType type;
    int source_field_id;
    int target_field_id;
    std::chrono::high_resolution_clock::time_point timestamp;
    std::vector<float> data;  // Flexible data payload
    std::unordered_map<std::string, float> metadata;
    
    FieldMessage(MessageType t, int src, int tgt) 
        : type(t), source_field_id(src), target_field_id(tgt),
          timestamp(std::chrono::high_resolution_clock::now()) {}
};

/**
 * Base class for all cognitive fields
 * Each field runs asynchronously at its own tick rate
 */
class CognitiveField {
public:
    CognitiveField(int field_id, float tick_rate_hz);
    virtual ~CognitiveField();
    
    // Start/stop the field's processing loop
    void start();
    void stop();
    bool is_running() const { return running_.load(); }
    
    // Message queue interface
    void send_message(const FieldMessage& msg);
    void receive_message(const FieldMessage& msg);
    
    // Core processing loop (override in derived classes)
    virtual void tick() = 0;
    
    // Field statistics
    struct Stats {
        size_t total_ticks;
        size_t messages_sent;
        size_t messages_received;
        float avg_tick_duration_ms;
        float current_energy;
    };
    Stats get_stats() const { return stats_; }
    
protected:
    int field_id_;
    float tick_rate_hz_;
    std::atomic<bool> running_;
    
    // Message queues
    std::queue<FieldMessage> inbox_;
    std::mutex inbox_mutex_;
    
    // Thread for async processing
    std::thread processing_thread_;
    void processing_loop();
    
    // Statistics
    Stats stats_;
    
    // Helper to broadcast to all fields
    void broadcast(const FieldMessage& msg);
};

/**
 * Global Field Registry
 * Manages all active fields and routes messages
 */
class FieldRegistry {
public:
    static FieldRegistry& instance();
    
    void register_field(int field_id, CognitiveField* field);
    void unregister_field(int field_id);
    
    void route_message(const FieldMessage& msg);
    void broadcast_message(const FieldMessage& msg);
    
    std::vector<int> get_all_field_ids() const;
    
private:
    FieldRegistry() = default;
    std::unordered_map<int, CognitiveField*> fields_;
    std::mutex registry_mutex_;
};

/**
 * Field IDs (constants for each cognitive field)
 */
namespace FieldID {
    constexpr int PERCEPTION = 1;
    constexpr int ATTENTION = 2;
    constexpr int REASONING = 3;
    constexpr int MEMORY = 4;
    constexpr int OUTPUT = 5;
    constexpr int REFLECTION = 6;
    constexpr int EVOLUTION = 7;
}

} // namespace cognitive_field
} // namespace melvin

#endif // FIELD_ARCHITECTURE_H

