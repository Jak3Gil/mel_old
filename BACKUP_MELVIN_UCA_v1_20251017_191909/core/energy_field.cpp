/*
 * Energy Field Implementation - Attention & Autonomous Thinking
 * 
 * Energy = Attention + Motivation
 * Flows, decays, regenerates from curiosity
 */

#include "energy_field.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace melvin {
namespace energy {

EnergyField::Config::Config() {}

EnergyField::EnergyField(const Config& config)
    : config_(config)
    , total_energy_(0.0f) {}

EnergyField::~EnergyField() = default;

void EnergyField::inject_input_energy(
    const std::vector<NodeID>& input_nodes,
    std::vector<Node>& nodes,
    float energy_amount
) {
    // Inject energy into specified nodes
    for (NodeID id : input_nodes) {
        for (auto& node : nodes) {
            if (node.id == id) {
                node.activation += energy_amount;
                total_energy_ += energy_amount;
                
                if (config_.verbose) {
                    std::cout << "[ENERGY] Injected " << energy_amount 
                              << " into node " << id << "\n";
                }
                break;
            }
        }
    }
}

void EnergyField::diffuse(
    std::vector<Node>& nodes,
    const std::vector<Edge>& edges,
    Storage* storage
) {
    // Create node ID to index mapping
    std::unordered_map<NodeID, size_t> id_to_idx;
    for (size_t i = 0; i < nodes.size(); ++i) {
        id_to_idx[nodes[i].id] = i;
    }
    
    // Allocate new energy states
    std::vector<float> new_energy(nodes.size(), 0.0f);
    
    // Diffuse energy along edges
    for (const auto& edge : edges) {
        auto from_it = id_to_idx.find(edge.from_id);
        auto to_it = id_to_idx.find(edge.to_id);
        
        if (from_it == id_to_idx.end() || to_it == id_to_idx.end()) {
            continue;
        }
        
        size_t from_idx = from_it->second;
        size_t to_idx = to_it->second;
        
        // Energy transfer = current_energy × decay × edge_weight × confidence
        float transfer = nodes[from_idx].activation * 
                        config_.decay_rate * 
                        edge.get_effective_weight() * 
                        edge.confidence;
        
        new_energy[to_idx] += transfer * config_.diffusion_rate;
    }
    
    // Apply new energy states
    total_energy_ = 0.0f;
    float max_activation = 0.0f;
    float sum_activation = 0.0f;
    
    for (size_t i = 0; i < nodes.size(); ++i) {
        // Decay current energy
        nodes[i].activation *= config_.decay_rate;
        
        // Add diffused energy
        nodes[i].activation += new_energy[i];
        
        // Maintain noise floor (baseline activity)
        if (nodes[i].activation < config_.noise_floor) {
            nodes[i].activation = config_.noise_floor;
        }
        
        total_energy_ += nodes[i].activation;
        max_activation = std::max(max_activation, nodes[i].activation);
        sum_activation += nodes[i].activation;
    }
    
    // Update stats
    stats_.total_energy = total_energy_;
    stats_.max_activation = max_activation;
    stats_.avg_activation = nodes.empty() ? 0.0f : sum_activation / nodes.size();
    stats_.diffusion_steps++;
    
    (void)storage;  // Reserved for future use
}

void EnergyField::idle_think(
    std::vector<Node>& nodes,
    const std::vector<Edge>& edges,
    Storage* storage
) {
    stats_.idle_steps++;
    
    float curiosity_total = 0.0f;
    
    // Step 1: Use prediction mismatch as curiosity
    for (auto& node : nodes) {
        // Curiosity = |predicted - actual|
        float surprise = std::abs(node.predicted_activation - node.activation);
        float curiosity_energy = surprise * config_.curiosity_gain;
        
        node.activation += curiosity_energy;
        curiosity_total += curiosity_energy;
    }
    
    stats_.curiosity_injected += curiosity_total;
    
    if (config_.verbose && curiosity_total > 0.01f) {
        std::cout << "[ENERGY] Curiosity injected: " << curiosity_total << "\n";
    }
    
    // Step 2: Add small random noise (simulate background thought)
    for (auto& node : nodes) {
        node.activation += random_noise();
    }
    
    // Step 3: Diffuse the new energy
    diffuse(nodes, edges, storage);
    
    if (config_.verbose) {
        std::cout << "[ENERGY] Idle thinking | Total energy: " << total_energy_ 
                  << " | Max: " << stats_.max_activation << "\n";
    }
}

void EnergyField::reinforce(
    std::vector<Node>& nodes,
    const std::vector<NodeID>& path,
    float reward
) {
    // Reinforce nodes in successful path
    for (NodeID id : path) {
        for (auto& node : nodes) {
            if (node.id == id) {
                node.activation += reward * 0.1f;
                total_energy_ += reward * 0.1f;
                break;
            }
        }
    }
    
    if (config_.verbose) {
        std::cout << "[ENERGY] Reinforced path with reward: " << reward << "\n";
    }
}

std::vector<float> EnergyField::compute_attention(const std::vector<Node>& nodes) const {
    std::vector<float> attention(nodes.size(), 0.0f);
    
    // Compute softmax with temperature (gamma)
    // Attention_i = E_i^γ / Σ E_j^γ
    
    float total = 0.0f;
    for (size_t i = 0; i < nodes.size(); ++i) {
        float powered = std::pow(nodes[i].activation + 1e-6f, config_.attention_gamma);
        attention[i] = powered;
        total += powered;
    }
    
    // Normalize
    if (total > 1e-6f) {
        for (auto& att : attention) {
            att /= total;
        }
    }
    
    return attention;
}

} // namespace energy
} // namespace melvin

