#pragma once

#include "../teaching/teaching_format.h"
#include <string>
#include <unordered_map>
#include <vector>

namespace melvin {
namespace learning {

// SRS card states (SuperMemo SM-2 inspired)
enum class CardState {
    NEW,       // Never reviewed
    LEARNING,  // Failed at least once, being learned
    MATURE,    // Passed multiple times, long intervals
    LAPSED     // Was mature, recently failed
};

// Spaced repetition card for a test
struct SRSCard {
    std::string test_id;       // "filename::test_name"
    std::string question;
    std::vector<std::string> expects;
    
    CardState state = CardState::NEW;
    int reviews = 0;
    int successes = 0;
    int failures = 0;
    
    double ease_factor = 2.5;  // SM-2 ease factor
    int interval_days = 0;
    
    std::string next_review_date;  // ISO 8601 (YYYY-MM-DD)
    std::string last_review_date;
    
    // Metadata
    std::string source_file;
    int source_line;
};

// SRS scheduler
class SRSScheduler {
public:
    SRSScheduler() = default;
    
    // Load SRS state from JSON
    bool load(const std::string& filepath = "melvin_srs.json");
    
    // Save SRS state to JSON
    bool save(const std::string& filepath = "melvin_srs.json");
    
    // Add cards from a teaching document
    void add_cards_from_doc(
        const melvin::teaching::TchDoc& doc,
        const std::string& lesson_id
    );
    
    // Get cards due for review today
    std::vector<SRSCard> get_due_cards(const std::string& today_iso);
    
    // Record a review result
    void record_review(
        const std::string& test_id,
        bool passed,
        int quality  // 0-5 (SM-2 quality rating)
    );
    
    // Get statistics
    struct Stats {
        int total_cards;
        int new_cards;
        int learning_cards;
        int mature_cards;
        int lapsed_cards;
        int due_today;
    };
    Stats get_stats(const std::string& today_iso);
    
private:
    std::unordered_map<std::string, SRSCard> cards_;
    
    // Compute next review date using SM-2 algorithm
    static std::string compute_next_review(
        const SRSCard& card,
        bool passed,
        int quality
    );
    
    // Get current date (ISO 8601)
    static std::string current_date_iso();
    
    // Add days to a date
    static std::string add_days(const std::string& date_iso, int days);
};

} // namespace learning
} // namespace melvin

