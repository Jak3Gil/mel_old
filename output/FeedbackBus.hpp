#pragma once

#include "melvin_types.h"
#include "OutputManager.hpp"
#include <string>
#include <memory>
#include <vector>
#include <queue>
#include <mutex>
#include <atomic>
#include <chrono>

namespace melvin {

/**
 * Feedback - Represents feedback on an output
 */
struct Feedback {
    std::string output_id;        // Identifier for the output
    float reward;                // Reward signal (-1 to 1)
    std::string feedback_text;   // Optional text feedback
    uint64_t timestamp;          // When this feedback was received
    
    Feedback() : reward(0.0f), timestamp(0) {}
    Feedback(const std::string& id, float r, const std::string& text = "") 
        : output_id(id), reward(r), feedback_text(text) {
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

/**
 * FeedbackBus - Captures and processes feedback on outputs
 * 
 * Responsibilities:
 * - Collect feedback from various sources
 * - Process feedback signals
 * - Provide feedback statistics
 * - Route feedback to learning system
 */
class FeedbackBus {
public:
    FeedbackBus();
    ~FeedbackBus() = default;

    // Core feedback methods
    void record(const OutputIntent& intent);
    void add_feedback(const Feedback& feedback);
    void add_feedback(const std::string& output_id, float reward, const std::string& text = "");
    
    // Feedback processing
    void process_feedback();
    std::vector<Feedback> get_pending_feedback() const;
    void clear_pending_feedback();
    
    // Feedback analysis
    float get_success_ratio() const;
    float get_average_reward() const;
    std::vector<Feedback> get_recent_feedback(size_t count = 10) const;
    
    // Configuration
    void set_feedback_enabled(bool enabled);
    void set_auto_process(bool enabled);
    void set_feedback_timeout(uint64_t timeout_ms);
    
    // Statistics
    size_t get_feedback_count() const;
    size_t get_positive_feedback_count() const;
    size_t get_negative_feedback_count() const;
    void reset_statistics();

private:
    // Feedback storage
    std::queue<Feedback> pending_feedback_;
    std::vector<Feedback> processed_feedback_;
    mutable std::mutex feedback_mutex_;
    
    // Configuration
    bool feedback_enabled_;
    bool auto_process_;
    uint64_t feedback_timeout_;
    
    // Statistics
    std::atomic<size_t> feedback_count_;
    std::atomic<size_t> positive_feedback_count_;
    std::atomic<size_t> negative_feedback_count_;
    std::atomic<float> total_reward_;
    
    // Internal methods
    void process_feedback_item(const Feedback& feedback);
    bool is_feedback_valid(const Feedback& feedback);
    void update_statistics(const Feedback& feedback);
    void cleanup_old_feedback();
};

} // namespace melvin
