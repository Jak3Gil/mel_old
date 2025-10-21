#pragma once

#include "types.h"
#include "storage.h"
#include <string>
#include <vector>
#include <memory>

namespace melvin {

/**
 * 📚 LearningSystem - Teaching & Continuous Learning
 * 
 * Handles:
 * - Teaching file ingestion (.tch format)
 * - Fact learning and reinforcement
 * - SRS (Spaced Repetition System)
 * - Edge decay & memory consolidation
 * - Emergent dimensional promotion
 */
class LearningSystem {
public:
    struct Config {
        // Learning rates
        float learning_rate = 0.01f;
        float decay_rate = 0.99f;
        float consolidation_rate = 0.01f;
        
        // SRS parameters
        int initial_interval_days = 1;
        float ease_factor_min = 1.3f;
        float ease_factor_max = 2.5f;
        
        // Dimensional promotion
        float variance_impact_threshold = 0.75f;
        float influence_weight_init = 0.1f;
        
        // Continuous learning
        bool enable_auto_learning = false;
        std::string inbox_dir = "data/inbox/";
        int watch_interval_seconds = 5;
        
        Config();
    };
    
    explicit LearningSystem(Storage* storage = nullptr, const Config& config = Config());
    ~LearningSystem();
    
    // ========================================================================
    // TEACHING
    // ========================================================================
    
    bool teach_file(const std::string& path);
    bool teach_fact(const std::string& fact, const std::string& context = "");
    bool teach_batch(const std::vector<std::string>& paths);
    
    // ========================================================================
    // REINFORCEMENT
    // ========================================================================
    
    void reinforce_path(const ReasoningPath& path, float reward);
    void decay_edges();
    void consolidate_memory();
    
    // ========================================================================
    // SRS SCHEDULING
    // ========================================================================
    
    std::vector<EdgeID> get_due_reviews();
    void mark_reviewed(const EdgeID& edge_id, bool success);
    
    // ========================================================================
    // EMERGENT DIMENSIONS
    // ========================================================================
    
    void update_variance_impacts(const std::vector<NodeID>& active_nodes, float fitness_delta);
    void promote_dimensions();
    void demote_dimensions();
    
    // ========================================================================
    // CONFIGURATION
    // ========================================================================
    
    void set_storage(Storage* storage);
    void set_config(const Config& config);
    const Config& get_config() const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        uint64_t facts_learned = 0;
        uint64_t files_processed = 0;
        uint64_t reinforcements = 0;
        uint64_t dimensions_promoted = 0;
        uint64_t reviews_due = 0;
    };
    
    Stats get_stats() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace melvin

