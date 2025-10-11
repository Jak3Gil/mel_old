/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN CONNECTION CHEMISTRY SYSTEM                                       ║
 * ║  Living molecular connections - no fixed types, pure continuous chemistry ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 * 
 * Philosophy:
 * -----------
 * A connection is not a label (exact, temporal, leap, etc.).
 * It's a LIVING CHEMICAL BRIDGE whose behavior emerges from continuous parameters.
 * 
 * Like protein complexes in cells, connections self-organize into functional clusters
 * (grammar, memory, empathy, logic) without ever being explicitly programmed.
 * 
 * Mathematical Foundation:
 * -----------------------
 * Connection weight evolution:
 * 
 *   dw_ab/dt = η_ab(A_a * A_b - κ*w_ab) + β*(ΔC_a * ΔC_b) - γ*E_ab
 * 
 * Where:
 *   w_ab: connection strength
 *   A_a, A_b: node activations  
 *   C_a, C_b: context potentials
 *   E_ab: accumulated prediction error/tension
 *   η: plasticity rate (how fast it adapts)
 *   κ: stability factor (resistance to change)
 *   β: diffusion coupling strength
 *   γ: error damping rate
 * 
 * Activation Flow:
 * ----------------
 * When diffusion occurs:
 * 
 *   flow = conductivity * affinity * (C[a] - C[b]) * (1 + directional_bias)
 * 
 * This creates:
 *   - Sequential paths: high directional_bias
 *   - Analogies: high affinity, low bias
 *   - Hypotheses: high plasticity, low stability
 *   - Facts: low plasticity, high stability
 * 
 * NO rule tables. ALL emergent.
 */

#pragma once

#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>

namespace melvin {

// Global chemistry constants (meta-learned)
struct ChemistryConstants {
    float plasticity_base = 0.002f;      // Base learning rate
    float stability_decay = 0.95f;       // How fast stability erodes if unused
    float energy_damping = 0.1f;         // How fast prediction errors dissipate
    float affinity_growth = 0.01f;       // How fast affinity increases with success
    float conductivity_floor = 0.01f;    // Minimum conductivity before pruning
    float directional_threshold = 0.1f;  // Minimum bias to be considered directional
    float fusion_similarity = 0.95f;     // Similarity threshold for merging connections
    float fission_volatility = 0.8f;     // Plasticity threshold for splitting
    float age_decay_rate = 0.0001f;      // How age affects conductivity
    float reinforcement_gain = 0.05f;    // Strength increase per successful activation
    
    ChemistryConstants() = default;
};

// Connection as a living molecular entity
struct Connection {
    // Identity
    uint64_t a = 0;  // Source node
    uint64_t b = 0;  // Destination node
    
    // Core chemistry parameters
    float conductivity = 0.5f;          // How easily activation flows (0-1)
    float directional_bias = 0.0f;      // Asymmetry: positive = A→B, negative = B→A, 0 = symmetric
    float plasticity_rate = 0.01f;      // How quickly connection adapts to experience
    float stability = 0.5f;             // Resistance to change (increases with reinforcement)
    float affinity = 0.5f;              // Compatibility with node activation states
    float energy_potential = 0.0f;      // Stored tension from prediction errors
    
    // Metabolism tracking
    uint32_t age = 0;                   // Connection lifespan in ticks
    float last_activity = 0.0f;         // Last activation strength
    float last_update_time = 0.0f;      // Time since last parameter update
    uint32_t activation_count = 0;      // Total number of activations
    float cumulative_flow = 0.0f;       // Total activation that has flowed through
    
    // Quality metrics
    float prediction_accuracy = 0.5f;   // How often flow correlates with positive outcomes
    float coherence_contribution = 0.0f; // How much this connection improves global coherence
    
    Connection() = default;
    Connection(uint64_t source, uint64_t dest) 
        : a(source), b(dest), conductivity(0.5f), directional_bias(0.0f),
          plasticity_rate(0.01f), stability(0.5f), affinity(0.5f),
          energy_potential(0.0f), age(0), last_activity(0.0f),
          last_update_time(0.0f), activation_count(0), cumulative_flow(0.0f),
          prediction_accuracy(0.5f), coherence_contribution(0.0f) {}
    
    // Compute activation flow based on node potentials and chemistry
    float compute_flow(float C_a, float C_b) const {
        // Potential gradient
        float gradient = C_a - C_b;
        
        // Base flow through conductivity and affinity
        float base_flow = conductivity * affinity * gradient;
        
        // Apply directional bias
        float directional_factor = 1.0f + directional_bias;
        if (gradient < 0.0f) {
            // Reverse flow - bias works against it
            directional_factor = 1.0f - directional_bias;
        }
        
        // Energy potential adds inertia/overshoot
        float energy_contribution = 0.1f * energy_potential;
        
        return base_flow * directional_factor + energy_contribution;
    }
    
    // Update chemistry based on activation and feedback
    void update_chemistry(float A_a, float A_b, float delta_C_a, float delta_C_b,
                         float fitness_signal, const ChemistryConstants& constants) {
        age++;
        
        // Hebbian plasticity: nodes that fire together wire together
        float hebbian = A_a * A_b;
        
        // Prediction error: did the potential change align with expectations?
        float prediction_error = std::abs(delta_C_a * delta_C_b - last_activity);
        
        // Update conductivity (connection strength)
        float conductivity_delta = plasticity_rate * (hebbian - constants.stability_decay * conductivity);
        conductivity += conductivity_delta;
        conductivity = std::max(constants.conductivity_floor, std::min(1.0f, conductivity));
        
        // Update plasticity rate based on novelty and error
        if (prediction_error > 0.5f) {
            // High error = need more plasticity
            plasticity_rate = std::min(0.1f, plasticity_rate * 1.05f);
        } else {
            // Low error = can reduce plasticity
            plasticity_rate = std::max(0.001f, plasticity_rate * 0.99f);
        }
        
        // Update stability based on success
        if (fitness_signal > 0.0f) {
            // Successful activation = increase stability
            stability = std::min(1.0f, stability + constants.reinforcement_gain);
        } else {
            // Unsuccessful = decrease stability slightly
            stability *= constants.stability_decay;
            stability = std::max(0.1f, stability);
        }
        
        // Update affinity based on activation correlation
        float correlation = (A_a > 0.1f && A_b > 0.1f) ? 1.0f : 0.0f;
        affinity += constants.affinity_growth * (correlation - affinity);
        affinity = std::max(0.0f, std::min(1.0f, affinity));
        
        // Update energy potential (prediction error accumulator)
        energy_potential += prediction_error - constants.energy_damping * energy_potential;
        energy_potential = std::max(-1.0f, std::min(1.0f, energy_potential));
        
        // Update directional bias based on asymmetric activation
        float asymmetry = A_a - A_b;
        if (std::abs(asymmetry) > 0.2f) {
            directional_bias += 0.01f * asymmetry;
            directional_bias = std::max(-1.0f, std::min(1.0f, directional_bias));
        }
        
        // Track activity
        last_activity = hebbian;
        activation_count++;
        cumulative_flow += std::abs(hebbian);
        
        // Age decay - very old, unused connections weaken
        if (age > 1000 && activation_count < age / 100) {
            conductivity *= (1.0f - constants.age_decay_rate);
        }
    }
    
    // Check if connection should be pruned
    bool should_prune(const ChemistryConstants& constants) const {
        // Prune if conductivity too low and old
        if (conductivity < constants.conductivity_floor && age > 100) {
            return true;
        }
        
        // Prune if no activity for long time
        if (age > 500 && activation_count == 0) {
            return true;
        }
        
        return false;
    }
    
    // Check if connection should undergo fission (split)
    bool should_split(const ChemistryConstants& constants) const {
        // High plasticity + high energy = volatile, exploratory
        return (plasticity_rate > constants.fission_volatility && 
                std::abs(energy_potential) > 0.7f &&
                activation_count > 50);
    }
    
    // Compute similarity to another connection for fusion
    float similarity(const Connection& other) const {
        if (a != other.a || b != other.b) return 0.0f; // Must connect same nodes
        
        // Compute parameter distance
        float dist = 0.0f;
        dist += std::abs(conductivity - other.conductivity);
        dist += std::abs(directional_bias - other.directional_bias);
        dist += std::abs(plasticity_rate - other.plasticity_rate);
        dist += std::abs(stability - other.stability);
        dist += std::abs(affinity - other.affinity);
        
        return 1.0f - (dist / 5.0f); // Normalize by number of parameters
    }
    
    // Merge with another connection (weighted average)
    void merge_with(const Connection& other, float weight = 0.5f) {
        conductivity = (1.0f - weight) * conductivity + weight * other.conductivity;
        directional_bias = (1.0f - weight) * directional_bias + weight * other.directional_bias;
        plasticity_rate = (1.0f - weight) * plasticity_rate + weight * other.plasticity_rate;
        stability = (1.0f - weight) * stability + weight * other.stability;
        affinity = (1.0f - weight) * affinity + weight * other.affinity;
        energy_potential = (1.0f - weight) * energy_potential + weight * other.energy_potential;
        
        // Merge statistics
        activation_count += other.activation_count;
        cumulative_flow += other.cumulative_flow;
        age = std::min(age, other.age); // Keep younger age
    }
    
    // Create a variant connection through mutation (for fission)
    Connection mutate(float mutation_rate = 0.1f) const {
        Connection variant = *this;
        
        // Randomly perturb parameters
        auto mutate_param = [mutation_rate](float value, float min_val, float max_val) {
            float perturbation = mutation_rate * (2.0f * (rand() / (float)RAND_MAX) - 1.0f);
            return std::max(min_val, std::min(max_val, value + perturbation));
        };
        
        variant.conductivity = mutate_param(conductivity, 0.0f, 1.0f);
        variant.directional_bias = mutate_param(directional_bias, -1.0f, 1.0f);
        variant.plasticity_rate = mutate_param(plasticity_rate, 0.001f, 0.1f);
        variant.stability = mutate_param(stability, 0.1f, 1.0f);
        variant.affinity = mutate_param(affinity, 0.0f, 1.0f);
        
        // Reset metadata for new connection
        variant.age = 0;
        variant.activation_count = 0;
        variant.cumulative_flow = 0.0f;
        
        return variant;
    }
    
    // Classify emergent behavior type (for visualization/debugging only)
    std::string inferred_behavior_type() const {
        // This is NOT stored or used for logic - just for human understanding
        if (stability > 0.8f && plasticity_rate < 0.01f) {
            return "factual";  // Rigid, unchanging
        } else if (directional_bias > 0.5f) {
            return "sequential"; // Strong flow direction
        } else if (plasticity_rate > 0.05f) {
            return "exploratory"; // Highly adaptive
        } else if (affinity > 0.8f) {
            return "conceptual"; // Strong semantic link
        } else if (std::abs(directional_bias) < 0.1f && conductivity > 0.6f) {
            return "associative"; // Symmetric, strong
        } else {
            return "generic"; // No strong specialization yet
        }
    }
};

// Connection pool management
class ConnectionChemistry {
private:
    std::vector<Connection> connections;
    ChemistryConstants constants;
    
    // Indices for fast lookup
    std::unordered_map<uint64_t, std::vector<size_t>> outgoing; // node -> connection indices
    std::unordered_map<uint64_t, std::vector<size_t>> incoming; // node -> connection indices
    
    // Statistics
    uint64_t total_activations = 0;
    uint64_t total_prunings = 0;
    uint64_t total_fissions = 0;
    uint64_t total_fusions = 0;
    
public:
    ConnectionChemistry() = default;
    explicit ConnectionChemistry(const ChemistryConstants& consts) : constants(consts) {}
    
    // Add a new connection
    size_t add_connection(uint64_t a, uint64_t b, float initial_conductivity = 0.5f) {
        Connection conn(a, b);
        conn.conductivity = initial_conductivity;
        
        size_t idx = connections.size();
        connections.push_back(conn);
        
        outgoing[a].push_back(idx);
        incoming[b].push_back(idx);
        
        return idx;
    }
    
    // Get connection by index
    Connection& get(size_t idx) {
        return connections[idx];
    }
    
    const Connection& get(size_t idx) const {
        return connections[idx];
    }
    
    // Find connection between nodes
    size_t find_connection(uint64_t a, uint64_t b) const {
        if (outgoing.find(a) == outgoing.end()) return SIZE_MAX;
        
        for (size_t idx : outgoing.at(a)) {
            if (connections[idx].b == b) {
                return idx;
            }
        }
        return SIZE_MAX;
    }
    
    // Get all outgoing connections from a node
    std::vector<size_t> get_outgoing(uint64_t node_id) const {
        auto it = outgoing.find(node_id);
        return (it != outgoing.end()) ? it->second : std::vector<size_t>();
    }
    
    // Get all incoming connections to a node  
    std::vector<size_t> get_incoming(uint64_t node_id) const {
        auto it = incoming.find(node_id);
        return (it != incoming.end()) ? it->second : std::vector<size_t>();
    }
    
    // Update all connections based on current field state
    void metabolize(const std::vector<float>& node_activations,
                   const std::vector<float>& context_field,
                   const std::vector<float>& delta_field,
                   float global_fitness) {
        for (auto& conn : connections) {
            // Get node states
            float A_a = 0.0f, A_b = 0.0f;
            float delta_C_a = 0.0f, delta_C_b = 0.0f;
            
            if (conn.a < node_activations.size() && conn.b < node_activations.size()) {
                A_a = node_activations[conn.a];
                A_b = node_activations[conn.b];
            }
            
            if (conn.a < delta_field.size() && conn.b < delta_field.size()) {
                delta_C_a = delta_field[conn.a];
                delta_C_b = delta_field[conn.b];
            }
            
            // Update chemistry
            conn.update_chemistry(A_a, A_b, delta_C_a, delta_C_b, global_fitness, constants);
        }
    }
    
    // Prune dead connections
    size_t prune_connections() {
        std::vector<Connection> surviving;
        size_t pruned_count = 0;
        
        for (const auto& conn : connections) {
            if (!conn.should_prune(constants)) {
                surviving.push_back(conn);
            } else {
                pruned_count++;
            }
        }
        
        if (pruned_count > 0) {
            connections = surviving;
            rebuild_indices();
            total_prunings += pruned_count;
        }
        
        return pruned_count;
    }
    
    // Fuse similar connections between same nodes
    size_t fuse_connections() {
        std::vector<bool> merged(connections.size(), false);
        std::vector<Connection> fused;
        size_t fusion_count = 0;
        
        for (size_t i = 0; i < connections.size(); ++i) {
            if (merged[i]) continue;
            
            Connection base = connections[i];
            bool found_match = false;
            
            for (size_t j = i + 1; j < connections.size(); ++j) {
                if (merged[j]) continue;
                
                float sim = connections[i].similarity(connections[j]);
                if (sim > constants.fusion_similarity) {
                    // Merge j into base
                    base.merge_with(connections[j], 0.5f);
                    merged[j] = true;
                    found_match = true;
                    fusion_count++;
                }
            }
            
            fused.push_back(base);
            merged[i] = true;
        }
        
        if (fusion_count > 0) {
            connections = fused;
            rebuild_indices();
            total_fusions += fusion_count;
        }
        
        return fusion_count;
    }
    
    // Split volatile connections into variants
    size_t split_connections() {
        std::vector<Connection> expanded;
        size_t fission_count = 0;
        
        for (const auto& conn : connections) {
            expanded.push_back(conn);
            
            if (conn.should_split(constants)) {
                // Create mutated variant
                Connection variant = conn.mutate(0.2f);
                expanded.push_back(variant);
                fission_count++;
            }
        }
        
        if (fission_count > 0) {
            connections = expanded;
            rebuild_indices();
            total_fissions += fission_count;
        }
        
        return fission_count;
    }
    
    // Get chemistry constants (for meta-learning)
    ChemistryConstants& get_constants() {
        return constants;
    }
    
    const ChemistryConstants& get_constants() const {
        return constants;
    }
    
    // Statistics
    size_t size() const {
        return connections.size();
    }
    
    uint64_t get_total_activations() const {
        return total_activations;
    }
    
    void log_activation() {
        total_activations++;
    }
    
    // Get connection statistics for analysis
    struct Stats {
        float mean_conductivity = 0.0f;
        float mean_plasticity = 0.0f;
        float mean_stability = 0.0f;
        float mean_affinity = 0.0f;
        size_t total_connections = 0;
        size_t directional_connections = 0;
        size_t stable_connections = 0;
        size_t plastic_connections = 0;
    };
    
    Stats compute_stats() const {
        Stats stats;
        stats.total_connections = connections.size();
        
        if (connections.empty()) return stats;
        
        for (const auto& conn : connections) {
            stats.mean_conductivity += conn.conductivity;
            stats.mean_plasticity += conn.plasticity_rate;
            stats.mean_stability += conn.stability;
            stats.mean_affinity += conn.affinity;
            
            if (std::abs(conn.directional_bias) > constants.directional_threshold) {
                stats.directional_connections++;
            }
            if (conn.stability > 0.7f) {
                stats.stable_connections++;
            }
            if (conn.plasticity_rate > 0.05f) {
                stats.plastic_connections++;
            }
        }
        
        float n = static_cast<float>(connections.size());
        stats.mean_conductivity /= n;
        stats.mean_plasticity /= n;
        stats.mean_stability /= n;
        stats.mean_affinity /= n;
        
        return stats;
    }
    
private:
    // Rebuild lookup indices after modifications
    void rebuild_indices() {
        outgoing.clear();
        incoming.clear();
        
        for (size_t i = 0; i < connections.size(); ++i) {
            outgoing[connections[i].a].push_back(i);
            incoming[connections[i].b].push_back(i);
        }
    }
};

} // namespace melvin

