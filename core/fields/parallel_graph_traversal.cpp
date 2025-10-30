#include "parallel_graph_traversal.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <sstream>

namespace melvin {
namespace fields {

ParallelGraphTraversal::ParallelGraphTraversal() 
    : num_threads_(std::thread::hardware_concurrency()) {
    if (num_threads_ == 0) num_threads_ = 4;  // Fallback
    
    // Initialize stability metrics
    stability_metrics_.energy_variance = 0.0f;
    stability_metrics_.energy_mean = 0.0f;
    stability_metrics_.entropy = 0.0f;
    stability_metrics_.active_count = 0;
    stability_metrics_.converged = false;
    stability_metrics_.backpressure_active = false;
}

ParallelGraphTraversal::~ParallelGraphTraversal() {}

void ParallelGraphTraversal::set_num_threads(size_t num_threads) {
    num_threads_ = std::max(size_t(1), num_threads);
}

std::vector<ActivatedNode> ParallelGraphTraversal::spread_activation(
    const std::vector<int>& origin_nodes,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    float min_activation_threshold,
    float decay_per_step,
    size_t max_nodes_to_activate) {
    
    auto start_time = std::chrono::high_resolution_clock::now();
    
    // Global activation map (thread-safe)
    std::unordered_map<int, float> global_activations;
    std::mutex activation_mutex;
    
    // Initialize origins with full energy
    std::vector<ActivatedNode> current_frontier;
    for (int node_id : origin_nodes) {
        current_frontier.emplace_back(node_id, 1.0f, 1.0f, 0, std::vector<int>{node_id});
        global_activations[node_id] = 1.0f;
    }
    
    std::vector<ActivatedNode> all_activated;
    all_activated.insert(all_activated.end(), current_frontier.begin(), current_frontier.end());
    
    size_t total_nodes_activated = origin_nodes.size();
    size_t total_edges_traversed = 0;
    int max_depth = 0;
    float total_energy = 0.0f;
    
    // v3.1: Apply adaptive threshold based on current load
    float current_threshold = compute_adaptive_threshold(total_nodes_activated);
    
    // Spread until energy dissipates (NO hop limit) OR convergence detected
    while (!current_frontier.empty() && total_nodes_activated < max_nodes_to_activate) {
        
        // v3.1: Check for early convergence
        if (stability_metrics_.converged) {
            break;  // Field has stabilized
        }
        
        // v3.1: Apply degree normalization to prevent hub dominance
        apply_degree_normalization(current_frontier, graph);
        
        // v3.1: Apply k-WTA inhibition
        apply_kWTA_inhibition(current_frontier);
        
        // Partition frontier across threads
        size_t partition_size = (current_frontier.size() + num_threads_ - 1) / num_threads_;
        std::vector<WorkerTask> tasks(num_threads_);
        
        for (size_t t = 0; t < num_threads_; ++t) {
            size_t start_idx = t * partition_size;
            size_t end_idx = std::min(start_idx + partition_size, current_frontier.size());
            
            if (start_idx < end_idx) {
                tasks[t].frontier.assign(
                    current_frontier.begin() + start_idx,
                    current_frontier.begin() + end_idx
                );
            }
        }
        
        // Launch parallel workers
        std::vector<std::future<void>> futures;
        for (auto& task : tasks) {
            if (task.frontier.empty()) continue;
            
            futures.push_back(std::async(std::launch::async,
                &ParallelGraphTraversal::spread_worker, this,
                std::ref(task), std::cref(graph), std::cref(embeddings),
                std::ref(global_activations), std::ref(activation_mutex),
                min_activation_threshold, decay_per_step
            ));
        }
        
        // Wait for all workers
        for (auto& f : futures) {
            f.get();
        }
        
        // Collect results for next iteration
        current_frontier.clear();
        for (auto& task : tasks) {
            for (auto& node : task.results) {
                // v3.1: Use adaptive threshold
                if (node.activation >= current_threshold) {
                    current_frontier.push_back(node);
                    all_activated.push_back(node);
                    total_nodes_activated++;
                    max_depth = std::max(max_depth, node.depth);
                    total_energy += node.activation;
                    
                    if (total_nodes_activated >= max_nodes_to_activate) break;
                }
            }
            total_edges_traversed += task.results.size();
        }
        
        // v3.1: Update stability metrics after each iteration
        update_stability_metrics(all_activated);
        
        // v3.1: Recompute adaptive threshold for next iteration
        current_threshold = compute_adaptive_threshold(total_nodes_activated);
    }
    
    auto end_time = std::chrono::high_resolution_clock::now();
    
    // Record statistics
    last_stats_.nodes_visited = total_nodes_activated;
    last_stats_.edges_traversed = total_edges_traversed;
    last_stats_.max_depth_reached = max_depth;
    last_stats_.total_energy_propagated = total_energy;
    last_stats_.avg_activation = (total_nodes_activated > 0) ? 
        (total_energy / total_nodes_activated) : 0.0f;
    last_stats_.duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - start_time);
    
    return all_activated;
}

void ParallelGraphTraversal::spread_worker(
    WorkerTask& task,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    std::unordered_map<int, float>& global_activations,
    std::mutex& activation_mutex,
    float min_threshold,
    float decay_rate) {
    
    task.results.clear();
    
    for (const auto& current : task.frontier) {
        // Find neighbors
        auto it = graph.find(current.node_id);
        if (it == graph.end()) continue;
        
        for (const auto& [neighbor_id, edge_weight] : it->second) {
            // Calculate new activation with decay
            float new_activation = current.activation * edge_weight * decay_rate;
            
            if (new_activation < min_threshold) continue;
            
            // Check if this is a better path to this node
            bool should_activate = false;
            {
                std::lock_guard<std::mutex> lock(activation_mutex);
                auto act_it = global_activations.find(neighbor_id);
                if (act_it == global_activations.end() || new_activation > act_it->second) {
                    global_activations[neighbor_id] = new_activation;
                    should_activate = true;
                }
            }
            
            if (should_activate) {
                // Build path
                std::vector<int> new_path = current.path;
                new_path.push_back(neighbor_id);
                
                task.results.emplace_back(
                    neighbor_id,
                    new_activation,
                    current.path_energy * edge_weight,
                    current.depth + 1,
                    new_path
                );
            }
        }
    }
}

std::vector<int> ParallelGraphTraversal::find_reasoning_chain(
    int start_node,
    int target_node,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    size_t max_chain_length) {
    
    if (start_node == target_node) {
        return {start_node};
    }
    
    // Bidirectional search
    SearchFrontier forward, backward;
    
    forward.node_to_path[start_node] = {start_node};
    forward.queue.push({1.0f, start_node});
    
    backward.node_to_path[target_node] = {target_node};
    backward.queue.push({1.0f, target_node});
    
    std::vector<int> result;
    
    // Alternate between forward and backward search
    for (size_t iter = 0; iter < max_chain_length; ++iter) {
        if (forward.queue.empty() && backward.queue.empty()) break;
        
        // Forward step
        if (!forward.queue.empty()) {
            if (bidirectional_step(forward, backward, graph, result)) {
                return result;
            }
        }
        
        // Backward step
        if (!backward.queue.empty()) {
            if (bidirectional_step(backward, forward, graph, result)) {
                // Reverse the backward part
                std::reverse(result.begin(), result.end());
                return result;
            }
        }
    }
    
    return {};  // No path found within limit
}

bool ParallelGraphTraversal::bidirectional_step(
    SearchFrontier& forward,
    SearchFrontier& backward,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    std::vector<int>& meeting_path) {
    
    if (forward.queue.empty()) return false;
    
    auto [priority, current_node] = forward.queue.top();
    forward.queue.pop();
    
    // Check if we've met the other search
    auto it = backward.node_to_path.find(current_node);
    if (it != backward.node_to_path.end()) {
        // Found meeting point!
        meeting_path = forward.node_to_path[current_node];
        auto& backward_path = it->second;
        // Append backward path (reversed)
        meeting_path.insert(meeting_path.end(), 
                           backward_path.rbegin() + 1, backward_path.rend());
        return true;
    }
    
    // Expand neighbors
    auto graph_it = graph.find(current_node);
    if (graph_it != graph.end()) {
        for (const auto& [neighbor_id, edge_weight] : graph_it->second) {
            if (forward.node_to_path.count(neighbor_id)) continue;
            
            auto new_path = forward.node_to_path[current_node];
            new_path.push_back(neighbor_id);
            forward.node_to_path[neighbor_id] = new_path;
            
            float new_priority = priority * edge_weight;
            forward.queue.push({new_priority, neighbor_id});
        }
    }
    
    return false;
}

std::unordered_set<int> ParallelGraphTraversal::get_energy_neighborhood(
    int origin_node,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    float energy_threshold) {
    
    std::unordered_set<int> neighborhood;
    std::unordered_map<int, float> activations;
    
    std::queue<std::pair<int, float>> frontier;
    frontier.push({origin_node, 1.0f});
    activations[origin_node] = 1.0f;
    neighborhood.insert(origin_node);
    
    while (!frontier.empty()) {
        auto [current_id, current_energy] = frontier.front();
        frontier.pop();
        
        auto it = graph.find(current_id);
        if (it == graph.end()) continue;
        
        for (const auto& [neighbor_id, edge_weight] : it->second) {
            float new_energy = current_energy * edge_weight * 0.85f;
            
            if (new_energy < energy_threshold) continue;
            
            if (activations.count(neighbor_id) && activations[neighbor_id] >= new_energy) {
                continue;
            }
            
            activations[neighbor_id] = new_energy;
            neighborhood.insert(neighbor_id);
            frontier.push({neighbor_id, new_energy});
        }
    }
    
    return neighborhood;
}

std::unordered_map<int, float> ParallelGraphTraversal::compute_activation_field(
    const std::vector<int>& origin_nodes,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    int num_iterations,
    float decay_rate) {
    
    std::unordered_map<int, float> activations;
    std::unordered_map<int, float> next_activations;
    
    // Initialize origins
    for (int node_id : origin_nodes) {
        activations[node_id] = 1.0f;
    }
    
    // Iterate until convergence
    for (int iter = 0; iter < num_iterations; ++iter) {
        next_activations.clear();
        
        // Spread activation
        for (const auto& [node_id, activation] : activations) {
            // Decay current activation
            float decayed = activation * (1.0f - decay_rate);
            next_activations[node_id] = decayed;
            
            // Spread to neighbors
            auto it = graph.find(node_id);
            if (it != graph.end()) {
                for (const auto& [neighbor_id, edge_weight] : it->second) {
                    float spread_energy = activation * edge_weight * decay_rate;
                    next_activations[neighbor_id] += spread_energy;
                }
            }
        }
        
        // Swap for next iteration
        activations = next_activations;
        
        // Remove very weak activations
        for (auto it = activations.begin(); it != activations.end(); ) {
            if (it->second < 0.001f) {
                it = activations.erase(it);
            } else {
                ++it;
            }
        }
    }
    
    return activations;
}

// ============================================================================
// ReasoningPathAnalyzer Implementation
// ============================================================================

std::string ReasoningPathAnalyzer::ReasoningChain::to_string() const {
    std::ostringstream oss;
    oss << "Reasoning Chain (length " << length << ", confidence " << total_confidence << "):\n";
    for (size_t i = 0; i < steps.size(); ++i) {
        const auto& step = steps[i];
        oss << "  Step " << (i+1) << ": " << step.from_node << " --[" 
            << step.reasoning_type << ", w=" << step.edge_weight 
            << "]--> " << step.to_node << " (act=" << step.activation_at_step << ")\n";
    }
    return oss.str();
}

ReasoningPathAnalyzer::ReasoningChain ReasoningPathAnalyzer::analyze_path(
    const std::vector<int>& node_path,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::string>& node_labels) {
    
    ReasoningChain chain;
    chain.length = node_path.size();
    chain.total_confidence = 1.0f;
    
    float activation = 1.0f;
    
    for (size_t i = 0; i < node_path.size() - 1; ++i) {
        int from_node = node_path[i];
        int to_node = node_path[i + 1];
        
        // Find edge weight
        float edge_weight = 0.0f;
        auto it = graph.find(from_node);
        if (it != graph.end()) {
            for (const auto& [neighbor, weight] : it->second) {
                if (neighbor == to_node) {
                    edge_weight = weight;
                    break;
                }
            }
        }
        
        activation *= edge_weight * 0.85f;  // Decay
        
        ReasoningStep step;
        step.from_node = from_node;
        step.to_node = to_node;
        step.edge_weight = edge_weight;
        step.activation_at_step = activation;
        step.reasoning_type = (edge_weight > 0.8f) ? "strong_association" : "weak_association";
        
        chain.steps.push_back(step);
        chain.total_confidence *= edge_weight;
    }
    
    chain.avg_activation = activation;
    
    return chain;
}

std::vector<ReasoningPathAnalyzer::ReasoningChain> ReasoningPathAnalyzer::find_strongest_chains(
    const std::vector<ActivatedNode>& activated_nodes,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    size_t top_k) {
    
    // Sort by path energy
    std::vector<ActivatedNode> sorted = activated_nodes;
    std::sort(sorted.begin(), sorted.end(),
        [](const ActivatedNode& a, const ActivatedNode& b) {
            return a.path_energy > b.path_energy;
        });
    
    std::vector<ReasoningChain> chains;
    
    for (size_t i = 0; i < std::min(top_k, sorted.size()); ++i) {
        const auto& node = sorted[i];
        if (node.path.size() < 2) continue;
        
        auto chain = analyze_path(node.path, graph, {});
        chains.push_back(chain);
    }
    
    return chains;
}

// ============================================================================
// Stability Functions Implementation (v3.1)
// ============================================================================

void ParallelGraphTraversal::apply_degree_normalization(
    std::vector<ActivatedNode>& nodes,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph) {
    
    if (stability_params_.degree_normalization <= 0.0f) return;
    
    for (auto& node : nodes) {
        auto it = graph.find(node.node_id);
        if (it != graph.end() && !it->second.empty()) {
            // Degree normalization: divide energy by sqrt(degree)
            float degree = static_cast<float>(it->second.size());
            float norm_factor = 1.0f / std::sqrt(degree);
            node.activation *= std::pow(norm_factor, stability_params_.degree_normalization);
        }
    }
}

void ParallelGraphTraversal::apply_kWTA_inhibition(std::vector<ActivatedNode>& nodes) {
    if (nodes.empty()) return;
    
    // Sort by activation (descending)
    std::vector<float> activations;
    for (const auto& node : nodes) {
        activations.push_back(node.activation);
    }
    std::sort(activations.begin(), activations.end(), std::greater<float>());
    
    // Find k-th percentile threshold
    size_t k_index = static_cast<size_t>(activations.size() * stability_params_.kWTA_percentile);
    if (k_index >= activations.size()) k_index = activations.size() - 1;
    
    float threshold = activations[k_index];
    
    // Inhibit nodes below threshold
    for (auto& node : nodes) {
        if (node.activation < threshold) {
            node.activation *= stability_params_.inhibition_strength;
        }
    }
}

float ParallelGraphTraversal::compute_adaptive_threshold(size_t active_count) {
    // Raise threshold if too many nodes are active (backpressure)
    float base_threshold = stability_params_.adaptive_theta;
    
    if (active_count > stability_params_.max_active_nodes) {
        float overload = static_cast<float>(active_count) / stability_params_.max_active_nodes;
        return base_threshold * overload;  // Linear increase
    }
    
    return base_threshold;
}

bool ParallelGraphTraversal::check_convergence() {
    if (energy_variance_history_.size() < static_cast<size_t>(stability_params_.convergence_window)) {
        return false;
    }
    
    // Check if variance has been low for entire window
    float sum = 0.0f;
    for (size_t i = energy_variance_history_.size() - stability_params_.convergence_window;
         i < energy_variance_history_.size(); ++i) {
        sum += energy_variance_history_[i];
    }
    float avg_variance = sum / stability_params_.convergence_window;
    
    // Also check that active count is stable
    size_t min_active = *std::min_element(
        active_count_history_.end() - stability_params_.convergence_window,
        active_count_history_.end()
    );
    size_t max_active = *std::max_element(
        active_count_history_.end() - stability_params_.convergence_window,
        active_count_history_.end()
    );
    
    bool variance_low = (avg_variance < stability_params_.variance_convergence);
    bool count_stable = ((max_active - min_active) < 10);  // Within 10 nodes
    
    return variance_low && count_stable;
}

void ParallelGraphTraversal::update_stability_metrics(const std::vector<ActivatedNode>& activated) {
    if (activated.empty()) {
        stability_metrics_.energy_mean = 0.0f;
        stability_metrics_.energy_variance = 0.0f;
        stability_metrics_.entropy = 0.0f;
        stability_metrics_.active_count = 0;
        return;
    }
    
    // Compute mean energy
    float sum = 0.0f;
    for (const auto& node : activated) {
        sum += node.activation;
    }
    stability_metrics_.energy_mean = sum / activated.size();
    
    // Compute variance
    float variance_sum = 0.0f;
    for (const auto& node : activated) {
        float diff = node.activation - stability_metrics_.energy_mean;
        variance_sum += diff * diff;
    }
    stability_metrics_.energy_variance = variance_sum / activated.size();
    
    // Compute entropy (Shannon entropy of activation distribution)
    float total_energy = sum;
    float entropy = 0.0f;
    for (const auto& node : activated) {
        if (node.activation > 0.0f) {
            float p = node.activation / total_energy;
            entropy -= p * std::log2(p);
        }
    }
    stability_metrics_.entropy = entropy;
    
    stability_metrics_.active_count = activated.size();
    
    // Update rolling windows
    energy_variance_history_.push_back(stability_metrics_.energy_variance);
    active_count_history_.push_back(stability_metrics_.active_count);
    
    // Keep window size bounded
    size_t max_window = stability_params_.convergence_window * 2;
    if (energy_variance_history_.size() > max_window) {
        energy_variance_history_.erase(energy_variance_history_.begin());
    }
    if (active_count_history_.size() > max_window) {
        active_count_history_.erase(active_count_history_.begin());
    }
    
    // Check convergence
    stability_metrics_.converged = check_convergence();
    
    // Check backpressure
    stability_metrics_.backpressure_active = 
        (stability_metrics_.active_count > stability_params_.max_active_nodes);
}

} // namespace fields
} // namespace melvin

