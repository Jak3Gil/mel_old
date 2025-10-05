#pragma once

#include "melvin_types.h"
#include "../input/PerceptionEngine.hpp"
#include "../reasoning/ReasoningEngine.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <atomic>

namespace melvin {

/**
 * Experience - Represents a learning experience
 */
struct Experience {
    InputConcept input;              // What was perceived
    Thought output;             // What was reasoned
    float reward;              // Feedback reward (-1 to 1)
    uint64_t timestamp;        // When this experience occurred
    bool reinforced;           // Whether this experience was reinforced
    
    Experience() : reward(0.0f), timestamp(0), reinforced(false) {}
    Experience(const InputConcept& i, const Thought& o, float r) 
        : input(i), output(o), reward(r), reinforced(false) {
        timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }
};

/**
 * LearningEngine - Handles short- and long-term learning
 * 
 * Responsibilities:
 * - Store input→output experiences
 * - Apply reinforcement learning
 * - Decay unused memory
 * - Update edge weights in persistent memory
 */
class LearningEngine {
public:
    LearningEngine(std::shared_ptr<Storage> storage);
    ~LearningEngine() = default;

    // Core learning methods
    void learn(const InputConcept& input, const Thought& output);
    void reinforce(const Thought& output, float reward);
    void decay_memory();
    
    // Experience management
    void add_experience(const Experience& experience);
    void reinforce_experience(size_t experience_id, float reward);
    std::vector<Experience> get_recent_experiences(size_t count = 10) const;
    
    // Memory operations
    void update_edge_weights(const std::vector<NodeID>& path, float reinforcement);
    void decay_edge_weights(float decay_rate = 0.01f);
    void consolidate_memory();
    
    // Learning parameters
    void set_reinforcement_rate(float rate);
    void set_decay_rate(float rate);
    void set_consolidation_threshold(float threshold);
    
    // Statistics
    size_t get_experience_count() const;
    size_t get_reinforced_count() const;
    float get_average_reward() const;
    float get_recent_growth_rate() const;
    void reset_statistics();

private:
    std::shared_ptr<Storage> storage_;
    
    // Experience storage
    std::vector<Experience> experiences_;
    mutable std::mutex experiences_mutex_;
    
    // Learning parameters
    float reinforcement_rate_;
    float decay_rate_;
    float consolidation_threshold_;
    
    // Statistics
    std::atomic<size_t> experience_count_;
    std::atomic<size_t> reinforced_count_;
    std::atomic<float> total_reward_;
    std::atomic<float> recent_growth_rate_;
    
    // Internal methods
    void store_experience_in_memory(const Experience& experience);
    void update_path_weights(const std::vector<NodeID>& path, const std::vector<Rel>& relations, float reinforcement);
    float compute_reinforcement_strength(float reward, float confidence);
    void prune_old_experiences();
    void update_growth_rate();
    
    // Memory consolidation
    void consolidate_similar_experiences();
    void merge_redundant_paths();
    void strengthen_frequent_patterns();
};

} // namespace melvin
