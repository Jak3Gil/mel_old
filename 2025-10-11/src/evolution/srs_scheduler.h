#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <string>

// Forward declaration
struct Edge;

namespace melvin {
namespace evolution {

// Review grade levels
enum class ReviewGrade {
    FAIL,   // Forgot completely
    HARD,   // Struggled to recall
    GOOD,   // Recalled correctly
    EASY    // Recalled effortlessly
};

// SRS configuration (SM-2 based)
struct SRSConfig {
    bool enabled = true;
    
    // Daily limits
    uint32_t daily_limit = 200;          // Max reviews per day
    
    // Interval bounds
    double min_interval_days = 1.0;      // Minimum review interval
    double max_interval_days = 60.0;     // Maximum review interval
    
    // Ease factor (difficulty multiplier)
    double ease_start = 2.5;             // Initial ease for new items
    double ease_floor = 1.3;             // Minimum ease
    double ease_step_good = 0.15;        // Ease increase on GOOD
    double ease_step_hard = -0.2;        // Ease decrease on HARD
    
    // Lapse handling
    double lapse_factor = 0.5;           // Interval multiplier on FAIL
    bool streak_reset_on_lapse = true;   // Reset streak on FAIL
    
    // Rehearsal generation
    bool auto_generate_lessons = true;   // Create .tch files for due items
    std::string rehearsal_dir = "data/inbox";  // Where to place rehearsals
};

// SRS item (per edge or fact)
struct SRSItem {
    uint64_t edge_id;                    // Which edge this tracks
    
    // SM-2 state
    double ease = 2.5;                   // Ease factor (difficulty)
    double interval_days = 1.0;          // Current review interval
    int64_t due_time = 0;                // Unix timestamp when next due
    
    // Performance tracking
    uint32_t streak = 0;                 // Consecutive successful reviews
    uint32_t lapses = 0;                 // Total failures
    uint32_t total_reviews = 0;          // Total review count
    
    // Metadata
    int64_t created_time = 0;            // When item was added to SRS
    int64_t last_review_time = 0;        // Last review timestamp
    ReviewGrade last_grade = ReviewGrade::GOOD;
};

// SRS rehearsal item (for generation)
struct RehearsalItem {
    uint64_t edge_id;
    std::string from_text;
    std::string rel;
    std::string to_text;
    double urgency;  // How soon it's due (0.0 = overdue, 1.0 = far future)
};

// SRS statistics
struct SRSStats {
    uint32_t items_total = 0;
    uint32_t items_due = 0;
    uint32_t items_reviewed = 0;
    uint32_t reviews_easy = 0;
    uint32_t reviews_good = 0;
    uint32_t reviews_hard = 0;
    uint32_t reviews_fail = 0;
    double pass_rate = 0.0;              // (EASY + GOOD + HARD) / total
    double avg_ease = 0.0;
    double avg_interval = 0.0;
};

// SRS Scheduler (manages spaced repetition)
class SRSScheduler {
public:
    explicit SRSScheduler(const SRSConfig& config);
    
    // Add edge to SRS system
    void add_item(uint64_t edge_id, int64_t current_time);
    
    // Update item after review
    void update_after_review(uint64_t edge_id, ReviewGrade grade, int64_t current_time);
    
    // Get items due for review
    std::vector<uint64_t> get_due_items(int64_t current_time, uint32_t limit = 0);
    
    // Generate rehearsal lesson file
    bool generate_rehearsal_lesson(
        const std::vector<RehearsalItem>& items,
        const std::string& output_path,
        int64_t current_time
    );
    
    // Get statistics
    SRSStats get_stats(int64_t current_time) const;
    
    // Get/set configuration
    const SRSConfig& config() const { return config_; }
    void set_config(const SRSConfig& config) { config_ = config; }
    
    // Get item if exists
    const SRSItem* get_item(uint64_t edge_id) const;
    
    // Get all items
    const std::unordered_map<uint64_t, SRSItem>& items() const { return items_; }
    
private:
    SRSConfig config_;
    std::unordered_map<uint64_t, SRSItem> items_;  // edge_id → SRS state
    
    // SM-2 algorithm: compute next interval and ease
    void apply_sm2_update(SRSItem& item, ReviewGrade grade, int64_t current_time);
    
    // Helper: Compute urgency score (for prioritization)
    double compute_urgency(const SRSItem& item, int64_t current_time) const;
};

// Utility: Convert grade to string
std::string grade_to_string(ReviewGrade grade);

// Utility: Convert string to grade
ReviewGrade string_to_grade(const std::string& str);

} // namespace evolution
} // namespace melvin

