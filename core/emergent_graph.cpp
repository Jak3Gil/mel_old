/**
 * @file emergent_graph.cpp
 * @brief Implementation of biologically-inspired emergent graph
 */

#include "emergent_graph.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iostream>

namespace melvin {
namespace emergent {

EmergentGraph::EmergentGraph() :
    total_energy_budget_(100.0f),
    current_energy_used_(0.0f),
    decay_rate_(0.9f),
    spread_factor_(0.3f),
    base_formation_rate_(0.01f),
    base_elimination_rate_(0.005f),
    need_threshold_(0.1f),
    cost_threshold_(0.05f),
    current_time_(0.0f)
{
}

void EmergentGraph::activate(const std::vector<int>& node_ids, float strength) {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    current_time_ += 1.0f;
    
    // Activate nodes
    for (int node_id : node_ids) {
        auto it = nodes_.find(node_id);
        if (it != nodes_.end()) {
            // Inject energy (biological input)
            it->second.activation += strength;
            it->second.usage_count++;
            
            // Track co-activation for Hebbian learning
            recent_activations_[node_id] = current_time_;
            
            // Consume energy
            consume_energy(0.1f * strength);
        }
    }
    
    // Record co-activation pairs (for connection formation)
    for (size_t i = 0; i < node_ids.size(); i++) {
        for (size_t j = i + 1; j < node_ids.size(); j++) {
            int a = node_ids[i];
            int b = node_ids[j];
            
            // Both nodes exist and fired together → potential connection
            if (nodes_.count(a) && nodes_.count(b)) {
                // Check if edge exists
                auto& node_a = nodes_[a];
                if (node_a.edges.count(b) == 0) {
                    // Create weak initial edge (biological: weak initial connection)
                    node_a.edges[b] = EmergentEdge(b, 0.01f);
                }
                
                // Record co-activation (strengthens need signal)
                auto& edge = node_a.edges[b];
                edge.coactivations++;
                edge.last_use_time = current_time_;
            }
        }
    }
}

void EmergentGraph::spread_activation(float dt) {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    // Decay all activations (biological: energy dissipates)
    for (auto& [id, node] : nodes_) {
        node.activation *= decay_rate_;
        
        // Decay edges that haven't been used (forget unused connections)
        for (auto& [to_id, edge] : node.edges) {
            float time_since_use = current_time_ - edge.last_use_time;
            if (time_since_use > 100.0f) {  // Not used in 100 cycles
                // Weaken unused edges
                edge.strength *= 0.99f;
            }
        }
    }
    
    // Spread activation through edges where need > cost
    std::unordered_map<int, float> new_activations;
    
    for (const auto& [from_id, node] : nodes_) {
        if (node.activation < 0.01f) continue;  // Skip inactive nodes
        
        for (const auto& [to_id, edge] : node.edges) {
            // BIOLOGICAL PRINCIPLE: Only spread if need > cost
            float need = compute_need(from_id, to_id);
            float cost = compute_cost(from_id, to_id);
            
            if (need > cost) {
                // Activation can flow through this edge
                float energy_transfer = node.activation * edge.strength * spread_factor_;
                energy_transfer *= (need - cost);  // Modulate by (need - cost)
                
                new_activations[to_id] += energy_transfer;
                
                // Consume energy for transmission
                consume_energy(edge.energy_consumption);
                
                // Update need signal (this connection is being used → useful)
                const_cast<EmergentEdge&>(edge).need_signal += 0.01f * (need - cost);
            }
        }
    }
    
    // Apply new activations
    for (const auto& [node_id, energy] : new_activations) {
        auto it = nodes_.find(node_id);
        if (it != nodes_.end()) {
            it->second.activation += energy;
        }
    }
    
    // Replenish energy over time (biological: metabolic recovery)
    replenish_energy(dt);
}

void EmergentGraph::update_connections() {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    // PRINCIPLE: Change = Need - Cost
    // For each edge, if need > cost → strengthen, if need < cost → weaken
    
    for (auto& [from_id, node] : nodes_) {
        for (auto it = node.edges.begin(); it != node.edges.end();) {
            auto& [to_id, edge] = *it;
            
            float need = compute_need(from_id, to_id);
            float cost = compute_cost(from_id, to_id);
            float change = need - cost;
            
            if (change > 0.0f) {
                // Need > Cost → Strengthen connection (LTP: Long-Term Potentiation)
                float delta = base_formation_rate_ * change;
                edge.strength = std::min(1.0f, edge.strength + delta);
                
                // Update information value (this connection is useful)
                edge.information_value += 0.001f * change;
                
                // Decrease cost over time if highly useful (efficiency gain)
                if (need > cost * 2.0f) {
                    edge.cost *= 0.999f;  // Slightly more efficient
                }
            } else {
                // Need < Cost → Weaken connection (LTD: Long-Term Depression)
                float delta = base_elimination_rate_ * std::abs(change);
                edge.strength = std::max(0.0f, edge.strength - delta);
                
                // Increase cost if rarely used (inefficiency penalty)
                edge.cost *= 1.001f;
            }
            
            // Remove very weak edges (synaptic elimination)
            if (edge.strength < 0.001f || edge.strength * need < cost_threshold_) {
                it = node.edges.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void EmergentGraph::prune_weak_connections(float threshold) {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    for (auto& [from_id, node] : nodes_) {
        for (auto it = node.edges.begin(); it != node.edges.end();) {
            auto& [to_id, edge] = *it;
            
            // Prune if: strength too low OR cost > need significantly
            float need = compute_need(from_id, to_id);
            float cost = compute_cost(from_id, to_id);
            
            if (edge.strength < threshold || (cost > need * 1.5f && edge.strength < 0.1f)) {
                it = node.edges.erase(it);
            } else {
                ++it;
            }
        }
    }
}

int EmergentGraph::add_node(const std::string& token, const std::vector<float>& embedding) {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    // Check if node already exists
    auto it = token_to_id_.find(token);
    if (it != token_to_id_.end()) {
        return it->second;  // Return existing
    }
    
    // Create new node (only if energy available)
    if (current_energy_used_ >= total_energy_budget_ * 0.9f) {
        // Energy constrained - can't create new node right now
        return -1;
    }
    
    int new_id = next_node_id_.fetch_add(1, std::memory_order_relaxed);
    
    EmergentNode node(new_id, token, embedding);
    nodes_[new_id] = node;
    token_to_id_[token] = new_id;
    
    // Consume energy for node creation
    consume_energy(1.0f);
    
    return new_id;
}

std::vector<int> EmergentGraph::get_active_nodes(float threshold) const {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    
    std::vector<int> active;
    for (const auto& [id, node] : nodes_) {
        if (node.activation > threshold) {
            active.push_back(id);
        }
    }
    
    // Sort by activation level (highest first)
    std::sort(active.begin(), active.end(), [this](int a, int b) {
        return nodes_.at(a).activation > nodes_.at(b).activation;
    });
    
    return active;
}

float EmergentGraph::get_activation(int node_id) const {
    std::lock_guard<std::mutex> lock(graph_mutex_);
    auto it = nodes_.find(node_id);
    return (it != nodes_.end()) ? it->second.activation : 0.0f;
}

float EmergentGraph::compute_need(int from_id, int to_id) const {
    // Need = InformationValue + Utility + Novelty
    
    auto from_it = nodes_.find(from_id);
    auto to_it = nodes_.find(to_id);
    if (from_it == nodes_.end() || to_it == nodes_.end()) return 0.0f;
    
    const auto& from_node = from_it->second;
    const auto& to_node = to_it->second;
    
    // Check if edge exists
    auto edge_it = from_node.edges.find(to_id);
    if (edge_it == from_node.edges.end()) {
        // No edge yet - return base need (for potential formation)
        return 0.1f;
    }
    
    const auto& edge = edge_it->second;
    
    // Information value: how much does this improve predictions?
    float information_value = edge.information_value;
    if (information_value < 0.001f) {
        information_value = compute_prediction_improvement(from_id, to_id);
    }
    
    // Utility: how often is this connection used? (co-activations)
    float utility = std::min(1.0f, edge.coactivations / 100.0f);
    
    // Novelty: how recently was this used?
    float time_since_use = current_time_ - edge.last_use_time;
    float novelty = std::exp(-time_since_use / 50.0f);  // Decays over time
    
    // Combined need signal
    float need = 0.4f * information_value + 0.3f * utility + 0.3f * novelty;
    
    return std::max(0.0f, std::min(1.0f, need));
}

float EmergentGraph::compute_cost(int from_id, int to_id) const {
    // Cost = EnergyConsumption + MemoryCost + ComputationCost
    
    auto from_it = nodes_.find(from_id);
    if (from_it == nodes_.end()) return 1.0f;  // High cost if node doesn't exist
    
    auto edge_it = from_it->second.edges.find(to_id);
    if (edge_it == from_it->second.edges.end()) {
        return 0.05f;  // Base cost for potential edge
    }
    
    const auto& edge = edge_it->second;
    
    // Energy consumption: proportional to connection strength and usage
    float energy_cost = edge.energy_consumption * edge.strength;
    
    // Memory cost: storing the edge
    float memory_cost = 0.001f;  // Fixed overhead per edge
    
    // Computation cost: traversing this edge
    float compute_cost = 0.001f * edge.strength;
    
    // Total cost
    float total_cost = energy_cost + memory_cost + compute_cost;
    
    return std::max(0.001f, total_cost);
}

float EmergentGraph::compute_information_value(int node_id) const {
    auto it = nodes_.find(node_id);
    if (it == nodes_.end()) return 0.5f;
    
    const auto& node = it->second;
    
    // Information value based on:
    // - How connected is this node? (hub nodes are more valuable)
    // - How often is it used?
    float connectivity = std::min(1.0f, node.edges.size() / 50.0f);
    float usage = std::min(1.0f, node.usage_count / 1000.0f);
    
    return 0.6f * connectivity + 0.4f * usage;
}

float EmergentGraph::compute_prediction_improvement(int from_id, int to_id) const {
    // Simple heuristic: if nodes co-activated frequently, 
    // this edge improves predictions
    
    auto from_it = nodes_.find(from_id);
    if (from_it == nodes_.end()) return 0.0f;
    
    auto edge_it = from_it->second.edges.find(to_id);
    if (edge_it == from_it->second.edges.end()) return 0.0f;
    
    const auto& edge = edge_it->second;
    
    // More co-activations = better prediction
    float improvement = std::min(1.0f, edge.coactivations / 50.0f);
    
    return improvement;
}

void EmergentGraph::consume_energy(float amount) {
    current_energy_used_ += amount;
    current_energy_used_ = std::min(current_energy_used_, total_energy_budget_);
}

void EmergentGraph::replenish_energy(float dt) {
    // Biological: energy recovers over time
    float recovery_rate = 1.0f;  // Energy per second
    current_energy_used_ = std::max(0.0f, current_energy_used_ - recovery_rate * dt);
}

void EmergentGraph::load(const std::string& filepath) {
    // TODO: Load existing graph from disk
    // Preserves emerged connections
}

void EmergentGraph::save(const std::string& filepath) const {
    // TODO: Save graph to disk
    // Preserves emerged structure
}

} // namespace emergent
} // namespace melvin

