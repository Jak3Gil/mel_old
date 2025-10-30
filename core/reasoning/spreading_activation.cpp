#include "spreading_activation.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <numeric>

namespace melvin {
namespace reasoning {

// GAP 5: Meta-Learning Adaptation
void MetaLearner::adapt(float success, float surprise) {
    // Exponential moving average
    recent_success_rate = 0.9f * recent_success_rate + 0.1f * success;
    recent_surprise_rate = 0.9f * recent_surprise_rate + 0.1f * surprise;
    
    recent_errors.push_back(1.0f - success);
    if (recent_errors.size() > 100) {
        recent_errors.pop_front();
    }
    
    cycle_count++;
    
    // Adapt learning strategy based on performance
    if (recent_success_rate < 0.3f || recent_surprise_rate > 0.5f) {
        // Struggling or encountering novelty → EXPLORE
        phase = EXPLORATION;
        learning_rate = 0.3f;
        exploration_rate = 0.5f;
    } else if (recent_success_rate < 0.7f) {
        // Improving but not mastered → REFINE
        phase = REFINEMENT;
        learning_rate = 0.1f;
        exploration_rate = 0.2f;
    } else {
        // Mastered → EXPLOIT and consolidate
        phase = EXPLOITATION;
        learning_rate = 0.03f;
        exploration_rate = 0.05f;
        consolidation_threshold = 0.9f;
    }
}

ActivationField::ActivationField(float decay_rate, float spread_rate, float min_activation)
    : decay_rate_(decay_rate)
    , spread_rate_(spread_rate)
    , min_activation_(min_activation)
    , tick_rate_(15.0f)
    , running_(false)
    , graph_ptr_(nullptr)
{
}

ActivationField::~ActivationField() {
    stop_background_loop();
}

void ActivationField::activate(int node_id, float strength) {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    activations_[node_id] = std::max(activations_[node_id], strength);
}

float ActivationField::get_activation(int node_id) const {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    auto it = activations_.find(node_id);
    return (it != activations_.end()) ? it->second : 0.0f;
}

std::unordered_map<int, float> ActivationField::get_active_nodes(float threshold) const {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    std::unordered_map<int, float> result;
    for (const auto& pair : activations_) {
        if (pair.second >= threshold) {
            result[pair.first] = pair.second;
        }
    }
    return result;
}

void ActivationField::start_background_loop() {
    if (!running_.load()) {
        running_.store(true);
        background_thread_ = std::thread(&ActivationField::background_loop, this);
        std::cout << "🔄 Activation field started (target: " << tick_rate_ << " Hz)" << std::endl;
    }
}

void ActivationField::stop_background_loop() {
    if (running_.load()) {
        running_.store(false);
        if (background_thread_.joinable()) {
            background_thread_.join();
        }
        std::cout << "⏹️  Activation field stopped" << std::endl;
    }
}

void ActivationField::background_loop() {
    using namespace std::chrono;
    
    while (running_.load()) {
        auto start = high_resolution_clock::now();
        
        // Tick if we have a graph
        if (graph_ptr_) {
            tick(*graph_ptr_);
        }
        
        auto end = high_resolution_clock::now();
        auto elapsed = duration_cast<milliseconds>(end - start).count();
        
        // Sleep to maintain tick rate
        int sleep_ms = static_cast<int>((1000.0f / tick_rate_) - elapsed);
        if (sleep_ms > 0) {
            std::this_thread::sleep_for(milliseconds(sleep_ms));
        }
    }
}

void ActivationField::tick(const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph) {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    
    // Store graph reference for background loop
    graph_ptr_ = &graph;
    
    // ========================================================================
    // ADAPTIVE INTELLIGENCE: Normalize activations to prevent runaway growth
    // ========================================================================
    float total_energy = 0.0f;
    for (const auto& pair : activations_) {
        total_energy += pair.second;
    }
    
    // Normalize if energy gets too high
    if (total_energy > 10.0f) {
        for (auto& pair : activations_) {
            pair.second /= (total_energy / 10.0f);
        }
        total_energy = 10.0f;
    }
    
    // ========================================================================
    // ADAPTIVE INTELLIGENCE: Compute entropy for stability control
    // ========================================================================
    float entropy = 0.0f;
    if (total_energy > 1e-6f) {
        for (const auto& pair : activations_) {
            float p = pair.second / total_energy;
            if (p > 1e-6f) {
                entropy -= p * std::log(p);
            }
        }
    }
    
    // ========================================================================
    // ADAPTIVE INTELLIGENCE: Adjust parameters based on entropy
    // Keep system in "critical zone" between order and chaos
    // ========================================================================
    // Target entropy range: [0.4, 1.0]
    // Too low (< 0.4) = too ordered, need more exploration
    // Too high (> 1.0) = too chaotic, need more stability
    
    if (entropy > 1.0f) {
        // Too chaotic → stabilize
        decay_rate_ *= 0.99f;
        spread_rate_ *= 0.95f;
    } else if (entropy < 0.4f && total_energy > 0.1f) {
        // Too ordered → explore more
        decay_rate_ = std::min(0.95f, decay_rate_ * 1.01f);
        spread_rate_ = std::min(0.5f, spread_rate_ * 1.05f);
    }
    
    // Clamp parameters to safe ranges
    decay_rate_ = std::max(0.85f, std::min(0.98f, decay_rate_));
    spread_rate_ = std::max(0.1f, std::min(0.5f, spread_rate_));
    
    // ========================================================================
    // Original decay and spreading logic
    // ========================================================================
    
    // Decay all activations
    std::vector<int> to_remove;
    for (auto& pair : activations_) {
        pair.second *= decay_rate_;
        if (pair.second < min_activation_) {
            to_remove.push_back(pair.first);
        }
    }
    
    // Remove below threshold
    for (int node_id : to_remove) {
        activations_.erase(node_id);
    }
    
    // Spread activation
    std::unordered_map<int, float> new_activations;
    for (const auto& pair : activations_) {
        int node_id = pair.first;
        float activation = pair.second;
        
        if (activation > min_activation_) {
            // Find node in graph
            auto it = graph.find(node_id);
            if (it != graph.end()) {
                // Spread to neighbors
                for (const auto& edge : it->second) {
                    int neighbor_id = edge.first;
                    float edge_weight = edge.second;
                    float spread_amount = activation * edge_weight * spread_rate_;
                    new_activations[neighbor_id] += spread_amount;
                }
            }
        }
    }
    
    // Apply new activations (max with existing)
    for (const auto& pair : new_activations) {
        activations_[pair.first] = std::max(activations_[pair.first], pair.second);
    }
    
    current_time_ += (1000.0f / tick_rate_);
}

// ==============================================================================
// GAP 1: PREDICTION-ERROR FEEDBACK
// ==============================================================================

int ActivationField::predict_next_node(int current_node) {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    
    if (!graph_ptr_) return -1;
    
    auto it = graph_ptr_->find(current_node);
    if (it == graph_ptr_->end()) return -1;
    
    // Find highest-weighted edge
    int best_node = -1;
    float best_weight = 0.0f;
    
    for (const auto& edge : it->second) {
        if (edge.second > best_weight) {
            best_weight = edge.second;
            best_node = edge.first;
        }
    }
    
    last_predicted_node_ = best_node;
    return best_node;
}

void ActivationField::apply_prediction_error(const PredictionError& error) {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    
    // Store error for meta-learning
    recent_predictions_.push_back(error);
    if (recent_predictions_.size() > 100) {
        recent_predictions_.pop_front();
    }
    
    // Compute success rate for meta-learning
    float success = (error.predicted_node == error.actual_node) ? 1.0f : 0.0f;
    float surprise = error.error_magnitude;
    
    // Update meta-learner
    meta_learner_.adapt(success, surprise);
    
    // Update energy traces based on error
    // High error = strengthen recent activations more
    float error_weight = error.error_magnitude * meta_learner_.learning_rate;
    
    for (auto& pair : energy_map_) {
        if (pair.second.energy_trace > 0.1f) {
            // Nodes in recent trace get updated by error
            pair.second.energy_trace *= (1.0f + error_weight);
        }
    }
}

// ==============================================================================
// GAP 2: ENHANCED ENERGY SYSTEM
// ==============================================================================

void ActivationField::inject_energy(int node_id, float strength, float salience, float novelty) {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    
    auto& dynamics = energy_map_[node_id];
    
    // E_input = α * salience * novelty * base_energy
    float energy_injection = base_input_energy_ * strength * salience * novelty;
    
    dynamics.energy_input += energy_injection;
    dynamics.salience = salience;
    dynamics.novelty_bonus = novelty;
    dynamics.activation_count++;
    dynamics.last_activation_time = current_time_;
    
    // Also update legacy activation for compatibility
    activations_[node_id] = std::max(activations_[node_id], energy_injection / 10.0f);
}

float ActivationField::get_energy(int node_id) const {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    auto it = energy_map_.find(node_id);
    return (it != energy_map_.end()) ? it->second.energy : 0.0f;
}

EnergyDynamics& ActivationField::get_energy_dynamics(int node_id) {
    // Note: Called with lock held by caller
    return energy_map_[node_id];
}

void ActivationField::update_energy_dynamics(const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph) {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    
    // Phase 1: Compute spreading activation (energy from neighbors)
    for (auto& pair : energy_map_) {
        pair.second.reset();  // Clear buffers
        
        int node_id = pair.first;
        auto it = graph.find(node_id);
        if (it != graph.end()) {
            // Accumulate energy from incoming edges
            for (const auto& edge : it->second) {
                int src_id = edge.first;
                float edge_weight = edge.second;
                
                auto src_it = energy_map_.find(src_id);
                if (src_it != energy_map_.end()) {
                    // E_spreading = edge_weight * source_energy * conductance
                    float conductance = 1.0f / (1.0f + std::exp(-edge_weight));  // Sigmoid
                    float spreading = edge_weight * src_it->second.energy * conductance * spread_rate_;
                    pair.second.energy_buffer += spreading;
                }
            }
        }
    }
    
    // Phase 2: Update energy levels
    std::vector<int> to_remove;
    for (auto& pair : energy_map_) {
        auto& dynamics = pair.second;
        
        // E(t+1) = E(t) + E_input + E_spreading - E_decay - E_output
        float decay = (1.0f - decay_rate_) * dynamics.energy;
        
        dynamics.energy = dynamics.energy + 
                         dynamics.energy_input + 
                         dynamics.energy_buffer - 
                         decay;
        
        // Clamp energy
        dynamics.energy = std::max(0.0f, std::min(100.0f, dynamics.energy));
        
        // Update eligibility trace (for credit assignment)
        dynamics.energy_trace = dynamics.energy_trace * 0.95f + dynamics.energy;
        
        // Remove if energy too low
        if (dynamics.energy < min_activation_ && dynamics.activation_count < 2) {
            to_remove.push_back(pair.first);
        }
    }
    
    // Cleanup
    for (int node_id : to_remove) {
        energy_map_.erase(node_id);
    }
}

// ==============================================================================
// GAP 3: ATTENTION MECHANISM
// ==============================================================================

void ActivationField::set_goal_node(int goal_id, const std::vector<float>& goal_embedding) {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    current_goal_node_ = goal_id;
    current_goal_embedding_ = goal_embedding;
}

float ActivationField::compute_goal_similarity(int node_id, const std::vector<float>& goal_emb,
                                               const std::unordered_map<int, std::vector<float>>& embeddings) {
    auto it = embeddings.find(node_id);
    if (it == embeddings.end() || goal_emb.empty()) return 1.0f;
    
    const auto& node_emb = it->second;
    if (node_emb.size() != goal_emb.size()) return 1.0f;
    
    // Cosine similarity
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < goal_emb.size(); i++) {
        dot += goal_emb[i] * node_emb[i];
        norm_a += goal_emb[i] * goal_emb[i];
        norm_b += node_emb[i] * node_emb[i];
    }
    
    if (norm_a > 0 && norm_b > 0) {
        return dot / (std::sqrt(norm_a) * std::sqrt(norm_b));
    }
    return 0.0f;
}

void ActivationField::compute_attention_weights(
    std::unordered_map<int, std::vector<TemporalEdge>>& edges,
    const std::unordered_map<int, std::vector<float>>& embeddings) {
    
    std::lock_guard<std::mutex> lock(activation_mutex_);
    
    if (current_goal_embedding_.empty()) return;
    
    // Compute attention for all edges
    for (auto& pair : edges) {
        for (auto& edge : pair.second) {
            // Base attention from edge weight
            float base = edge.weight;
            
            // Goal relevance (semantic similarity)
            float goal_rel = compute_goal_similarity(edge.dst, current_goal_embedding_, embeddings);
            
            // Surprise value (difference between predicted and actual)
            float surprise = 0.0f;
            auto it = energy_map_.find(edge.dst);
            if (it != energy_map_.end()) {
                surprise = std::abs(it->second.energy - base);
            }
            
            // Compute combined attention
            edge.attention.compute(base, goal_rel, surprise);
            
            // Normalize attention (softmax-like)
            edge.attention.combined = std::exp(edge.attention.combined);
        }
    }
    
    // Normalize across all edges
    float sum = 0.0f;
    for (const auto& pair : edges) {
        for (const auto& edge : pair.second) {
            sum += edge.attention.combined;
        }
    }
    
    if (sum > 0) {
        for (auto& pair : edges) {
            for (auto& edge : pair.second) {
                edge.attention.combined /= sum;
            }
        }
    }
}

// ==============================================================================
// GAP 4: TEMPORAL PREDICTIONS
// ==============================================================================

std::vector<int> ActivationField::predict_future_states(
    int current_node, 
    const std::unordered_map<int, std::vector<TemporalEdge>>& temporal_edges,
    float lookahead_ms) {
    
    std::lock_guard<std::mutex> lock(activation_mutex_);
    
    std::vector<int> predictions;
    
    auto it = temporal_edges.find(current_node);
    if (it == temporal_edges.end()) return predictions;
    
    // Follow temporal/causal edges forward in time
    for (const auto& edge : it->second) {
        if (edge.delay_ms <= lookahead_ms && 
            (edge.type == TEMPORAL || edge.type == CAUSAL || edge.type == PREDICTIVE)) {
            predictions.push_back(edge.dst);
        }
    }
    
    return predictions;
}

// ==============================================================================
// GAP 5: META-LEARNING
// ==============================================================================

void ActivationField::update_meta_parameters() {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    
    // Compute recent performance
    float success_rate = 0.0f;
    float surprise_rate = 0.0f;
    
    if (!recent_predictions_.empty()) {
        int successes = 0;
        float total_surprise = 0.0f;
        
        for (const auto& pred : recent_predictions_) {
            if (pred.predicted_node == pred.actual_node) successes++;
            total_surprise += pred.error_magnitude;
        }
        
        success_rate = static_cast<float>(successes) / recent_predictions_.size();
        surprise_rate = total_surprise / recent_predictions_.size();
    }
    
    // Update meta-learner
    meta_learner_.adapt(success_rate, surprise_rate);
    
    // Apply adapted parameters to system
    decay_rate_ = 1.0f - (0.1f * meta_learner_.learning_rate / 0.1f);  // Higher LR = faster decay
    spread_rate_ = 0.3f * (1.0f + meta_learner_.exploration_rate);      // Higher explore = more spread
}

// ==============================================================================
// GAP 6: CONSOLIDATION SUPPORT
// ==============================================================================

std::vector<int> ActivationField::get_high_energy_nodes(float threshold) const {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    
    std::vector<int> high_energy_nodes;
    
    for (const auto& pair : energy_map_) {
        if (pair.second.energy >= threshold) {
            high_energy_nodes.push_back(pair.first);
        }
    }
    
    return high_energy_nodes;
}

void ActivationField::decay_eligibility_traces(float decay_factor) {
    std::lock_guard<std::mutex> lock(activation_mutex_);
    
    for (auto& pair : energy_map_) {
        pair.second.energy_trace *= decay_factor;
    }
}

} // namespace reasoning
} // namespace melvin
