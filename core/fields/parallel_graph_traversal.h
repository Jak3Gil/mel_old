#ifndef PARALLEL_GRAPH_TRAVERSAL_H
#define PARALLEL_GRAPH_TRAVERSAL_H

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <thread>
#include <atomic>
#include <mutex>
#include <future>
#include <algorithm>

namespace melvin {
namespace fields {

/**
 * @brief Parallel Graph Traversal - Brain-Like Reasoning
 * 
 * The human brain doesn't count "hops" - it spreads activation
 * through the graph until energy dissipates naturally.
 * 
 * This system:
 * - NO hop limits (travel as far as energy allows)
 * - Massively parallel (thousands of nodes per second)
 * - Energy-driven stopping (not artificial cutoffs)
 * - Supports complex reasoning through long chains
 */

// Activated node with path information
struct ActivatedNode {
    int node_id;
    float activation;           // Current activation level
    float path_energy;          // Total energy through this path
    int depth;                  // For statistics only, not a limit
    std::vector<int> path;      // Path taken (for reasoning transparency)
    
    ActivatedNode(int id, float act, float energy, int d, const std::vector<int>& p = {})
        : node_id(id), activation(act), path_energy(energy), depth(d), path(p) {}
};

// Graph traversal statistics
struct TraversalStats {
    size_t nodes_visited;
    size_t edges_traversed;
    int max_depth_reached;
    float total_energy_propagated;
    float avg_activation;
    std::chrono::milliseconds duration;
};

/**
 * @brief Parallel Graph Traversal Engine
 * 
 * Spreads activation through the graph in parallel, just like the brain.
 * Stops when energy dissipates, not at arbitrary hop counts.
 */
// Stability parameters (genome-driven)
struct FieldStabilityParams {
    float adaptive_theta;           // Adaptive threshold (raises under load)
    float beta_temperature;         // Diffusion intensity scaling
    float kWTA_percentile;          // k-Winner-Take-All inhibition (e.g., 0.9 = inhibit bottom 90%)
    float degree_normalization;     // Enable degree-based normalization
    float inhibition_strength;      // How much to suppress non-winners
    size_t max_active_nodes;        // Hard limit on active nodes
    float variance_convergence;     // Stop if variance < this
    int convergence_window;         // Rolling window for stability check
    
    FieldStabilityParams()
        : adaptive_theta(0.001f), beta_temperature(1.0f), kWTA_percentile(0.9f),
          degree_normalization(1.0f), inhibition_strength(0.1f),
          max_active_nodes(5000), variance_convergence(0.001f),
          convergence_window(20) {}
};

class ParallelGraphTraversal {
public:
    ParallelGraphTraversal();
    ~ParallelGraphTraversal();
    
    // Set stability parameters (from genome)
    void set_stability_params(const FieldStabilityParams& params) {
        stability_params_ = params;
    }
    
    /**
     * @brief Spread activation with biological stability constraints
     * 
     * NEW v3.1 Features:
     * - Degree normalization (prevents hub dominance)
     * - k-WTA inhibition (winner-take-all)
     * - Adaptive threshold (raises under load)
     * - Convergence detection (stops when stable)
     * - Backpressure (throttles if too many active)
     * 
     * @param origin_nodes Starting points (can be multiple for complex queries)
     * @param graph Adjacency list: node_id -> [(neighbor_id, edge_weight)]
     * @param embeddings Node embeddings for similarity-based traversal
     * @param min_activation_threshold Stop when activation falls below this
     * @param decay_per_step Energy decay per traversal step (0.9 = 10% decay)
     * @param max_nodes_to_activate Safety limit (prevent runaway)
     * @return All activated nodes with their activation levels
     */
    std::vector<ActivatedNode> spread_activation(
        const std::vector<int>& origin_nodes,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        float min_activation_threshold = 0.001f,
        float decay_per_step = 0.85f,
        size_t max_nodes_to_activate = 10000
    );
    
    /**
     * @brief Find reasoning chain between two concepts
     * 
     * Uses bidirectional search from both ends.
     * Returns the actual chain of reasoning.
     * 
     * @return Vector of node IDs forming the reasoning chain
     */
    std::vector<int> find_reasoning_chain(
        int start_node,
        int target_node,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        size_t max_chain_length = 1000  // Safety only, not a traversal limit
    );
    
    /**
     * @brief Get all nodes within energy radius
     * 
     * Like asking "what concepts are related to X?"
     * Returns everything reachable with sufficient energy.
     */
    std::unordered_set<int> get_energy_neighborhood(
        int origin_node,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        float energy_threshold = 0.01f
    );
    
    /**
     * @brief Compute activation spreading in parallel
     * 
     * Divides the graph into partitions and processes in parallel.
     * Returns a heat map of activations across the entire graph.
     */
    std::unordered_map<int, float> compute_activation_field(
        const std::vector<int>& origin_nodes,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        int num_iterations = 10,  // Iterate until convergence
        float decay_rate = 0.1f
    );
    
    // Get last traversal statistics
    TraversalStats get_last_stats() const { return last_stats_; }
    
    // Configure parallelism
    void set_num_threads(size_t num_threads);
    size_t get_num_threads() const { return num_threads_; }
    
    // Stability metrics
    struct StabilityMetrics {
        float energy_variance;
        float energy_mean;
        float entropy;
        size_t active_count;
        bool converged;
        bool backpressure_active;
    };
    StabilityMetrics get_stability_metrics() const { return stability_metrics_; }
    
private:
    size_t num_threads_;
    TraversalStats last_stats_;
    FieldStabilityParams stability_params_;
    StabilityMetrics stability_metrics_;
    
    // Rolling window for convergence detection
    std::vector<float> energy_variance_history_;
    std::vector<size_t> active_count_history_;
    
    // Parallel workers
    struct WorkerTask {
        std::vector<ActivatedNode> frontier;  // Nodes to process
        std::vector<ActivatedNode> results;   // Activated nodes found
    };
    
    // Stability functions
    void apply_degree_normalization(
        std::vector<ActivatedNode>& nodes,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph
    );
    
    void apply_kWTA_inhibition(std::vector<ActivatedNode>& nodes);
    
    float compute_adaptive_threshold(size_t active_count);
    
    bool check_convergence();
    
    void update_stability_metrics(const std::vector<ActivatedNode>& activated);
    
    // Worker function for parallel spreading
    void spread_worker(
        WorkerTask& task,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        std::unordered_map<int, float>& global_activations,
        std::mutex& activation_mutex,
        float min_threshold,
        float decay_rate
    );
    
    // Bidirectional search helper
    struct SearchFrontier {
        std::unordered_map<int, std::vector<int>> node_to_path;
        std::priority_queue<std::pair<float, int>> queue;  // (priority, node_id)
    };
    
    bool bidirectional_step(
        SearchFrontier& forward,
        SearchFrontier& backward,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        std::vector<int>& meeting_path
    );
};

/**
 * @brief Reasoning Path Analyzer
 * 
 * Analyzes the paths taken during graph traversal to understand
 * how MELVIN arrived at a conclusion.
 */
class ReasoningPathAnalyzer {
public:
    struct ReasoningStep {
        int from_node;
        int to_node;
        float edge_weight;
        float activation_at_step;
        std::string reasoning_type;  // "association", "causal", "temporal", etc.
    };
    
    struct ReasoningChain {
        std::vector<ReasoningStep> steps;
        float total_confidence;
        float avg_activation;
        int length;
        
        std::string to_string() const;
    };
    
    // Analyze how activation reached a target
    static ReasoningChain analyze_path(
        const std::vector<int>& node_path,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::string>& node_labels
    );
    
    // Find strongest reasoning chains
    static std::vector<ReasoningChain> find_strongest_chains(
        const std::vector<ActivatedNode>& activated_nodes,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        size_t top_k = 10
    );
};

} // namespace fields
} // namespace melvin

#endif // PARALLEL_GRAPH_TRAVERSAL_H

