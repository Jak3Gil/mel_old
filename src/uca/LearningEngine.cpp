#include "src/uca/LearningEngine.hpp"
#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace uca {

// Mathematical constants for learning
static constexpr float HEBBIAN_ETA = 0.01f;        // C1: Learning rate
static constexpr float DECAY_LAMBDA = 1e-4f;       // C1: Decay rate per reinforcement tick
static constexpr float EMA_ALPHA = 0.05f;          // C3: EMA smoothing factor
static constexpr float TD_GAMMA = 0.9f;            // C2: Temporal discount factor

// Mock edge weight storage (in real implementation, this would be persistent)
static std::unordered_map<std::string, float> mock_edge_weights;
static std::unordered_map<std::string, float> mock_edge_counts;
static std::unordered_map<std::string, float> mock_value_function;  // C2: TD learning

bool LearningEngine::configure(const DynamicGenome&) { 
    return true; 
}

// C1: Hebbian learning with decay
void apply_hebbian_update(const std::string& edge_id, float reward) {
    auto it = mock_edge_weights.find(edge_id);
    float current_weight = (it != mock_edge_weights.end()) ? it->second : 0.5f;
    
    // Hebbian update: w ← (1-λ)w + η·r·x_i·x_j
    // Simplified: x_i·x_j = 1.0 for co-activated nodes
    float hebbian_increment = HEBBIAN_ETA * reward * 1.0f;  // x_i·x_j = 1.0
    float new_weight = (1.0f - DECAY_LAMBDA) * current_weight + hebbian_increment;
    
    // Clamp weights to reasonable range
    new_weight = std::clamp(new_weight, 0.0f, 2.0f);
    mock_edge_weights[edge_id] = new_weight;
}

// C2: TD(0) learning for value function
void apply_td_update(const std::string& edge_id, float reward, float next_value = 0.0f) {
    auto it = mock_value_function.find(edge_id);
    float current_value = (it != mock_value_function.end()) ? it->second : 0.0f;
    
    // TD error: δ = r + γ·V(s') - V(s)
    float td_error = reward + TD_GAMMA * next_value - current_value;
    
    // Update value function: V(s) ← V(s) + α·δ
    float learning_rate = 0.1f;  // TD learning rate
    float new_value = current_value + learning_rate * td_error;
    
    mock_value_function[edge_id] = new_value;
}

// C3: EMA for count updates
void apply_ema_count_update(const std::string& edge_id) {
    auto it = mock_edge_counts.find(edge_id);
    float current_count = (it != mock_edge_counts.end()) ? it->second : 0.0f;
    
    // EMA update: count ← (1-α)·count + α·1
    float new_count = (1.0f - EMA_ALPHA) * current_count + EMA_ALPHA * 1.0f;
    
    mock_edge_counts[edge_id] = new_count;
}

// Generate edge ID from path nodes
std::string generate_edge_id(uint64_t from_node, uint64_t to_node) {
    return std::to_string(from_node) + "->" + std::to_string(to_node);
}

LearningDelta LearningEngine::reinforce(const std::vector<ReasoningPath>& paths,
                                        const Feedback& fb) {
    LearningDelta delta{};
    
    if (paths.empty()) return delta;
    
    // Process each reasoning path
    for (const auto& path : paths) {
        if (path.node_ids.size() < 2) continue;
        
        // Apply learning to each edge in the path
        for (size_t i = 0; i < path.node_ids.size() - 1; ++i) {
            uint64_t from_node = path.node_ids[i];
            uint64_t to_node = path.node_ids[i + 1];
            std::string edge_id = generate_edge_id(from_node, to_node);
            
            // C1: Hebbian learning with decay
            apply_hebbian_update(edge_id, fb.reward);
            delta.edges_updated++;
            
            // C2: TD(0) learning (simplified - use reward directly)
            float next_value = (i < path.node_ids.size() - 2) ? 0.5f : 0.0f;  // Mock next value
            apply_td_update(edge_id, fb.reward, next_value);
            
            // C3: EMA count update for trust factor
            apply_ema_count_update(edge_id);
            
            delta.paths_reinforced++;
        }
    }
    
    return delta;
}

}
