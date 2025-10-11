#include "srs_scheduler.h"
#include <algorithm>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>

// Edge structure stub
struct Edge {
    uint64_t u, v;
    float weight = 1.0f;
    std::string rel;
    uint32_t count = 0;
    uint32_t last_access_time = 0;
};

namespace melvin {
namespace evolution {

// SRSScheduler implementation
SRSScheduler::SRSScheduler(const SRSConfig& config)
    : config_(config) {
}

void SRSScheduler::add_item(uint64_t edge_id, int64_t current_time) {
    // Don't re-add existing items
    if (items_.find(edge_id) != items_.end()) {
        return;
    }
    
    SRSItem item;
    item.edge_id = edge_id;
    item.ease = config_.ease_start;
    item.interval_days = config_.min_interval_days;
    item.due_time = current_time + static_cast<int64_t>(config_.min_interval_days * 86400.0);
    item.streak = 0;
    item.lapses = 0;
    item.total_reviews = 0;
    item.created_time = current_time;
    item.last_review_time = 0;
    item.last_grade = ReviewGrade::GOOD;
    
    items_[edge_id] = item;
}

void SRSScheduler::update_after_review(uint64_t edge_id, ReviewGrade grade, int64_t current_time) {
    auto it = items_.find(edge_id);
    if (it == items_.end()) {
        // Item doesn't exist, add it first
        add_item(edge_id, current_time);
        it = items_.find(edge_id);
    }
    
    SRSItem& item = it->second;
    
    // Update review history
    item.total_reviews++;
    item.last_review_time = current_time;
    item.last_grade = grade;
    
    // Apply SM-2 update
    apply_sm2_update(item, grade, current_time);
}

std::vector<uint64_t> SRSScheduler::get_due_items(int64_t current_time, uint32_t limit) {
    std::vector<std::pair<uint64_t, double>> due_items;  // (edge_id, urgency)
    
    for (const auto& [edge_id, item] : items_) {
        if (item.due_time <= current_time) {
            double urgency = compute_urgency(item, current_time);
            due_items.push_back({edge_id, urgency});
        }
    }
    
    // Sort by urgency (most urgent first)
    std::sort(due_items.begin(), due_items.end(),
              [](const auto& a, const auto& b) {
                  return a.second > b.second;
              });
    
    // Apply limit
    uint32_t max_items = limit > 0 ? std::min(limit, static_cast<uint32_t>(due_items.size()))
                                   : static_cast<uint32_t>(due_items.size());
    
    std::vector<uint64_t> result;
    for (uint32_t i = 0; i < max_items; i++) {
        result.push_back(due_items[i].first);
    }
    
    return result;
}

bool SRSScheduler::generate_rehearsal_lesson(
    const std::vector<RehearsalItem>& items,
    const std::string& output_path,
    int64_t current_time
) {
    if (items.empty()) {
        return false;
    }
    
    try {
        std::ofstream f(output_path);
        if (!f) {
            return false;
        }
        
        // Generate timestamp
        auto t = static_cast<time_t>(current_time);
        auto tm = *std::localtime(&t);
        
        f << "# Auto-Generated SRS Rehearsal\n";
        f << "# Generated: " << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "\n";
        f << "# Items: " << items.size() << "\n";
        f << "\n";
        
        // Generate FACT blocks for each rehearsal item
        for (size_t i = 0; i < items.size(); i++) {
            const auto& item = items[i];
            
            f << "#FACT\n";
            f << item.from_text << " " << item.rel << " " << item.to_text << "\n";
            f << "\n";
        }
        
        // Generate TEST blocks for verification
        int test_num = 0;
        for (const auto& item : items) {
            f << "#TEST Rehearsal" << test_num++ << "\n";
            f << "#QUERY What does " << item.from_text << " " << item.rel << "?\n";
            f << "#EXPECT " << item.to_text << "\n";
            f << "\n";
        }
        
        return true;
        
    } catch (...) {
        return false;
    }
}

SRSStats SRSScheduler::get_stats(int64_t current_time) const {
    SRSStats stats;
    
    stats.items_total = static_cast<uint32_t>(items_.size());
    
    double total_ease = 0.0;
    double total_interval = 0.0;
    
    for (const auto& [edge_id, item] : items_) {
        // Count due items
        if (item.due_time <= current_time) {
            stats.items_due++;
        }
        
        // Count by grade
        if (item.total_reviews > 0) {
            switch (item.last_grade) {
                case ReviewGrade::EASY: stats.reviews_easy++; break;
                case ReviewGrade::GOOD: stats.reviews_good++; break;
                case ReviewGrade::HARD: stats.reviews_hard++; break;
                case ReviewGrade::FAIL: stats.reviews_fail++; break;
            }
        }
        
        total_ease += item.ease;
        total_interval += item.interval_days;
    }
    
    // Compute averages
    if (stats.items_total > 0) {
        stats.avg_ease = total_ease / stats.items_total;
        stats.avg_interval = total_interval / stats.items_total;
    }
    
    // Compute pass rate
    uint32_t total_graded = stats.reviews_easy + stats.reviews_good + 
                           stats.reviews_hard + stats.reviews_fail;
    if (total_graded > 0) {
        uint32_t passed = stats.reviews_easy + stats.reviews_good + stats.reviews_hard;
        stats.pass_rate = static_cast<double>(passed) / total_graded;
    }
    
    return stats;
}

const SRSItem* SRSScheduler::get_item(uint64_t edge_id) const {
    auto it = items_.find(edge_id);
    if (it != items_.end()) {
        return &it->second;
    }
    return nullptr;
}

void SRSScheduler::apply_sm2_update(SRSItem& item, ReviewGrade grade, int64_t current_time) {
    // SM-2 Algorithm Implementation
    
    if (grade == ReviewGrade::FAIL) {
        // Failure: reduce ease, shrink interval, reset streak
        item.ease = std::max(config_.ease_floor, item.ease - 0.8);
        item.interval_days = std::max(config_.min_interval_days, 
                                      item.interval_days * config_.lapse_factor);
        
        if (config_.streak_reset_on_lapse) {
            item.streak = 0;
        }
        item.lapses++;
        
    } else {
        // Success: adjust ease based on difficulty
        double ease_delta = 0.0;
        
        switch (grade) {
            case ReviewGrade::EASY:
                ease_delta = config_.ease_step_good * 1.2;  // Bigger increase
                break;
            case ReviewGrade::GOOD:
                ease_delta = config_.ease_step_good;
                break;
            case ReviewGrade::HARD:
                ease_delta = config_.ease_step_hard;  // Negative
                break;
            default:
                ease_delta = 0.0;
        }
        
        item.ease = std::max(config_.ease_floor, item.ease + ease_delta);
        
        // Update interval based on streak
        if (item.streak == 0) {
            // First successful review
            item.interval_days = config_.min_interval_days;
        } else {
            // Subsequent reviews: multiply by ease
            item.interval_days = std::min(config_.max_interval_days,
                                          item.interval_days * item.ease);
            
            // Bonus acceleration for EASY
            if (grade == ReviewGrade::EASY) {
                item.interval_days *= 1.3;
                item.interval_days = std::min(config_.max_interval_days, item.interval_days);
            }
        }
        
        item.streak++;
    }
    
    // Schedule next review
    int64_t interval_seconds = static_cast<int64_t>(item.interval_days * 86400.0);
    item.due_time = current_time + interval_seconds;
}

double SRSScheduler::compute_urgency(const SRSItem& item, int64_t current_time) const {
    // Urgency = how overdue the item is
    // 0.0 = just due
    // 1.0 = very overdue (more than 1 interval past due)
    // -N = not yet due (future)
    
    if (item.due_time > current_time) {
        return -1.0;  // Not due yet
    }
    
    int64_t overdue_seconds = current_time - item.due_time;
    double overdue_days = static_cast<double>(overdue_seconds) / 86400.0;
    
    // Normalize by interval (how many intervals overdue)
    double urgency = overdue_days / std::max(item.interval_days, 0.1);
    
    // Boost by lapse count (struggling items are more urgent)
    urgency += item.lapses * 0.1;
    
    return urgency;
}

// Utility functions
std::string grade_to_string(ReviewGrade grade) {
    switch (grade) {
        case ReviewGrade::FAIL: return "FAIL";
        case ReviewGrade::HARD: return "HARD";
        case ReviewGrade::GOOD: return "GOOD";
        case ReviewGrade::EASY: return "EASY";
        default: return "UNKNOWN";
    }
}

ReviewGrade string_to_grade(const std::string& str) {
    if (str == "FAIL") return ReviewGrade::FAIL;
    if (str == "HARD") return ReviewGrade::HARD;
    if (str == "GOOD") return ReviewGrade::GOOD;
    if (str == "EASY") return ReviewGrade::EASY;
    return ReviewGrade::GOOD;  // Default
}

} // namespace evolution
} // namespace melvin

