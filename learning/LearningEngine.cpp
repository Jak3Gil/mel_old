#include "LearningEngine.hpp"
#include "../storage.h"
#include <algorithm>
#include <cmath>
#include <mutex>

namespace melvin {

LearningEngine::LearningEngine(std::shared_ptr<Storage> storage) 
    : storage_(storage), reinforcement_rate_(0.1f), decay_rate_(0.01f), 
      consolidation_threshold_(0.8f), experience_count_(0), reinforced_count_(0),
      total_reward_(0.0f), recent_growth_rate_(0.0f) {
}

void LearningEngine::learn(const InputConcept& input, const Thought& output) {
    // Create experience from input-output pair
    Experience experience(input, output, 0.0f);  // Neutral reward initially
    
    // Store the experience
    add_experience(experience);
    
    // Store in persistent memory
    store_experience_in_memory(experience);
    
    // Update growth rate
    update_growth_rate();
}

void LearningEngine::reinforce(const Thought& output, float reward) {
    // Find the most recent experience with matching output
    std::lock_guard<std::mutex> lock(experiences_mutex_);
    
    for (auto it = experiences_.rbegin(); it != experiences_.rend(); ++it) {
        if (it->output.text == output.text && !it->reinforced) {
            // Reinforce this experience
            it->reward = reward;
            it->reinforced = true;
            
            // Update path weights
            update_path_weights(it->output.path, it->output.relations, reward);
            
            // Update statistics
            reinforced_count_++;
            total_reward_ = total_reward_.load() + reward;
            
            break;
        }
    }
}

void LearningEngine::decay_memory() {
    // Decay edge weights
    decay_edge_weights(decay_rate_);
    
    // Prune old experiences
    prune_old_experiences();
    
    // Consolidate memory if needed
    if (experience_count_.load() % 100 == 0) {
        consolidate_memory();
    }
}

void LearningEngine::add_experience(const Experience& experience) {
    std::lock_guard<std::mutex> lock(experiences_mutex_);
    
    experiences_.push_back(experience);
    experience_count_++;
    
    // Keep only recent experiences (limit to 1000)
    if (experiences_.size() > 1000) {
        experiences_.erase(experiences_.begin(), experiences_.begin() + 100);
    }
}

void LearningEngine::reinforce_experience(size_t experience_id, float reward) {
    std::lock_guard<std::mutex> lock(experiences_mutex_);
    
    if (experience_id < experiences_.size()) {
        experiences_[experience_id].reward = reward;
        experiences_[experience_id].reinforced = true;
        
        // Update path weights
        update_path_weights(experiences_[experience_id].output.path, 
                          experiences_[experience_id].output.relations, reward);
        
        reinforced_count_++;
        total_reward_ = total_reward_.load() + reward;
    }
}

std::vector<Experience> LearningEngine::get_recent_experiences(size_t count) const {
    std::lock_guard<std::mutex> lock(experiences_mutex_);
    
    std::vector<Experience> recent;
    size_t start = (experiences_.size() > count) ? experiences_.size() - count : 0;
    
    for (size_t i = start; i < experiences_.size(); ++i) {
        recent.push_back(experiences_[i]);
    }
    
    return recent;
}

void LearningEngine::update_edge_weights(const std::vector<NodeID>& path, float reinforcement) {
    if (path.size() < 2) return;
    
    // Update weights for each edge in the path
    for (size_t i = 0; i < path.size() - 1; ++i) {
        NodeID from = path[i];
        NodeID to = path[i + 1];
        
        // Compute reinforcement strength
        float strength = compute_reinforcement_strength(reinforcement, 1.0f);
        
        // Update edge weight in storage
        // Note: This would call storage_->update_edge_weight(from, to, strength)
        // For now, we'll simulate the update
    }
}

void LearningEngine::decay_edge_weights(float decay_rate) {
    // Apply decay to all edge weights
    // Note: This would call storage_->decay_all_weights(decay_rate)
    // For now, we'll simulate the decay
}

void LearningEngine::consolidate_memory() {
    // Consolidate similar experiences
    consolidate_similar_experiences();
    
    // Merge redundant paths
    merge_redundant_paths();
    
    // Strengthen frequent patterns
    strengthen_frequent_patterns();
}

void LearningEngine::set_reinforcement_rate(float rate) {
    reinforcement_rate_ = std::max(0.0f, std::min(1.0f, rate));
}

void LearningEngine::set_decay_rate(float rate) {
    decay_rate_ = std::max(0.0f, std::min(1.0f, rate));
}

void LearningEngine::set_consolidation_threshold(float threshold) {
    consolidation_threshold_ = std::max(0.0f, std::min(1.0f, threshold));
}

size_t LearningEngine::get_experience_count() const {
    return experience_count_.load();
}

size_t LearningEngine::get_reinforced_count() const {
    return reinforced_count_.load();
}

float LearningEngine::get_average_reward() const {
    size_t count = reinforced_count_.load();
    if (count == 0) return 0.0f;
    return total_reward_.load() / count;
}

float LearningEngine::get_recent_growth_rate() const {
    return recent_growth_rate_.load();
}

void LearningEngine::reset_statistics() {
    experience_count_.store(0);
    reinforced_count_.store(0);
    total_reward_.store(0.0f);
    recent_growth_rate_.store(0.0f);
}

void LearningEngine::store_experience_in_memory(const Experience& experience) {
    // Store the experience in persistent memory
    // This would involve:
    // 1. Creating nodes for input tokens
    // 2. Creating edges for the reasoning path
    // 3. Storing the experience metadata
    
    // For now, we'll simulate the storage
}

void LearningEngine::update_path_weights(const std::vector<NodeID>& path, const std::vector<Rel>& relations, float reinforcement) {
    if (path.size() < 2) return;
    
    // Update weights for each edge in the path
    for (size_t i = 0; i < path.size() - 1; ++i) {
        NodeID from = path[i];
        NodeID to = path[i + 1];
        
        // Compute reinforcement strength
        float strength = compute_reinforcement_strength(reinforcement, 1.0f);
        
        // Update edge weight in storage
        // Note: This would call storage_->update_edge_weight(from, to, strength)
    }
}

float LearningEngine::compute_reinforcement_strength(float reward, float confidence) {
    // Combine reward and confidence to determine reinforcement strength
    float strength = reward * confidence * reinforcement_rate_;
    
    // Clamp to reasonable range
    return std::max(-1.0f, std::min(1.0f, strength));
}

void LearningEngine::prune_old_experiences() {
    std::lock_guard<std::mutex> lock(experiences_mutex_);
    
    // Remove experiences older than 24 hours
    uint64_t cutoff_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - (24 * 60 * 60 * 1000);
    
    experiences_.erase(
        std::remove_if(experiences_.begin(), experiences_.end(),
            [cutoff_time](const Experience& exp) {
                return exp.timestamp < cutoff_time && !exp.reinforced;
            }),
        experiences_.end()
    );
}

void LearningEngine::update_growth_rate() {
    // Compute recent growth rate based on experience count
    static size_t last_count = 0;
    size_t current_count = experience_count_.load();
    
    if (last_count > 0) {
        float growth = static_cast<float>(current_count - last_count) / last_count;
        recent_growth_rate_ = growth;
    }
    
    last_count = current_count;
}

void LearningEngine::consolidate_similar_experiences() {
    std::lock_guard<std::mutex> lock(experiences_mutex_);
    
    // Find similar experiences and merge them
    // This would involve:
    // 1. Computing similarity between experiences
    // 2. Merging similar experiences
    // 3. Updating weights accordingly
    
    // For now, we'll simulate consolidation
}

void LearningEngine::merge_redundant_paths() {
    // Merge redundant reasoning paths
    // This would involve:
    // 1. Finding paths that lead to similar conclusions
    // 2. Merging them into a single, stronger path
    // 3. Updating the graph accordingly
    
    // For now, we'll simulate merging
}

void LearningEngine::strengthen_frequent_patterns() {
    // Strengthen frequently used reasoning patterns
    // This would involve:
    // 1. Identifying frequently used paths
    // 2. Increasing their weights
    // 3. Making them more likely to be used
    
    // For now, we'll simulate strengthening
}

} // namespace melvin
