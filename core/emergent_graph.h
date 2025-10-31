/**
 * @file emergent_graph.h
 * @brief Biologically-inspired emergent graph where connections form based on need - cost
 * 
 * PRINCIPLE: Change = Need - Cost
 * 
 * Connections are not pre-designed. They emerge:
 * - Need: Information value, prediction improvement, utility
 * - Cost: Energy, memory, computation time
 * - Connection forms/grows when: need > cost
 * - Connection weakens/prunes when: need < cost
 * 
 * No hardcoded structures. Everything self-organizes through usage.
 */

#ifndef MELVIN_EMERGENT_GRAPH_H
#define MELVIN_EMERGENT_GRAPH_H

#include <unordered_map>
#include <vector>
#include <string>
#include <atomic>
#include <mutex>

namespace melvin {
namespace emergent {

/**
 * @brief A connection that emerged from co-activation patterns
 */
struct EmergentEdge {
    int to_node;
    float strength;           // Current connection strength (0-1)
    float need_signal;        // Accumulated information value
    float cost;               // Maintenance cost (energy/memory)
    int coactivations;        // Times nodes fired together
    float last_use_time;      // When last used
    float formation_rate;     // How fast this connection formed (learning rate)
    
    // Biological parameters
    float energy_consumption; // Cost per activation
    float information_value; // Benefit per activation
    
    EmergentEdge(int to, float initial_strength = 0.01f) :
        to_node(to),
        strength(initial_strength),
        need_signal(0.0f),
        cost(0.001f),  // Base maintenance cost
        coactivations(0),
        last_use_time(0.0f),
        formation_rate(0.01f),
        energy_consumption(0.001f),
        information_value(0.0f)
    {}
};

/**
 * @brief A node in the emergent graph
 */
struct EmergentNode {
    int id;
    std::string token;                    // Optional: word/concept
    std::vector<float> embedding;         // Semantic representation
    float activation;                     // Current energy level
    float total_need;                     // Accumulated need (utility)
    float total_cost;                     // Accumulated cost (energy spent)
    
    // Emergent connections (not pre-designed)
    std::unordered_map<int, EmergentEdge> edges;
    
    // Biological parameters
    float energy_budget;                  // Available energy
    float information_value;             // How useful this node is
    int usage_count;                     // How often used
    
    EmergentNode(int node_id, const std::string& t = "", const std::vector<float>& emb = {}) :
        id(node_id),
        token(t),
        embedding(emb),
        activation(0.0f),
        total_need(0.0f),
        total_cost(0.0f),
        energy_budget(1.0f),
        information_value(0.5f),
        usage_count(0)
    {}
};

/**
 * @brief Emergent graph - connections form based on need - cost
 * 
 * BIOLOGICAL PRINCIPLES:
 * 1. Neurons that fire together, wire together (Hebbian learning)
 * 2. Unused connections weaken and prune (synaptic elimination)
 * 3. High-value connections strengthen (long-term potentiation)
 * 4. Energy constraints limit growth (metabolic cost)
 */
class EmergentGraph {
public:
    EmergentGraph();
    ~EmergentGraph() = default;
    
    /**
     * @brief Activate nodes (input)
     * 
     * This is the ONLY way nodes get activated.
     * Connections will emerge from co-activation patterns.
     */
    void activate(const std::vector<int>& node_ids, float strength = 1.0f);
    
    /**
     * @brief Let activation spread through emergent connections
     * 
     * Activation flows through edges where: need > cost
     * Energy is consumed as it spreads.
     */
    void spread_activation(float dt = 1.0f);
    
    /**
     * @brief Update connections based on co-activation (need - cost)
     * 
     * This is called after activation spread:
     * - If nodes A and B fired together → strengthen edge A→B
     * - Need signal increases based on information value
     * - Cost accumulates based on energy consumption
     * - If need > cost → connection grows
     * - If need < cost → connection weakens/prunes
     */
    void update_connections();
    
    /**
     * @brief Prune weak connections where cost > need
     * 
     * Biological synaptic elimination.
     * Removes connections that aren't paying their metabolic cost.
     */
    void prune_weak_connections(float threshold = 0.01f);
    
    /**
     * @brief Add a new node (emerges when needed)
     * 
     * Returns new node ID or existing if token already exists.
     */
    int add_node(const std::string& token, const std::vector<float>& embedding);
    
    /**
     * @brief Get currently active nodes (activation > threshold)
     */
    std::vector<int> get_active_nodes(float threshold = 0.1f) const;
    
    /**
     * @brief Get activation level of a node
     */
    float get_activation(int node_id) const;
    
    /**
     * @brief Compute need signal for an edge
     * 
     * Need = InformationValue - Cost
     * InformationValue = prediction_improvement + utility + novelty
     */
    float compute_need(int from_id, int to_id) const;
    
    /**
     * @brief Compute cost for maintaining an edge
     * 
     * Cost = EnergyConsumption + MemoryCost + ComputationCost
     */
    float compute_cost(int from_id, int to_id) const;
    
    /**
     * @brief Get total energy budget (system-wide constraint)
     */
    float get_energy_budget() const { return total_energy_budget_; }
    void set_energy_budget(float budget) { total_energy_budget_ = budget; }
    
    /**
     * @brief Load existing graph (for persistence)
     */
    void load(const std::string& filepath);
    
    /**
     * @brief Save current graph (for persistence)
     */
    void save(const std::string& filepath) const;
    
    // Accessors
    const std::unordered_map<int, EmergentNode>& nodes() const { return nodes_; }
    std::unordered_map<int, EmergentNode>& nodes() { return nodes_; }
    
    const std::unordered_map<std::string, int>& token_to_id() const { return token_to_id_; }
    
private:
    // The graph (emerges through usage)
    std::unordered_map<int, EmergentNode> nodes_;
    std::unordered_map<std::string, int> token_to_id_;
    
    // Thread safety
    mutable std::mutex graph_mutex_;
    std::atomic<int> next_node_id_{0};
    
    // Energy system (biological constraint)
    float total_energy_budget_{100.0f};  // Total available energy
    float current_energy_used_{0.0f};    // Current consumption
    
    // Activation decay (biological)
    float decay_rate_{0.9f};             // Energy dissipates over time
    float spread_factor_{0.3f};          // How much activation spreads
    
    // Connection formation parameters
    float base_formation_rate_{0.01f};    // How fast connections form
    float base_elimination_rate_{0.005f}; // How fast unused connections weaken
    float need_threshold_{0.1f};          // Minimum need to maintain connection
    float cost_threshold_{0.05f};         // Maximum cost before pruning
    
    // Co-activation tracking (for Hebbian learning)
    std::unordered_map<int, float> recent_activations_;  // Node ID -> activation time
    float current_time_{0.0f};
    
    // Helpers
    void strengthen_edge(int from_id, int to_id, float delta);
    void weaken_edge(int from_id, int to_id, float delta);
    void create_edge_if_needed(int from_id, int to_id);
    float compute_information_value(int node_id) const;
    float compute_prediction_improvement(int from_id, int to_id) const;
    
    // Energy management
    void consume_energy(float amount);
    void replenish_energy(float dt);
};

} // namespace emergent
} // namespace melvin

#endif // MELVIN_EMERGENT_GRAPH_H

