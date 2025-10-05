#include "FeedbackBus.hpp"
#include <algorithm>
#include <iostream>

namespace melvin {

FeedbackBus::FeedbackBus() 
    : feedback_enabled_(true), auto_process_(true), feedback_timeout_(300000), // 5 minutes
      feedback_count_(0), positive_feedback_count_(0), negative_feedback_count_(0),
      total_reward_(0.0f) {
}

void FeedbackBus::record(const OutputIntent& intent) {
    if (!feedback_enabled_) return;
    
    // Generate a unique ID for the output
    std::string output_id = "output_" + std::to_string(intent.timestamp);
    
    // Create initial feedback record (neutral)
    Feedback feedback(output_id, 0.0f, "Output recorded");
    
    // Add to pending feedback
    {
        std::lock_guard<std::mutex> lock(feedback_mutex_);
        pending_feedback_.push(feedback);
    }
    
    // Auto-process if enabled
    if (auto_process_) {
        process_feedback();
    }
}

void FeedbackBus::add_feedback(const Feedback& feedback) {
    if (!feedback_enabled_ || !is_feedback_valid(feedback)) return;
    
    {
        std::lock_guard<std::mutex> lock(feedback_mutex_);
        pending_feedback_.push(feedback);
    }
    
    // Auto-process if enabled
    if (auto_process_) {
        process_feedback();
    }
}

void FeedbackBus::add_feedback(const std::string& output_id, float reward, const std::string& text) {
    Feedback feedback(output_id, reward, text);
    add_feedback(feedback);
}

void FeedbackBus::process_feedback() {
    std::lock_guard<std::mutex> lock(feedback_mutex_);
    
    while (!pending_feedback_.empty()) {
        Feedback feedback = pending_feedback_.front();
        pending_feedback_.pop();
        
        process_feedback_item(feedback);
    }
}

std::vector<Feedback> FeedbackBus::get_pending_feedback() const {
    std::lock_guard<std::mutex> lock(feedback_mutex_);
    
    std::vector<Feedback> pending;
    std::queue<Feedback> temp_queue = pending_feedback_;
    
    while (!temp_queue.empty()) {
        pending.push_back(temp_queue.front());
        temp_queue.pop();
    }
    
    return pending;
}

void FeedbackBus::clear_pending_feedback() {
    std::lock_guard<std::mutex> lock(feedback_mutex_);
    
    while (!pending_feedback_.empty()) {
        pending_feedback_.pop();
    }
}

float FeedbackBus::get_success_ratio() const {
    size_t total = feedback_count_.load();
    if (total == 0) return 0.0f;
    
    size_t positive = positive_feedback_count_.load();
    return static_cast<float>(positive) / total;
}

float FeedbackBus::get_average_reward() const {
    size_t count = feedback_count_.load();
    if (count == 0) return 0.0f;
    
    return total_reward_.load() / count;
}

std::vector<Feedback> FeedbackBus::get_recent_feedback(size_t count) const {
    std::lock_guard<std::mutex> lock(feedback_mutex_);
    
    std::vector<Feedback> recent;
    size_t start = (processed_feedback_.size() > count) ? processed_feedback_.size() - count : 0;
    
    for (size_t i = start; i < processed_feedback_.size(); ++i) {
        recent.push_back(processed_feedback_[i]);
    }
    
    return recent;
}

void FeedbackBus::set_feedback_enabled(bool enabled) {
    feedback_enabled_ = enabled;
}

void FeedbackBus::set_auto_process(bool enabled) {
    auto_process_ = enabled;
}

void FeedbackBus::set_feedback_timeout(uint64_t timeout_ms) {
    feedback_timeout_ = timeout_ms;
}

size_t FeedbackBus::get_feedback_count() const {
    return feedback_count_.load();
}

size_t FeedbackBus::get_positive_feedback_count() const {
    return positive_feedback_count_.load();
}

size_t FeedbackBus::get_negative_feedback_count() const {
    return negative_feedback_count_.load();
}

void FeedbackBus::reset_statistics() {
    feedback_count_.store(0);
    positive_feedback_count_.store(0);
    negative_feedback_count_.store(0);
    total_reward_.store(0.0f);
    
    std::lock_guard<std::mutex> lock(feedback_mutex_);
    processed_feedback_.clear();
}

void FeedbackBus::process_feedback_item(const Feedback& feedback) {
    // Add to processed feedback
    processed_feedback_.push_back(feedback);
    
    // Update statistics
    update_statistics(feedback);
    
    // Cleanup old feedback
    cleanup_old_feedback();
}

bool FeedbackBus::is_feedback_valid(const Feedback& feedback) {
    // Check if feedback is within valid range
    if (feedback.reward < -1.0f || feedback.reward > 1.0f) {
        return false;
    }
    
    // Check if feedback is not too old
    uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    if (current_time - feedback.timestamp > feedback_timeout_) {
        return false;
    }
    
    return true;
}

void FeedbackBus::update_statistics(const Feedback& feedback) {
    feedback_count_++;
    total_reward_ = total_reward_.load() + feedback.reward;
    
    if (feedback.reward > 0.0f) {
        positive_feedback_count_++;
    } else if (feedback.reward < 0.0f) {
        negative_feedback_count_++;
    }
}

void FeedbackBus::cleanup_old_feedback() {
    uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Remove feedback older than timeout
    processed_feedback_.erase(
        std::remove_if(processed_feedback_.begin(), processed_feedback_.end(),
            [this, current_time](const Feedback& feedback) {
                return current_time - feedback.timestamp > feedback_timeout_;
            }),
        processed_feedback_.end()
    );
}

} // namespace melvin
