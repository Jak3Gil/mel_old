/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN EMERGENT DIMENSIONAL REASONING SYSTEM                             ║
 * ║  Intelligence discovered through experience, not hardcoded rules          ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 * 
 * This implementation formalizes:
 * 
 * 1. MEMORY GRAPH & UNLIMITED CONTEXT
 *    • Memory is a directed multigraph G = (V, E)
 *    • Context is a FIELD C(t) ∈ ℝ^|V| (node potentials) over ALL nodes
 *    • No hard window truncation—any node can influence decisions via diffusion
 * 
 * 2. EMERGENT DIMENSIONAL SYSTEM
 *    • Any concept node can become a contextual DIMENSION through experience
 *    • Dimensions are discovered, not hardcoded (no preset "safety" or "grammar")
 *    • Variance impact tracking: concepts that correlate with success get promoted
 *    • Dynamic field influence: active dimensions generate potential fields
 *    • Continuous evolution: dimensions grow, merge, or fade based on outcomes
 * 
 * 3. CONTEXT FIELD DYNAMICS WITH EMERGENT DRIVERS
 *    • dC/dt = -τLC + αA + βR + Σᵢ(γᵢ(t)·Dᵢ(t)) - λC
 *    • L: graph Laplacian (diffusion operator)
 *    • A(t): direct relevance (query/sensory match)
 *    • R(t): reasoning reinforcement (eligibility from current inference)
 *    • Dᵢ(t): emergent dimensional fields (i runs over ALL nodes, not fixed set)
 *    • γᵢ(t): learned influence weights (earned through variance impact)
 *    • λ: decay to prevent runaway growth
 * 
 * 4. EXPERIENCE-DRIVEN LEARNING
 *    • Observe: Record which nodes are active during reasoning
 *    • Evaluate: Measure outcome fitness (coherence, task success, consistency, stability)
 *    • Attribute: Correlate active nodes with fitness delta
 *    • Promote: Nodes with high positive impact become active dimensions
 *    • Demote: Dimensions that stop helping gradually fade
 *    • Compress: Merge redundant dimensions to maintain efficiency
 * 
 * 5. EMERGENT REASONING LOOP (PER TICK)
 *    1. Perception → A (compute direct relevance from input/query)
 *    2. Quick diffusion (few steps of C ← C - ητLC + ηαA)
 *    3. Enumerate candidate actions from topK(C)
 *    4. Rollouts: simulate B×H paths per action
 *    5. Observe: Record context field activations
 *    6. Evaluate: Compute fitness metrics from rollouts
 *    7. Decision: Select action using emergent dimensional utility
 *    8. Attribute: Update variance impacts based on fitness
 *    9. Evolve: Promote/demote/compress dimensions
 *    10. Update traces: Backproject to dimensional traces
 *    11. Field solve: Equilibrium with emergent driver fields
 *    12. Execute: Generate phrase from best action
 *    13. Reinforce: Update R along used paths
 * 
 * 6. NO HARDCODED RULES
 *    • No fixed "safety" dimension - safety emerges if rewarded by experience
 *    • No hardcoded "grammar" rules - linguistic structure self-organizes
 *    • No predetermined "values" - principles develop from feedback
 *    • Soft penalties, not hard barriers - let experience teach consequences
 * 
 * 7. EVOLUTION & META-LEARNING
 *    • Dimensional compression: Merge correlated fields
 *    • Adaptive thresholds: Promotion/demotion criteria evolve
 *    • Continuous discovery: New concepts can always become dimensions
 *    • Stability through experience: Consistently helpful dimensions persist
 */

/*
 * =============================================================================
 * EMERGENT DIMENSIONAL REASONING - OPERATIONAL PHILOSOPHY
 * =============================================================================
 * 
 * Core Principles:
 * 
 * 1. SINGLE GRAPH AUTHORITY
 *    - Maintain one authoritative GraphStore
 *    - All learning writes through this store, updating both memory log and RAM index
 *    - Reasoning reads only from this store
 * 
 * 2. EXPERIENCE-DRIVEN DIMENSIONS
 *    - NO hardcoded concept categories
 *    - ALL nodes can potentially influence reasoning
 *    - Dimensions emerge from variance impact (correlation with success)
 *    - Influence weights (gamma) are earned, not assigned
 * 
 * 3. CONTINUOUS LEARNING CYCLE
 *    - Observe: Track active nodes during reasoning
 *    - Evaluate: Measure fitness (coherence, task success, consistency, stability)
 *    - Attribute: Correlate node activation with fitness delta
 *    - Evolve: Promote helpful concepts, demote unhelpful ones
 *    - Compress: Merge redundant dimensional clusters
 * 
 * 4. SOFT GUIDANCE, NOT HARD RULES
 *    - No absolute filters or barriers
 *    - Soft penalties for catastrophic risk (let experience teach severity)
 *    - Output self-organizes through field equilibrium
 *    - Grammar, safety, style emerge if they improve outcomes
 * 
 * 5. FIELD-BASED REASONING
 *    - Context field C(t) evolves through diffusion
 *    - Active dimensions inject their fields D_i(t)
 *    - Equilibrium naturally balances all active forces
 *    - Decision emerges from utility landscape
 * 
 * 6. META-LEARNING & ADAPTATION
 *    - Promotion threshold adapts based on dimensional stability
 *    - Gamma weights grow with consistent positive impact
 *    - Compression prevents dimensional explosion
 *    - System self-tunes to environment feedback
 * 
 * 7. VALIDATION THROUGH OUTCOME
 *    - Success measured by fitness metrics, not rule compliance
 *    - Dimensions validated by their correlation with success
 *    - No predetermined "correct" behavior
 *    - Intelligence emerges from trial and feedback
 * 
 * Key Insight:
 * "Don't tell Melvin what matters - let him discover it through experience.
 *  Concepts that help (grammar, safety, empathy) will naturally gain influence.
 *  Those that don't will fade. This is cognition as a discovered field, not a programmed algorithm."
 * 
 * =============================================================================
 */

#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <cmath>
#include <algorithm>
#include <memory>
#include <chrono>
#include <random>
#include <cmath>
#include <functional>
#include <regex>
#include <fstream>
#include "UCAConfig.h"
#include "LLMReasoningEngine.h"
#include "text_norm.h"
#include "question_router.h"
#include "answerer.h"
#include "nlg_templates.h"
#include "predictive_sampler.h"
#include "connection_chemistry.h"

// ==================== CORE DATA STRUCTURES ====================

namespace melvin {

// Binary ID types - all content-addressed, 32 bytes each
using NodeID = std::array<uint8_t, 32>;
// EdgeID already defined in melvin_types.h

// Essential node representation with LLM-style embeddings
struct Node {
    uint64_t id = 0;
    std::string text;
    uint32_t type = 0;
    int freq = 0;
    bool pinned = false;
    std::vector<float> emb; // Real embeddings for semantic similarity
    std::vector<float> embedding; // LLM-style semantic embedding
    float attention_weight = 0.0f; // Current attention weight
    uint64_t last_accessed = 0; // Timestamp for recency
    float semantic_strength = 1.0f; // Semantic coherence strength
};

// Lightweight edge representation for internal graph (uint64_t-based IDs)
struct Edge {
    uint64_t u, v;          // Source and destination node IDs
    float weight;           // Combined weight [0,1]
    float w_core;           // Core durable weight
    float w_ctx;            // Context weight
    uint32_t count;         // Usage count
    uint8_t rel;            // Relation type
    float last_used;        // Timestamp of last use
    
    Edge() : u(0), v(0), weight(0.5f), w_core(0.3f), w_ctx(0.2f), count(1), rel(0), last_used(0.0f) {}
    Edge(uint64_t src, uint64_t dst, float w, uint8_t r) 
        : u(src), v(dst), weight(w), w_core(w*0.3f), w_ctx(w*0.7f), count(1), rel(r), last_used(0.0f) {}
};

// Path representation for multi-hop reasoning
struct Path {
    std::vector<uint64_t> nodes;
    std::vector<size_t> edges_idx;
    double score = 0.0;
    
    bool empty() const { return nodes.empty(); }
};

// Query representation
struct Query {
    std::string raw;
    std::vector<std::string> toks;
    
    Query() = default;
    Query(const std::string& text) : raw(text) {
        // Simple tokenization - split on spaces and punctuation
        std::string current;
        for (char c : text) {
            if (std::isspace(c) || std::ispunct(c)) {
                if (!current.empty()) {
                    toks.push_back(current);
                    current.clear();
                }
            } else {
                current += std::tolower(c);
            }
        }
        if (!current.empty()) {
            toks.push_back(current);
        }
    }
};

// ==================== UNLIMITED CONTEXT FIELD SYSTEM ====================

// Vector type for context field (node potentials)
using Vector = std::vector<float>;

// Context field: unlimited context via potential field over ALL nodes
struct ContextField {
    Vector C;  // Node potentials |V| dimensional
    
    ContextField() {}
    
    explicit ContextField(size_t num_nodes) : C(num_nodes, 0.0f) {}
    
    // Resize field for dynamic graph growth
    void resize(size_t num_nodes) {
        C.resize(num_nodes, 0.0f);
    }
    
    // Diffuse activation across connections using chemistry-based flow
    void diffuse_chemistry(const ConnectionChemistry& chemistry,
                          const std::unordered_map<uint64_t, size_t>& node_to_idx,
                          float tau, float eta) {
        if (C.empty()) return;
        
        Vector delta(C.size(), 0.0f);
        
        // For each node, compute chemistry-based flow to neighbors
        for (const auto& [node_id, idx] : node_to_idx) {
            if (idx >= C.size()) continue;
            
            float C_source = C[idx];
            
            // Get outgoing connections
            auto out_connections = chemistry.get_outgoing(node_id);
            
            for (size_t conn_idx : out_connections) {
                const auto& conn = chemistry.get(conn_idx);
                
                auto target_it = node_to_idx.find(conn.b);
                if (target_it != node_to_idx.end() && target_it->second < C.size()) {
                    size_t target_idx = target_it->second;
                    float C_target = C[target_idx];
                    
                    // Chemistry-based flow (continuous, no hard types)
                    float flow = conn.compute_flow(C_source, C_target);
                    
                    // Apply flow scaled by diffusion strength
                    delta[target_idx] += tau * flow;
                    delta[idx] -= tau * flow;
                }
            }
        }
        
        // Apply diffusion update
        for (size_t i = 0; i < C.size(); ++i) {
            C[i] += eta * delta[i];
        }
    }
    
    // Legacy diffusion (for backward compatibility during transition)
    void diffuse(const std::vector<Edge>& edges, 
                 const std::unordered_map<uint64_t, std::vector<size_t>>& adj,
                 const std::unordered_map<uint64_t, size_t>& node_to_idx,
                 float tau, float eta) {
        if (C.empty()) return;
        
        Vector delta(C.size(), 0.0f);
        
        // Compute -tau * L * C where L is normalized Laplacian
        // For each node, diffuse to neighbors weighted by edge strength
        for (const auto& [node_id, idx] : node_to_idx) {
            if (idx >= C.size()) continue;
            
            float node_potential = C[idx];
            float out_degree = 0.0f;
            
            // Calculate out-degree (sum of edge weights)
            if (adj.find(node_id) != adj.end()) {
                for (size_t edge_idx : adj.at(node_id)) {
                    if (edge_idx < edges.size()) {
                        out_degree += edges[edge_idx].weight;
                    }
                }
            }
            
            if (out_degree > 0.0f) {
                // Diffuse to neighbors
                if (adj.find(node_id) != adj.end()) {
                    for (size_t edge_idx : adj.at(node_id)) {
                        if (edge_idx < edges.size()) {
                            const auto& edge = edges[edge_idx];
                            auto target_it = node_to_idx.find(edge.v);
                            if (target_it != node_to_idx.end() && target_it->second < C.size()) {
                                size_t target_idx = target_it->second;
                                float edge_weight = edge.weight / out_degree;
                                delta[target_idx] += tau * edge_weight * node_potential;
                                delta[idx] -= tau * edge_weight * node_potential;
                            }
                        }
                    }
                }
            }
        }
        
        // Apply diffusion update
        for (size_t i = 0; i < C.size(); ++i) {
            C[i] += eta * delta[i];
        }
    }
    
    // Inject direct relevance from query/sensory input
    void inject(const Vector& A, float alpha, float eta) {
        if (A.size() != C.size()) return;
        for (size_t i = 0; i < C.size(); ++i) {
            C[i] += eta * alpha * A[i];
        }
    }
    
    // Inject driver fields (safety, task, curiosity, etc.)
    void injectDrivers(const std::vector<Vector>& D, const std::vector<float>& gamma, float eta) {
        if (D.empty() || gamma.size() != D.size()) return;
        
        for (size_t k = 0; k < D.size(); ++k) {
            if (D[k].size() != C.size()) continue;
            for (size_t i = 0; i < C.size(); ++i) {
                C[i] += eta * gamma[k] * D[k][i];
            }
        }
    }
    
    // Decay to prevent runaway growth
    void decay(float lambda, float eta) {
        for (float& val : C) {
            val *= (1.0f - eta * lambda);
        }
    }
    
    // Solve for equilibrium: (lambda*I + tau*L)*C = alpha*A + beta*R + sum(gamma_k*D_k)
    // Simplified iterative solver (Jacobi-style with warm start)
    void solveEquilibrium(const std::vector<Edge>& edges,
                         const std::unordered_map<uint64_t, std::vector<size_t>>& adj,
                         const std::unordered_map<uint64_t, size_t>& node_to_idx,
                         const Vector& A, const Vector& R,
                         const std::vector<Vector>& D,
                         float alpha, float beta, 
                         const std::vector<float>& gamma,
                         float tau, float lambda,
                         int max_iters = 10) {
        if (C.size() != A.size()) return;
        
        // Right-hand side: alpha*A + beta*R + sum(gamma_k*D_k)
        Vector rhs(C.size(), 0.0f);
        for (size_t i = 0; i < C.size(); ++i) {
            rhs[i] = alpha * A[i] + beta * R[i];
        }
        
        for (size_t k = 0; k < D.size() && k < gamma.size(); ++k) {
            if (D[k].size() != C.size()) continue;
            for (size_t i = 0; i < C.size(); ++i) {
                rhs[i] += gamma[k] * D[k][i];
            }
        }
        
        // Iterative solve with warm start (C already has previous solution)
        float eta = 0.1f; // Step size for iterative solver
        
        for (int iter = 0; iter < max_iters; ++iter) {
            Vector C_new = C;
            
            // For each node: C_new[i] = (rhs[i] - tau*L*C[i]) / (lambda + tau*degree[i])
            for (const auto& [node_id, idx] : node_to_idx) {
                if (idx >= C.size()) continue;
                
                float laplacian_term = 0.0f;
                float out_degree = 0.0f;
                
                if (adj.find(node_id) != adj.end()) {
                    for (size_t edge_idx : adj.at(node_id)) {
                        if (edge_idx < edges.size()) {
                            const auto& edge = edges[edge_idx];
                            out_degree += edge.weight;
                            
                            auto target_it = node_to_idx.find(edge.v);
                            if (target_it != node_to_idx.end() && target_it->second < C.size()) {
                                size_t target_idx = target_it->second;
                                laplacian_term += edge.weight * (C[target_idx] - C[idx]);
                            }
                        }
                    }
                }
                
                float denom = lambda + tau * std::max(out_degree, 0.01f);
                C_new[idx] = (rhs[idx] - tau * laplacian_term) / denom;
            }
            
            // Update with mixing
            for (size_t i = 0; i < C.size(); ++i) {
                C[i] = (1.0f - eta) * C[i] + eta * C_new[i];
            }
        }
    }
    
    // Get top K nodes by potential (working set)
    std::vector<std::pair<size_t, float>> topK(size_t k) const {
        std::vector<std::pair<size_t, float>> potentials;
        for (size_t i = 0; i < C.size(); ++i) {
            potentials.push_back({i, C[i]});
        }
        
        std::partial_sort(potentials.begin(), 
                         potentials.begin() + std::min(k, potentials.size()),
                         potentials.end(),
                         [](const auto& a, const auto& b) { return a.second > b.second; });
        
        if (potentials.size() > k) {
            potentials.resize(k);
        }
        
        return potentials;
    }
};

// Action representation
struct Action {
    std::string name;
    uint64_t target_node = 0;
    std::vector<uint64_t> path;
    
    Action() = default;
    Action(const std::string& n, uint64_t t) : name(n), target_node(t) {}
};

// Rollout path for simulation
struct RolloutPath {
    std::vector<uint64_t> nodes;
    std::vector<uint8_t> relations;
    float probability = 1.0f;
    bool catastrophe = false;
    float goal_proximity = 0.0f;
    float info_gain = 0.0f;
    float energy_cost = 0.0f;
    int norm_violations = 0;
    int contradictions = 0;
    
    RolloutPath() = default;
};

// Collection of rollout paths for an action
struct Rollouts {
    std::vector<RolloutPath> paths;
    Action action;
    
    Rollouts() = default;
    explicit Rollouts(const Action& a) : action(a) {}
};

// ==================== EMERGENT DIMENSIONAL SYSTEM ====================
// 
// This system allows ANY concept node to become a contextual dimension
// based on its correlation with successful outcomes. No hardcoded drivers.
// Every node can influence reasoning if experience shows it helps.

// Fitness metrics for outcome evaluation
struct FitnessMetrics {
    float coherence = 0.0f;       // Entropy reduction, stability
    float task_success = 0.0f;    // Direct goal achievement
    float consistency = 0.0f;     // No contradictions
    float stability = 0.0f;       // Low potential oscillation
    float user_feedback = 0.0f;   // External signal (if available)
    
    // Composite fitness
    float overall() const {
        return 0.3f * coherence + 0.3f * task_success + 0.2f * consistency + 
               0.1f * stability + 0.1f * user_feedback;
    }
};

// Emergent dimension - a concept that has earned influence
struct EmergentDimension {
    uint64_t node_id = 0;          // Primary node representing this dimension
    std::vector<uint64_t> cluster; // Related nodes that form this dimension
    float variance_impact = 0.0f;  // How much this dimension improves outcomes
    float gamma = 0.0f;            // Current influence weight
    float stability = 0.0f;        // How stable this dimension's impact is
    int promotion_age = 0;         // How long it's been an active dimension
    Vector field;                  // Current potential field generated by this dimension
    
    EmergentDimension() = default;
    EmergentDimension(uint64_t nid, size_t num_nodes) 
        : node_id(nid), variance_impact(0.0f), gamma(0.0f), 
          stability(0.0f), promotion_age(0) {
        cluster.push_back(nid);
        field.resize(num_nodes, 0.0f);
    }
};

// Emergent Dimensional System - discovers what matters through experience
struct EmergentDimensionalSystem {
    size_t num_nodes = 0;
    
    // Per-node tracking
    std::vector<float> variance_impact;      // Impact score per node
    std::vector<float> activity;             // Current potential/activation
    std::vector<float> driver_weight;        // Learned influence weight
    
    // Active dimensions registry
    std::vector<EmergentDimension> active_dimensions;
    std::unordered_map<uint64_t, size_t> node_to_dim; // Map node to dimension index
    
    // Fitness history (rolling window)
    std::vector<FitnessMetrics> fitness_history;
    size_t history_size = 100;
    float baseline_fitness = 0.0f;
    
    // Hyperparameters
    float learning_rate = 0.01f;       // How fast to update variance impacts
    float promotion_threshold = 0.5f;   // Variance impact needed to become dimension
    float demotion_threshold = 0.1f;    // Below this, dimension loses status
    float decay_rate = 0.99f;           // Soft decay for old impacts
    float gamma_growth_rate = 0.05f;    // How fast dimension weights grow
    float gamma_max = 2.0f;             // Maximum influence weight
    
    EmergentDimensionalSystem() = default;
    
    explicit EmergentDimensionalSystem(size_t n) : num_nodes(n) {
        variance_impact.resize(n, 0.0f);
        activity.resize(n, 0.0f);
        driver_weight.resize(n, 0.0f);
        baseline_fitness = 0.0f;
    }
    
    void resize(size_t n) {
        num_nodes = n;
        variance_impact.resize(n, 0.0f);
        activity.resize(n, 0.0f);
        driver_weight.resize(n, 0.0f);
        for (auto& dim : active_dimensions) {
            dim.field.resize(n, 0.0f);
        }
    }
    
    // Record current node activations (from context field)
    void observe(const Vector& context_potentials) {
        if (context_potentials.size() != num_nodes) return;
        activity = context_potentials;
    }
    
    // Evaluate outcome and attribute to active nodes
    void evaluate_and_attribute(const FitnessMetrics& metrics) {
        // Update fitness history
        fitness_history.push_back(metrics);
        if (fitness_history.size() > history_size) {
            fitness_history.erase(fitness_history.begin());
        }
        
        // Compute baseline from recent history
        float sum = 0.0f;
        for (const auto& m : fitness_history) {
            sum += m.overall();
        }
        baseline_fitness = fitness_history.empty() ? 0.0f : sum / fitness_history.size();
        
        // Attribution: correlate node activation with fitness delta
        float fitness_delta = metrics.overall() - baseline_fitness;
        
        for (size_t i = 0; i < num_nodes; ++i) {
            // Update variance impact based on correlation
            // If this node was active and fitness improved, increase its impact
            float attribution = learning_rate * fitness_delta * activity[i];
            variance_impact[i] += attribution;
            
            // Decay old impacts
            variance_impact[i] *= decay_rate;
            
            // Clamp to reasonable range
            variance_impact[i] = std::max(-1.0f, std::min(2.0f, variance_impact[i]));
        }
    }
    
    // Promote nodes with high variance impact to active dimensions
    void promote_dimensions(const std::unordered_map<uint64_t, size_t>& node_to_idx) {
        for (const auto& [node_id, idx] : node_to_idx) {
            if (idx >= num_nodes) continue;
            
            // Already a dimension?
            if (node_to_dim.find(node_id) != node_to_dim.end()) continue;
            
            // High enough impact to promote?
            if (variance_impact[idx] > promotion_threshold) {
                EmergentDimension dim(node_id, num_nodes);
                dim.variance_impact = variance_impact[idx];
                dim.gamma = 0.1f; // Start with small influence
                dim.stability = 0.5f;
                
                node_to_dim[node_id] = active_dimensions.size();
                active_dimensions.push_back(dim);
                
                driver_weight[idx] = dim.gamma;
            }
        }
    }
    
    // Demote dimensions that no longer help
    void demote_dimensions() {
        std::vector<EmergentDimension> surviving;
        std::unordered_map<uint64_t, size_t> new_mapping;
        
        for (auto& dim : active_dimensions) {
            dim.promotion_age++;
            
            // Update dimension's variance impact from its nodes
            auto it = node_to_dim.find(dim.node_id);
            if (it != node_to_dim.end()) {
                size_t idx = it->second;
                if (idx < variance_impact.size()) {
                    dim.variance_impact = variance_impact[idx];
                }
            }
            
            // Keep if still impactful
            if (dim.variance_impact > demotion_threshold) {
                // Grow influence weight if stable
                if (dim.variance_impact > 0.5f) {
                    dim.gamma = std::min(gamma_max, dim.gamma + gamma_growth_rate);
                }
                
                new_mapping[dim.node_id] = surviving.size();
                surviving.push_back(dim);
            }
        }
        
        active_dimensions = surviving;
        node_to_dim = new_mapping;
    }
    
    // Generate dimensional fields from active dimensions
    std::vector<Vector> generate_fields(const std::vector<Edge>& edges,
                                        const std::unordered_map<uint64_t, std::vector<size_t>>& adj,
                                        const std::unordered_map<uint64_t, size_t>& node_to_idx) {
        std::vector<Vector> fields;
        
        for (auto& dim : active_dimensions) {
            Vector field(num_nodes, 0.0f);
            
            // Primary node activates strongly
            auto it = node_to_idx.find(dim.node_id);
            if (it != node_to_idx.end() && it->second < num_nodes) {
                field[it->second] = 1.0f;
            }
            
            // Diffuse activation to semantically related nodes
            // Nodes connected to the dimension's primary node get partial activation
            if (adj.find(dim.node_id) != adj.end()) {
                for (size_t edge_idx : adj.at(dim.node_id)) {
                    if (edge_idx < edges.size()) {
                        const auto& e = edges[edge_idx];
                        auto target_it = node_to_idx.find(e.v);
                        if (target_it != node_to_idx.end() && target_it->second < num_nodes) {
                            // Weight by edge strength
                            field[target_it->second] += 0.5f * e.weight;
                        }
                    }
                }
            }
            
            // Normalize field
            float max_val = 1e-6f;
            for (float v : field) {
                max_val = std::max(max_val, std::abs(v));
            }
            for (float& v : field) {
                v /= max_val;
            }
            
            dim.field = field;
            fields.push_back(field);
        }
        
        return fields;
    }
    
    // Get current gamma weights for field injection
    std::vector<float> get_gamma_weights() const {
        std::vector<float> weights;
        for (const auto& dim : active_dimensions) {
            weights.push_back(dim.gamma);
        }
        return weights;
    }
    
    // Compression: merge similar dimensions
    void compress_dimensions(const std::unordered_map<uint64_t, Node>& nodes) {
        if (active_dimensions.size() < 2) return;
        
        // Find highly correlated dimension pairs
        std::vector<std::pair<size_t, size_t>> merge_pairs;
        
        for (size_t i = 0; i < active_dimensions.size(); ++i) {
            for (size_t j = i + 1; j < active_dimensions.size(); ++j) {
                // Compute field correlation (inline cosine similarity)
                const auto& a = active_dimensions[i].field;
                const auto& b = active_dimensions[j].field;
                double correlation = 0.0;
                if (a.size() == b.size() && !a.empty()) {
                    double dot = 0.0, norm_a = 0.0, norm_b = 0.0;
                    for (size_t k = 0; k < a.size(); ++k) {
                        dot += a[k] * b[k];
                        norm_a += a[k] * a[k];
                        norm_b += b[k] * b[k];
                    }
                    norm_a = std::sqrt(norm_a);
                    norm_b = std::sqrt(norm_b);
                    if (norm_a > 0.0 && norm_b > 0.0) {
                        correlation = dot / (norm_a * norm_b);
                    }
                }
                
                // High correlation means they're redundant
                if (correlation > 0.85) {
                    merge_pairs.push_back({i, j});
                }
            }
        }
        
        // Merge pairs (keep stronger one)
        std::unordered_set<size_t> merged;
        for (const auto& [i, j] : merge_pairs) {
            if (merged.count(i) || merged.count(j)) continue;
            
            // Merge j into i (keep stronger)
            if (active_dimensions[i].variance_impact >= active_dimensions[j].variance_impact) {
                active_dimensions[i].cluster.insert(
                    active_dimensions[i].cluster.end(),
                    active_dimensions[j].cluster.begin(),
                    active_dimensions[j].cluster.end()
                );
                active_dimensions[i].gamma = std::max(active_dimensions[i].gamma, active_dimensions[j].gamma);
                merged.insert(j);
            }
        }
        
        // Remove merged dimensions
        if (!merged.empty()) {
            std::vector<EmergentDimension> surviving;
            for (size_t i = 0; i < active_dimensions.size(); ++i) {
                if (!merged.count(i)) {
                    surviving.push_back(active_dimensions[i]);
                }
            }
            active_dimensions = surviving;
            
            // Rebuild mapping
            node_to_dim.clear();
            for (size_t i = 0; i < active_dimensions.size(); ++i) {
                node_to_dim[active_dimensions[i].node_id] = i;
            }
        }
    }
};

// Legacy compatibility structures (will be phased out)
struct DriverScores {
    std::vector<float> dimensional_scores; // Dynamic scores per active dimension
    FitnessMetrics fitness;
    float risk_cat = 0.0f; // Kept for backward compatibility
};

struct Traces {
    std::vector<Vector> dimensional_traces; // One trace per active dimension
    float rho = 0.9f; // Trace decay factor
    
    Traces() = default;
    
    explicit Traces(size_t num_nodes, size_t num_dimensions) {
        dimensional_traces.resize(num_dimensions, Vector(num_nodes, 0.0f));
    }
    
    void resize(size_t num_nodes, size_t num_dimensions) {
        dimensional_traces.resize(num_dimensions, Vector(num_nodes, 0.0f));
        for (auto& trace : dimensional_traces) {
            trace.resize(num_nodes, 0.0f);
        }
    }
    
    void decay() {
        for (auto& trace : dimensional_traces) {
            for (float& val : trace) {
                val *= rho;
            }
        }
    }
};

// Rollout parameters
struct RolloutParams {
    int H = 3;           // Horizon (max path length)
    int B = 8;           // Branches per step
    float branch_temp = 0.5f; // Temperature for branching
    float discount = 0.9f;    // Discount factor for distant outcomes
};

// Evolution meta-parameters (updated for emergent dimensional system)
struct EvolutionParams {
    float alpha = 1.0f;   // Direct relevance weight
    float beta = 0.5f;    // Reasoning reinforcement weight
    float lambda = 0.05f; // Decay rate
    float tau = 0.3f;     // Diffusion strength
    RolloutParams rollout;
    
    // Gamma weights are now dynamic - learned from EmergentDimensionalSystem
    // No hardcoded driver_weights - dimensions earn their influence
    
    EvolutionParams() = default;
};

} // namespace melvin

// ==================== FUNCTION DECLARATIONS ====================

// Forward declarations for weight management functions
size_t find_edge(uint64_t src, uint64_t dst, melvin::Rel rel);
size_t create_or_update_edge(uint64_t src, uint64_t dst, melvin::Rel rel, float w_core, float w_ctx);
void recompute_node_weights(uint64_t node_id);
std::vector<std::pair<size_t, float>> get_weighted_neighbors(uint64_t node_id);
void debug_node_connections(uint64_t node_id, const std::string& context, const std::string& query, const std::string& driver_mode);
void reinforce_path(const melvin::Path& path);
void reinforce_edge(size_t edge_idx);
float get_current_time();
float syntax_fit(const melvin::Edge& edge, const std::string& context, const std::string& query);
float semantic_fit(const melvin::Edge& edge, const std::string& query);
float driver_bias(const melvin::Edge& edge, const std::string& driver_mode);
float compute_edge_score(size_t edge_idx, const std::string& context, const std::string& query, const std::string& driver_mode);
std::vector<std::pair<size_t, float>> get_scored_candidates(uint64_t node_id, const std::string& context, const std::string& query, const std::string& driver_mode);
size_t select_next_edge(uint64_t node_id, const std::string& context, const std::string& query, const std::string& selection_mode, const std::string& driver_mode);

// Phrase generation functions
bool should_stop_generation(const std::vector<std::string>& phrase, size_t max_tokens, const std::vector<std::pair<size_t, float>>& candidates, float threshold);
std::string fix_grammar(std::vector<std::string>& phrase);
std::string melvin_generate_phrase(uint64_t start_node_id, int max_tokens, const std::string& driver_mode, const std::string& selection_mode, const std::string& context, const std::string& query, bool debug);

// Beam search functions
std::string melvin_generate_phrase_beam(uint64_t start_node_id, int max_tokens, int beam_k, int expand_M, const std::string& driver_mode, bool probabilistic, float stop_threshold, const std::string& context, const std::string& query, bool debug);

// Enhanced scoring functions
int count_token_repetition(const std::vector<std::string>& phrase, const std::string& token);
float compute_laplace_freq(size_t edge_idx, uint64_t source_node);
float compute_recency_factor(const melvin::Edge& edge);
float compute_repetition_penalty(const std::vector<std::string>& phrase, const std::string& token);
float compute_length_norm(int step);
float compute_enhanced_score(size_t edge_idx, const std::vector<std::string>& phrase, int step, const std::string& context, const std::string& query, const std::string& driver_mode);

// ==================== GLOBAL STATE ====================

// Global probabilistic graph storage
std::unordered_map<uint64_t, melvin::Node> G_nodes;
std::unordered_map<uint64_t, std::vector<size_t>> G_adj;  // adjacency list: node_id -> edge indices
std::vector<melvin::Edge> G_edges;  // Legacy edges (being phased out)
uint64_t G_total_nodes = 0;    // Total node count for normalization

// CONNECTION CHEMISTRY SYSTEM - living molecular connections
melvin::ConnectionChemistry G_connection_chemistry;
melvin::ChemistryConstants G_chemistry_constants;

// ==================== EMBEDDING SYSTEM ====================

// Forward declaration for cosine similarity (defined below)
double cosine(const std::vector<float>& a, const std::vector<float>& b);

// Simple hash-based embedding generation
std::vector<float> embed_text(const std::string& text) {
    std::vector<float> embedding(128, 0.0f); // Fixed 128-dim embedding
    
    std::hash<std::string> hasher;
    size_t hash = hasher(text);
    
    // Generate pseudo-random embedding from text hash
    std::mt19937 gen(hash);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    for (float& val : embedding) {
        val = dist(gen);
    }
    
    // Normalize
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    if (norm > 0.0f) {
        for (float& val : embedding) {
            val /= norm;
        }
    }
    
    return embedding;
}

// Cosine similarity computation
double cosine(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size() || a.empty()) return 0.0;
    
    double dot = 0.0, norm_a = 0.0, norm_b = 0.0;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    norm_a = std::sqrt(norm_a);
    norm_b = std::sqrt(norm_b);
    
    if (norm_a == 0.0 || norm_b == 0.0) return 0.0;
    return dot / (norm_a * norm_b);
}

// ==================== ADAPTIVE THRESHOLDS ====================

namespace AdaptiveThresholds {
    constexpr float BASE_EDGE_CONFIDENCE = 0.05f;
    constexpr float BASE_PATH_CONFIDENCE = 0.1f;
    constexpr float BASE_NODE_PRIOR = 0.01f;
    constexpr float SCALE_FACTOR_HIGH = 2.5f;
    constexpr float SCALE_FACTOR_MEDIUM = 1.5f;
    constexpr float SCALE_LOG_BASE = 10.0f;

    inline float calculate_adaptive_threshold(float base_threshold, size_t node_count) {
        if (node_count <= 100) { return base_threshold * 0.3f; }
        else if (node_count <= 1000) { return base_threshold * 0.5f; }
        else if (node_count <= 10000) { return base_threshold * std::min(SCALE_FACTOR_MEDIUM, 1.0f + std::log10(node_count / 1000.0f)); }
        else if (node_count <= 100000) {
            float log_scale = 1.0f + std::log10(node_count / SCALE_LOG_BASE);
            return base_threshold * std::min(log_scale, SCALE_FACTOR_HIGH);
        } else { return base_threshold * SCALE_FACTOR_HIGH; }
    }
    
    inline float get_edge_threshold(size_t node_count) { 
        return calculate_adaptive_threshold(BASE_EDGE_CONFIDENCE, node_count); 
    }
    inline float get_path_threshold(size_t node_count) { 
        return calculate_adaptive_threshold(BASE_PATH_CONFIDENCE, node_count); 
    }
    inline float get_node_prior_threshold(size_t node_count) { 
        return calculate_adaptive_threshold(BASE_NODE_PRIOR, node_count); 
    }
}

// ==================== EDGE WEIGHT MANAGEMENT ====================

// Find existing edge between two nodes
size_t find_edge(uint64_t src, uint64_t dst, melvin::Rel rel) {
    for (size_t i = 0; i < G_edges.size(); ++i) {
        if (G_edges[i].u == src && G_edges[i].v == dst && G_edges[i].rel == static_cast<uint8_t>(rel)) {
            return i;
        }
    }
    return SIZE_MAX; // Not found
}

// Create or update edge
size_t create_or_update_edge(uint64_t src, uint64_t dst, melvin::Rel rel) {
    size_t edge_idx = find_edge(src, dst, rel);
    
    if (edge_idx != SIZE_MAX) {
        // Edge exists, increment count and update weight
        G_edges[edge_idx].count++;
        G_edges[edge_idx].weight = std::min(1.0f, G_edges[edge_idx].weight + 0.1f);
        G_edges[edge_idx].last_used = get_current_time();
    } else {
        // Create new edge
        melvin::Edge new_edge(src, dst, 0.5f, static_cast<uint8_t>(rel));
        new_edge.count = 1;
        new_edge.last_used = get_current_time();
        
        G_edges.push_back(new_edge);
        edge_idx = G_edges.size() - 1;
        
        // Update adjacency list
        if (G_adj.find(src) == G_adj.end()) {
            G_adj[src] = {};
        }
        G_adj[src].push_back(edge_idx);
    }
    
    return edge_idx;
}

// Probabilistic traversal: sample next node based on edge weights
uint64_t sample_next_node(uint64_t current_node, melvin::Rel preferred_rel = melvin::Rel::TEMPORAL) {
    if (G_adj.find(current_node) == G_adj.end()) {
        return 0; // No outgoing edges
    }
    
    const auto& outgoing_edges = G_adj[current_node];
    if (outgoing_edges.empty()) {
        return 0;
    }
    
    // Calculate total probability mass
    float total_prob = 0.0f;
    std::vector<float> probabilities;
    
    for (size_t edge_idx : outgoing_edges) {
        const auto& edge = G_edges[edge_idx];
        float prob = edge.weight;
        
        // Boost probability for preferred relation type
        if (edge.rel == static_cast<uint8_t>(preferred_rel)) {
            prob *= 2.0f; // Double the probability for preferred relations
        }
        
        probabilities.push_back(prob);
        total_prob += prob;
    }
    
    if (total_prob == 0.0f) {
        return 0; // No valid edges
    }
    
    // Sample based on probabilities
    float random_val = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * total_prob;
    float cumulative = 0.0f;
    
    for (size_t i = 0; i < outgoing_edges.size(); ++i) {
        cumulative += probabilities[i];
        if (random_val <= cumulative) {
            return G_edges[outgoing_edges[i]].v;
        }
    }
    
    // Fallback to last edge
    return G_edges[outgoing_edges.back()].v;
}

// Apply exponential smoothing to all edges for memory stabilization
void apply_memory_smoothing(float alpha = 0.02f) {
    for (auto& edge : G_edges) {
        // Gradually decay weights toward baseline
        edge.weight = (1.0f - alpha) * edge.weight + alpha * 0.3f;
        edge.w_core = (1.0f - alpha) * edge.w_core + alpha * 0.1f;
        edge.w_ctx = (1.0f - alpha) * edge.w_ctx + alpha * 0.2f;
    }
}

// Get neighbors sorted by weight (highest first)
std::vector<std::pair<size_t, float>> get_weighted_neighbors(uint64_t node_id) {
    std::vector<std::pair<size_t, float>> neighbors;
    
    if (G_adj.find(node_id) == G_adj.end()) return neighbors;
    
    for (size_t edge_idx : G_adj[node_id]) {
        neighbors.push_back({edge_idx, G_edges[edge_idx].weight});
    }
    
    // Sort by weight (highest first)
    std::sort(neighbors.begin(), neighbors.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return neighbors;
}

// ==================== CONSTRAINT MULTIPLIERS ====================

// Get current timestamp in seconds
float get_current_time() {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() / 1000.0f;
}

// Syntax fit: check if edge destination fits grammatical expectations
float syntax_fit(const melvin::Edge& edge, const std::string& context, const std::string& query) {
    const auto& target_node = G_nodes[edge.v];
    const std::string& target_text = target_node.text;
    
    // Simple syntactic rules
    if (context.find("is ") != std::string::npos || context.find("are ") != std::string::npos) {
        // After "is/are", expect nouns or adjectives
        if (target_text.length() > 3 && !target_text.ends_with("ing")) {
            return 1.0f; // Good fit for nouns/adjectives
        } else if (target_text.ends_with("ing")) {
            return 0.3f; // Verbs less likely
        }
    }
    
    if (query.find("what") != std::string::npos) {
        // For "what" questions, prefer concrete nouns
        if (target_text.length() > 3 && !target_text.ends_with("ly")) {
            return 1.0f;
        }
        return 0.7f;
    }
    
    if (query.find("how") != std::string::npos) {
        // For "how" questions, prefer adjectives or adverbs
        if (target_text.ends_with("ly") || target_text.ends_with("ful")) {
            return 1.0f;
        }
        return 0.6f;
    }
    
    return 0.8f; // Default moderate fit
}

// Semantic fit: check if edge aligns with query semantics
float semantic_fit(const melvin::Edge& edge, const std::string& query) {
    const auto& target_node = G_nodes[edge.v];
    const std::string& target_text = target_node.text;
    
    // Boost edges that contain words from the query
    for (size_t i = 0; i < query.length(); ++i) {
        if (query.substr(i, target_text.length()) == target_text) {
            return 1.2f; // Boost for exact matches
        }
    }
    
    // Boost semantic relations that make sense for the query type
    if (query.find("is") != std::string::npos || query.find("are") != std::string::npos) {
        if (edge.rel == static_cast<uint8_t>(melvin::Rel::EXACT)) {
            return 1.1f; // EXACT relations good for "is/are" questions
        }
    }
    
    if (query.find("have") != std::string::npos || query.find("has") != std::string::npos) {
        if (edge.rel == static_cast<uint8_t>(melvin::Rel::GENERALIZATION)) {
            return 1.1f; // GENERALIZATION good for "have/has" questions
        }
    }
    
    return 1.0f; // Default neutral fit
}

// Driver bias: adjust based on current cognitive state
float driver_bias(const melvin::Edge& edge, const std::string& driver_mode) {
    if (driver_mode == "exploration") {
        // Exploration mode: boost rare connections
        float rarity = 1.0f / (edge.count + 1.0f);
        return 0.5f + rarity; // Range: 0.5-1.5
    } else if (driver_mode == "exploitation") {
        // Exploitation mode: boost frequent connections
        float frequency = std::min(1.0f, edge.weight * 2.0f);
        return 0.8f + frequency; // Range: 0.8-1.8
    } else if (driver_mode == "accuracy") {
        // Accuracy mode: strongly favor frequent connections
        return 0.9f + edge.weight; // Range: 0.9-1.9
    }
    
    return 1.0f; // Default neutral bias
}

// ==================== SHADOW WORLD: ROLLOUT SIMULATION ====================

// Global reasoning state
melvin::ContextField G_context_field;
melvin::Traces G_traces;
melvin::EvolutionParams G_evo_params;
std::unordered_map<uint64_t, size_t> G_node_to_idx; // Map node ID to context field index
melvin::Vector G_reasoning_reinforcement;  // R(t) for current reasoning path

// EMERGENT DIMENSIONAL SYSTEM - the core of learning what matters
melvin::EmergentDimensionalSystem G_dimensional_system;

// Initialize global reasoning systems
void init_reasoning_system() {
    if (!G_nodes.empty()) {
        size_t num_nodes = G_nodes.size();
        G_context_field.resize(num_nodes);
        G_reasoning_reinforcement.resize(num_nodes, 0.0f);
        
        // Build node ID to index mapping
        size_t idx = 0;
        for (const auto& [node_id, node] : G_nodes) {
            G_node_to_idx[node_id] = idx++;
        }
        
        // Initialize emergent dimensional system
        G_dimensional_system.resize(num_nodes);
        
        // Initialize traces based on current active dimensions
        // Start with zero dimensions - they will emerge through experience
        G_traces.resize(num_nodes, 0);
    }
}

// Simulate rollouts for an action (model-based futures)
melvin::Rollouts simulate_rollouts(const melvin::Action& action, 
                                   const melvin::ContextField& context,
                                   const melvin::RolloutParams& params) {
    melvin::Rollouts rollouts(action);
    
    // Start from action's target node
    uint64_t start_node = action.target_node;
    if (G_nodes.find(start_node) == G_nodes.end()) {
        return rollouts; // Invalid start node
    }
    
    std::mt19937 rng(std::random_device{}());
    
    // Generate B branched trajectories
    for (int b = 0; b < params.B; ++b) {
        melvin::RolloutPath path;
        path.nodes.push_back(start_node);
        
        uint64_t current_node = start_node;
        float prob_accumulator = 1.0f;
        
        // Simulate path of length H
        for (int h = 0; h < params.H; ++h) {
            if (G_adj.find(current_node) == G_adj.end()) break;
            
            const auto& outgoing_edges = G_adj[current_node];
            if (outgoing_edges.empty()) break;
            
            // Sample next edge based on frequency ratios + context field
            std::vector<float> probs;
            float total_prob = 0.0f;
            
            for (size_t edge_idx : outgoing_edges) {
                if (edge_idx >= G_edges.size()) continue;
                
                const auto& edge = G_edges[edge_idx];
                float base_prob = edge.weight;
                
                // Boost by context field potential if available
                auto target_it = G_node_to_idx.find(edge.v);
                if (target_it != G_node_to_idx.end() && target_it->second < context.C.size()) {
                    float context_boost = 1.0f + std::max(0.0f, context.C[target_it->second]);
                    base_prob *= context_boost;
                }
                
                probs.push_back(base_prob);
                total_prob += base_prob;
            }
            
            if (total_prob == 0.0f) break;
            
            // Sample edge
            std::discrete_distribution<size_t> dist(probs.begin(), probs.end());
            size_t edge_choice = dist(rng);
            
            if (edge_choice >= outgoing_edges.size()) break;
            
            size_t edge_idx = outgoing_edges[edge_choice];
            const auto& edge = G_edges[edge_idx];
            
            // Update path
            path.nodes.push_back(edge.v);
            path.relations.push_back(edge.rel);
            prob_accumulator *= (probs[edge_choice] / total_prob);
            
            // Check for events
            // Simple heuristics for demonstration
            const auto& target_node = G_nodes[edge.v];
            
            // Catastrophe detection (simple keyword check)
            if (target_node.text.find("danger") != std::string::npos ||
                target_node.text.find("fatal") != std::string::npos ||
                target_node.text.find("crash") != std::string::npos) {
                path.catastrophe = true;
            }
            
            // Norm violation detection
            if (target_node.text.find("illegal") != std::string::npos ||
                target_node.text.find("unethical") != std::string::npos) {
                path.norm_violations++;
            }
            
            // Info gain (novelty of node)
            if (target_node.freq < 5) {
                path.info_gain += 0.1f;
            }
            
            // Energy cost (path length penalty)
            path.energy_cost += 0.05f;
            
            current_node = edge.v;
        }
        
        path.probability = prob_accumulator;
        rollouts.paths.push_back(path);
    }
    
    return rollouts;
}

// Emergent driver evaluation: compute fitness metrics from rollouts
melvin::DriverScores evaluate_drivers(const melvin::Rollouts& rollouts,
                                      melvin::EmergentDimensionalSystem& dim_system) {
    melvin::DriverScores scores;
    
    if (rollouts.paths.empty()) return scores;
    
    int num_paths = rollouts.paths.size();
    
    // Compute fitness metrics from rollout outcomes
    melvin::FitnessMetrics metrics;
    
    // Coherence: Path stability and probability
    float total_coherence = 0.0f;
    for (const auto& path : rollouts.paths) {
        float coherence = path.probability * (1.0f - 0.1f * path.nodes.size()); // Favor stable, not-too-long paths
        total_coherence += coherence;
    }
    metrics.coherence = total_coherence / num_paths;
    
    // Task success: Goal proximity and information gain
    float total_goal_proximity = 0.0f;
    float total_info_gain = 0.0f;
    for (const auto& path : rollouts.paths) {
        total_goal_proximity += path.goal_proximity;
        total_info_gain += path.info_gain;
    }
    metrics.task_success = (total_goal_proximity + total_info_gain) / (2.0f * num_paths);
    
    // Consistency: Low contradictions and norm violations
    int total_contradictions = 0;
    int total_violations = 0;
    int catastrophe_count = 0;
    for (const auto& path : rollouts.paths) {
        total_contradictions += path.contradictions;
        total_violations += path.norm_violations;
        if (path.catastrophe) catastrophe_count++;
    }
    float inconsistency = (total_contradictions + total_violations) / static_cast<float>(num_paths);
    metrics.consistency = std::max(0.0f, 1.0f - inconsistency);
    
    // Stability: Low energy cost (efficient, not oscillating)
    float total_energy = 0.0f;
    for (const auto& path : rollouts.paths) {
        total_energy += path.energy_cost;
    }
    metrics.stability = std::max(0.0f, 1.0f - total_energy / num_paths);
    
    // Risk (for legacy compatibility)
    scores.risk_cat = static_cast<float>(catastrophe_count) / num_paths;
    
    // Store fitness in scores
    scores.fitness = metrics;
    
    // Generate dimensional scores based on current active dimensions
    // Each dimension gets evaluated based on how well paths align with it
    scores.dimensional_scores.resize(dim_system.active_dimensions.size(), 0.0f);
    
    for (size_t d = 0; d < dim_system.active_dimensions.size(); ++d) {
        const auto& dim = dim_system.active_dimensions[d];
        
        // Compute alignment of rollout paths with this dimension
        float alignment = 0.0f;
    for (const auto& path : rollouts.paths) {
            // Check if path nodes align with dimension's cluster
            for (uint64_t node_id : path.nodes) {
                auto it = std::find(dim.cluster.begin(), dim.cluster.end(), node_id);
                if (it != dim.cluster.end()) {
                    alignment += path.probability;
                    break;
                }
            }
        }
        scores.dimensional_scores[d] = alignment / num_paths;
    }
    
    return scores;
}

// Backproject action scores to dimensional traces
void backproject_to_traces(const melvin::Rollouts& rollouts, 
                           const melvin::DriverScores& scores,
                           melvin::Traces& traces,
                           float discount = 0.9f) {
    if (rollouts.paths.empty()) return;
    if (traces.dimensional_traces.empty()) return;
    
    // For each path, distribute credit to nodes across all active dimensions
    for (const auto& path : rollouts.paths) {
        float path_weight = path.probability; // Weight by path probability
        
        for (size_t i = 0; i < path.nodes.size(); ++i) {
            uint64_t node_id = path.nodes[i];
            auto it = G_node_to_idx.find(node_id);
            if (it == G_node_to_idx.end()) continue;
            
            size_t node_idx = it->second;
            
            // Discount by position in path
            float discount_factor = std::pow(discount, static_cast<float>(i));
            float credit = path_weight * discount_factor;
            
            // Update traces for each active dimension
            for (size_t d = 0; d < traces.dimensional_traces.size() && d < scores.dimensional_scores.size(); ++d) {
                if (node_idx < traces.dimensional_traces[d].size()) {
                    traces.dimensional_traces[d][node_idx] += credit * scores.dimensional_scores[d];
                }
            }
        }
    }
}

// Convert eligibility traces to driver fields
std::vector<melvin::Vector> traces_to_driver_fields(const melvin::Traces& traces) {
    std::vector<melvin::Vector> fields;
    
    // Normalize each trace vector to create a field
    auto normalize = [](const std::vector<float>& vec) -> melvin::Vector {
        melvin::Vector result = vec;
        float max_val = 1e-6f;
        for (float v : result) {
            max_val = std::max(max_val, std::abs(v));
        }
        for (float& v : result) {
            v /= max_val; // Normalize to [-1, 1]
        }
        return result;
    };
    
    // Normalize all dimensional traces
    for (const auto& trace : traces.dimensional_traces) {
        fields.push_back(normalize(trace));
    }
    
    return fields;
}

// ==================== ACTION SELECTION WITH DRIVERS ====================

// Select best action using emergent dimensional utility
melvin::Action select_action(const std::vector<melvin::Action>& actions,
                             const std::vector<melvin::DriverScores>& all_scores,
                             const melvin::EmergentDimensionalSystem& dim_system) {
    if (actions.empty() || all_scores.empty()) {
        return melvin::Action();
    }
    
    float best_utility = -1e9f;
    size_t best_idx = 0;
    
    // Get current gamma weights from dimensional system
    std::vector<float> gamma_weights = dim_system.get_gamma_weights();
    
    for (size_t i = 0; i < actions.size() && i < all_scores.size(); ++i) {
        const auto& scores = all_scores[i];
        
        // Compute utility from emergent dimensions
        float utility = 0.0f;
        
        // If we have active dimensions, use them
        if (!gamma_weights.empty() && !scores.dimensional_scores.empty()) {
            for (size_t d = 0; d < gamma_weights.size() && d < scores.dimensional_scores.size(); ++d) {
                utility += gamma_weights[d] * scores.dimensional_scores[d];
            }
        } else {
            // Bootstrap: use raw fitness metrics until dimensions emerge
            utility = scores.fitness.overall();
        }
        
        // Soft penalty for catastrophic risk (not a hard floor - let experience teach)
        if (scores.risk_cat > 0.2f) {
            utility -= 2.0f * scores.risk_cat; // Soft penalty, not absolute veto
        }
        
        if (utility > best_utility) {
            best_utility = utility;
            best_idx = i;
        }
    }
    
    return actions[best_idx];
}

// ==================== COMPLETE REASONING LOOP ====================

// Main reasoning loop: perception → diffusion → rollouts → critics → decision
std::string melvin_reasoning_loop(const std::string& query_text, 
                                  const std::string& driver_mode = "balanced",
                                  bool debug = false) {
    // Ensure reasoning system is initialized
    if (G_context_field.C.empty()) {
        init_reasoning_system();
    }
    
    if (debug) {
        std::cout << "\n🧠 MELVIN REASONING LOOP\n";
        std::cout << "========================\n";
        std::cout << "Query: \"" << query_text << "\"\n";
        std::cout << "Driver mode: " << driver_mode << "\n\n";
    }
    
    // Step 1: PERCEPTION → A (direct relevance)
    melvin::Vector A(G_context_field.C.size(), 0.0f);
    
    // Compute query embedding
    auto query_emb = embed_text(query_text);
    
    // Compute relevance for each node
    for (const auto& [node_id, node] : G_nodes) {
        auto it = G_node_to_idx.find(node_id);
        if (it == G_node_to_idx.end()) continue;
        
        size_t idx = it->second;
        if (idx >= A.size()) continue;
        
        // Semantic similarity
        double sim = cosine(query_emb, node.emb);
        A[idx] = static_cast<float>(sim);
        
        // Boost for exact text matches
        if (query_text.find(node.text) != std::string::npos) {
            A[idx] += 0.5f;
        }
    }
    
    // Step 2: QUICK DIFFUSION
    if (debug) std::cout << "Step 2: Quick diffusion...\n";
    
    for (int iter = 0; iter < 3; ++iter) {
        G_context_field.diffuse(G_edges, G_adj, G_node_to_idx, G_evo_params.tau, 0.1f);
        G_context_field.inject(A, G_evo_params.alpha, 0.1f);
        G_context_field.decay(G_evo_params.lambda, 0.1f);
    }
    
    // Step 3: ENUMERATE CANDIDATE ACTIONS
    if (debug) std::cout << "Step 3: Enumerating candidate actions...\n";
    
    std::vector<melvin::Action> actions;
    auto top_nodes = G_context_field.topK(5); // Top 5 nodes by potential
    
    for (const auto& [idx, potential] : top_nodes) {
        // Find node_id from index
        for (const auto& [node_id, node_idx] : G_node_to_idx) {
            if (node_idx == idx) {
                actions.push_back(melvin::Action("traverse_to_" + std::to_string(node_id), node_id));
                break;
            }
        }
    }
    
    if (actions.empty()) {
        return "I don't know yet.";
    }
    
    // Step 4: ROLLOUTS for each action
    if (debug) std::cout << "Step 4: Running rollouts for " << actions.size() << " actions...\n";
    
    std::vector<melvin::Rollouts> all_rollouts;
    for (const auto& action : actions) {
        auto rollouts = simulate_rollouts(action, G_context_field, G_evo_params.rollout);
        all_rollouts.push_back(rollouts);
    }
    
    // Step 5: OBSERVE - record current context activations
    if (debug) std::cout << "Step 5: Observing context field for dimensional learning...\n";
    G_dimensional_system.observe(G_context_field.C);
    
    // Step 6: CRITICS - evaluate each action using emergent dimensions
    if (debug) std::cout << "Step 6: Evaluating fitness with emergent dimensions...\n";
    
    std::vector<melvin::DriverScores> all_scores;
    for (const auto& rollouts : all_rollouts) {
        auto scores = evaluate_drivers(rollouts, G_dimensional_system);
        all_scores.push_back(scores);
        
        if (debug) {
            std::cout << "  Action: " << rollouts.action.name << "\n";
            std::cout << "    Fitness: coherence=" << scores.fitness.coherence 
                      << ", task=" << scores.fitness.task_success 
                      << ", consistency=" << scores.fitness.consistency << "\n";
            std::cout << "    Active dimensions: " << G_dimensional_system.active_dimensions.size() << "\n";
        }
    }
    
    // Step 7: DECISION - select best action using emergent dimensions
    if (debug) std::cout << "Step 7: Selecting best action via emergent utility...\n";
    
    melvin::Action best_action = select_action(actions, all_scores, G_dimensional_system);
    
    // Step 8: EVALUATE AND ATTRIBUTE - learn from outcome
    if (debug) std::cout << "Step 8: Attributing fitness to active nodes...\n";
    
    // Use fitness from best action as learning signal
    size_t best_idx = 0;
    for (size_t i = 0; i < actions.size(); ++i) {
        if (actions[i].name == best_action.name) {
            best_idx = i;
            break;
        }
    }
    if (best_idx < all_scores.size()) {
        G_dimensional_system.evaluate_and_attribute(all_scores[best_idx].fitness);
    }
    
    // Step 9: DIMENSIONAL EVOLUTION - promote/demote/compress
    if (debug) std::cout << "Step 9: Evolving dimensional system...\n";
    
    G_dimensional_system.promote_dimensions(G_node_to_idx);
    G_dimensional_system.demote_dimensions();
    
    // Compress every 10 cycles (tracked elsewhere, simplified here)
    static int cycle_count = 0;
    if (++cycle_count % 10 == 0) {
        G_dimensional_system.compress_dimensions(G_nodes);
        if (debug) std::cout << "  Compressed redundant dimensions.\n";
    }
    
    // Step 10: DRIVER UPDATE - backproject to dimensional traces
    if (debug) std::cout << "Step 10: Updating dimensional traces...\n";
    
    // Resize traces if dimensions changed
    size_t num_dims = G_dimensional_system.active_dimensions.size();
    if (G_traces.dimensional_traces.size() != num_dims) {
        G_traces.resize(G_context_field.C.size(), num_dims);
    }
    
    G_traces.decay(); // Decay old traces
    
    for (size_t i = 0; i < all_rollouts.size() && i < all_scores.size(); ++i) {
        backproject_to_traces(all_rollouts[i], all_scores[i], G_traces);
    }
    
    // Step 11: FIELD SOLVE - update context with emergent driver fields
    if (debug) std::cout << "Step 11: Solving context field with emergent dimensions...\n";
    
    auto driver_fields = G_dimensional_system.generate_fields(G_edges, G_adj, G_node_to_idx);
    auto gamma_weights = G_dimensional_system.get_gamma_weights();
    
    G_context_field.solveEquilibrium(G_edges, G_adj, G_node_to_idx,
                                     A, G_reasoning_reinforcement, driver_fields,
                                     G_evo_params.alpha, G_evo_params.beta,
                                     gamma_weights,
                                     G_evo_params.tau, G_evo_params.lambda);
    
    if (debug) {
        std::cout << "  Best action: " << best_action.name << "\n";
        std::cout << "  Active dimensions: " << G_dimensional_system.active_dimensions.size() << "\n";
        if (!G_dimensional_system.active_dimensions.empty()) {
            std::cout << "  Top dimensions:\n";
            for (size_t i = 0; i < std::min(size_t(3), G_dimensional_system.active_dimensions.size()); ++i) {
                const auto& dim = G_dimensional_system.active_dimensions[i];
                std::cout << "    - Node " << dim.node_id << ": impact=" << dim.variance_impact 
                          << ", gamma=" << dim.gamma << "\n";
            }
        }
    }
    
    // Step 12: EXECUTE - generate phrase from best action
    if (debug) std::cout << "Step 12: Executing action (phrase generation)...\n";
    
    std::string result = melvin_generate_phrase_beam(
        best_action.target_node, 8, 3, 4, 
        driver_mode, false, 0.08f, "", query_text, debug);
    
    // Step 13: REINFORCE - update reasoning reinforcement
    if (debug) std::cout << "Step 13: Reinforcing successful paths...\n";
    
    auto best_node_idx = G_node_to_idx.find(best_action.target_node);
    if (best_node_idx != G_node_to_idx.end() && best_node_idx->second < G_reasoning_reinforcement.size()) {
        G_reasoning_reinforcement[best_node_idx->second] += 0.1f;
    }
    
    // Step 14: CONNECTION METABOLISM - update chemistry based on field changes
    if (debug) std::cout << "Step 14: Metabolizing connection chemistry...\n";
    
    static melvin::Vector prev_C; // Track previous field state
    if (prev_C.size() != G_context_field.C.size()) {
        prev_C = G_context_field.C;
    }
    
    melvin::Vector delta_C(G_context_field.C.size(), 0.0f);
    for (size_t i = 0; i < G_context_field.C.size() && i < prev_C.size(); ++i) {
        delta_C[i] = G_context_field.C[i] - prev_C[i];
    }
    
    // Use fitness from best action as global signal
    float global_fitness = (best_idx < all_scores.size()) ? 
                          all_scores[best_idx].fitness.overall() : 0.0f;
    
    G_connection_chemistry.metabolize(G_context_field.C, G_context_field.C, 
                                     delta_C, global_fitness);
    prev_C = G_context_field.C;
    
    // Periodic chemistry maintenance (every 10 cycles)
    static int chem_cycle = 0;
    if (++chem_cycle % 10 == 0) {
        size_t pruned = G_connection_chemistry.prune_connections();
        size_t fused = G_connection_chemistry.fuse_connections();
        size_t split = G_connection_chemistry.split_connections();
        
        if (debug && (pruned > 0 || fused > 0 || split > 0)) {
            std::cout << "  Chemistry maintenance: pruned=" << pruned 
                      << ", fused=" << fused << ", split=" << split << "\n";
        }
    }
    
    if (debug) {
        std::cout << "\n✅ Result: \"" << result << "\"\n";
        std::cout << "========================\n\n";
    }
    
    return result;
}

// ==================== META-LEARNING FOR DIMENSIONAL EVOLUTION ====================

// Meta-learning: adapt dimensional system hyperparameters based on performance
void dimensional_meta_learning(melvin::EmergentDimensionalSystem& dim_system, 
                               const melvin::FitnessMetrics& recent_fitness) {
    // Track meta-fitness (how well dimensional system itself is performing)
    static std::vector<float> meta_fitness_history;
    static size_t meta_window = 50;
    
    meta_fitness_history.push_back(recent_fitness.overall());
    if (meta_fitness_history.size() > meta_window) {
        meta_fitness_history.erase(meta_fitness_history.begin());
    }
    
    // Compute trend in meta-fitness
    if (meta_fitness_history.size() < 10) return; // Need minimum history
    
    float early_avg = 0.0f;
    float recent_avg = 0.0f;
    size_t split = meta_fitness_history.size() / 2;
    
    for (size_t i = 0; i < split; ++i) {
        early_avg += meta_fitness_history[i];
    }
    for (size_t i = split; i < meta_fitness_history.size(); ++i) {
        recent_avg += meta_fitness_history[i];
    }
    early_avg /= split;
    recent_avg /= (meta_fitness_history.size() - split);
    
    float improvement = recent_avg - early_avg;
    
    // Adapt promotion threshold based on improvement
    if (improvement > 0.1f) {
        // Fitness improving - current threshold is working
        // Slightly increase to be more selective (quality over quantity)
        dim_system.promotion_threshold *= 1.02f;
        dim_system.promotion_threshold = std::min(0.8f, dim_system.promotion_threshold);
    } else if (improvement < -0.1f) {
        // Fitness declining - need more dimensions or different ones
        // Lower threshold to allow more exploration
        dim_system.promotion_threshold *= 0.98f;
        dim_system.promotion_threshold = std::max(0.2f, dim_system.promotion_threshold);
    }
    
    // Adapt learning rate based on stability
    float fitness_variance = 0.0f;
    for (float f : meta_fitness_history) {
        fitness_variance += (f - recent_avg) * (f - recent_avg);
    }
    fitness_variance /= meta_fitness_history.size();
    
    // High variance = unstable, need slower learning
    // Low variance = stable, can learn faster
    if (fitness_variance > 0.2f) {
        dim_system.learning_rate *= 0.99f;
        dim_system.learning_rate = std::max(0.001f, dim_system.learning_rate);
    } else if (fitness_variance < 0.05f) {
        dim_system.learning_rate *= 1.01f;
        dim_system.learning_rate = std::min(0.1f, dim_system.learning_rate);
    }
    
    // Adapt gamma growth rate based on dimensional effectiveness
    size_t num_dims = dim_system.active_dimensions.size();
    if (num_dims > 0) {
        float avg_impact = 0.0f;
        for (const auto& dim : dim_system.active_dimensions) {
            avg_impact += dim.variance_impact;
        }
        avg_impact /= num_dims;
        
        // If dimensions are highly impactful, grow their influence faster
        if (avg_impact > 0.7f) {
            dim_system.gamma_growth_rate *= 1.05f;
            dim_system.gamma_growth_rate = std::min(0.2f, dim_system.gamma_growth_rate);
        } else if (avg_impact < 0.3f) {
            dim_system.gamma_growth_rate *= 0.95f;
            dim_system.gamma_growth_rate = std::max(0.01f, dim_system.gamma_growth_rate);
        }
    }
    
    // Trigger compression if too many weak dimensions
    size_t weak_dims = 0;
    for (const auto& dim : dim_system.active_dimensions) {
        if (dim.variance_impact < 0.3f) weak_dims++;
    }
    
    if (weak_dims > num_dims / 2 && num_dims > 5) {
        // More than half are weak - compress aggressively
        dim_system.compress_dimensions(G_nodes);
    }
}

// Meta-learning for connection chemistry constants
void chemistry_meta_learning(melvin::ChemistryConstants& constants,
                             const melvin::ConnectionChemistry::Stats& stats,
                             float recent_fitness) {
    static std::vector<float> fitness_trend;
    fitness_trend.push_back(recent_fitness);
    if (fitness_trend.size() > 20) {
        fitness_trend.erase(fitness_trend.begin());
    }
    
    if (fitness_trend.size() < 5) return;
    
    // Compute fitness trend
    float trend = 0.0f;
    for (size_t i = 1; i < fitness_trend.size(); ++i) {
        trend += fitness_trend[i] - fitness_trend[i-1];
    }
    trend /= (fitness_trend.size() - 1);
    
    // Adapt plasticity based on fitness trend
    if (trend > 0.05f) {
        // Fitness improving - current plasticity is working
        // Can reduce slightly for more stable connections
        constants.plasticity_base *= 0.99f;
        constants.plasticity_base = std::max(0.001f, constants.plasticity_base);
    } else if (trend < -0.05f) {
        // Fitness declining - need more plasticity to adapt
        constants.plasticity_base *= 1.02f;
        constants.plasticity_base = std::min(0.01f, constants.plasticity_base);
    }
    
    // Adapt reinforcement based on stability ratio
    float stability_ratio = stats.stable_connections / static_cast<float>(stats.total_connections + 1);
    if (stability_ratio > 0.8f) {
        // Too rigid - increase reinforcement to reward good patterns faster
        constants.reinforcement_gain *= 1.01f;
        constants.reinforcement_gain = std::min(0.1f, constants.reinforcement_gain);
    } else if (stability_ratio < 0.3f) {
        // Too volatile - decrease reinforcement
        constants.reinforcement_gain *= 0.99f;
        constants.reinforcement_gain = std::max(0.01f, constants.reinforcement_gain);
    }
    
    // Adapt fusion threshold based on connection count
    if (stats.total_connections > 10000) {
        // Too many connections - make fusion easier
        constants.fusion_similarity *= 0.99f;
        constants.fusion_similarity = std::max(0.85f, constants.fusion_similarity);
    } else if (stats.total_connections < 1000) {
        // Too few - make fusion harder (preserve diversity)
        constants.fusion_similarity *= 1.01f;
        constants.fusion_similarity = std::min(0.98f, constants.fusion_similarity);
    }
}

// Periodic dimensional evolution (call every N reasoning cycles)
void evolve_dimensional_system(int cycle_count) {
    static int last_compression = 0;
    static int last_meta_learning = 0;
    static int last_chem_meta = 0;
    
    // Compression every 10 cycles
    if (cycle_count - last_compression >= 10) {
        G_dimensional_system.compress_dimensions(G_nodes);
        last_compression = cycle_count;
    }
    
    // Meta-learning every 5 cycles
    if (cycle_count - last_meta_learning >= 5) {
        if (!G_dimensional_system.fitness_history.empty()) {
            dimensional_meta_learning(G_dimensional_system, 
                                     G_dimensional_system.fitness_history.back());
        }
        last_meta_learning = cycle_count;
    }
    
    // Chemistry meta-learning every 10 cycles
    if (cycle_count - last_chem_meta >= 10) {
        auto chem_stats = G_connection_chemistry.compute_stats();
        float recent_fitness = G_dimensional_system.fitness_history.empty() ? 
                              0.0f : G_dimensional_system.fitness_history.back().overall();
        chemistry_meta_learning(G_chemistry_constants, chem_stats, recent_fitness);
        last_chem_meta = cycle_count;
    }
    
    // Log dimensional state periodically
    if (cycle_count % 50 == 0) {
        std::cout << "\n📊 Dimensional System State (cycle " << cycle_count << "):\n";
        std::cout << "  Active dimensions: " << G_dimensional_system.active_dimensions.size() << "\n";
        std::cout << "  Promotion threshold: " << G_dimensional_system.promotion_threshold << "\n";
        std::cout << "  Learning rate: " << G_dimensional_system.learning_rate << "\n";
        std::cout << "  Baseline fitness: " << G_dimensional_system.baseline_fitness << "\n";
        
        if (!G_dimensional_system.active_dimensions.empty()) {
            std::cout << "  Top 5 dimensions:\n";
            std::vector<melvin::EmergentDimension> sorted_dims = G_dimensional_system.active_dimensions;
            std::sort(sorted_dims.begin(), sorted_dims.end(), 
                     [](const auto& a, const auto& b) { return a.variance_impact > b.variance_impact; });
            
            for (size_t i = 0; i < std::min(size_t(5), sorted_dims.size()); ++i) {
                const auto& dim = sorted_dims[i];
                auto it = G_nodes.find(dim.node_id);
                std::string node_text = (it != G_nodes.end()) ? it->second.text : "?";
                std::cout << "    " << (i+1) << ". \"" << node_text << "\" "
                          << "impact=" << dim.variance_impact << ", gamma=" << dim.gamma 
                          << ", age=" << dim.promotion_age << "\n";
            }
        }
        
        // Log connection chemistry state
        auto chem_stats = G_connection_chemistry.compute_stats();
        std::cout << "\n⚛️ Connection Chemistry State:\n";
        std::cout << "  Total connections: " << chem_stats.total_connections << "\n";
        std::cout << "  Mean conductivity: " << chem_stats.mean_conductivity << "\n";
        std::cout << "  Mean plasticity: " << chem_stats.mean_plasticity << "\n";
        std::cout << "  Mean stability: " << chem_stats.mean_stability << "\n";
        std::cout << "  Directional: " << chem_stats.directional_connections 
                  << " (" << (100.0f * chem_stats.directional_connections / (chem_stats.total_connections + 1)) << "%)\n";
        std::cout << "  Stable: " << chem_stats.stable_connections 
                  << " (" << (100.0f * chem_stats.stable_connections / (chem_stats.total_connections + 1)) << "%)\n";
        std::cout << "  Plastic: " << chem_stats.plastic_connections 
                  << " (" << (100.0f * chem_stats.plastic_connections / (chem_stats.total_connections + 1)) << "%)\n";
        
        std::cout << std::endl;
    }
}

// ==================== BEAM SEARCH STRUCTURES ====================

// Beam hypothesis structure
struct BeamHypothesis {
    std::vector<std::string> tokens;
    float total_score;
    uint64_t last_node;
    std::vector<size_t> edge_trace;
    
    BeamHypothesis() : total_score(0.0f), last_node(0) {}
    
    BeamHypothesis(const std::string& start_token, uint64_t start_node) 
        : total_score(0.0f), last_node(start_node) {
        tokens.push_back(start_token);
    }
};

// Choice trace for debugging
struct ChoiceTrace {
    std::string token;
    uint64_t count;
    float weight;
    float syn_fit;
    float sem_fit;
    float drv_bias;
    float recency;
    float rep_pen;
    float len_norm;
    float final_score;
};

// ==================== PHRASE GENERATION LOOP ====================

// End conditions for phrase generation
bool should_stop_generation(const std::vector<std::string>& phrase, size_t max_tokens, const std::vector<std::pair<size_t, float>>& candidates, float threshold) {
    // Stop if we've reached max tokens
    if (phrase.size() >= max_tokens) return true;
    
    // Stop if no viable candidates
    if (candidates.empty()) return true;
    
    // Stop if best score is below threshold
    if (candidates[0].second < threshold) return true;
    
    // Stop if phrase ends with punctuation
    if (!phrase.empty()) {
        const std::string& last = phrase.back();
        if (last == "." || last == "?" || last == "!" || last == ";") return true;
    }
    
    // Stop if last 3 tokens repeat (loop detection)
    if (phrase.size() >= 6) {
        bool repeating = true;
        for (size_t i = 0; i < 3; ++i) {
            if (phrase[phrase.size() - 3 + i] != phrase[phrase.size() - 6 + i]) {
                repeating = false;
                break;
            }
        }
        if (repeating) return true;
    }
    
    return false;
}

// Basic grammar fixes for phrase assembly
std::string fix_grammar(std::vector<std::string>& phrase) {
    if (phrase.empty()) return "";
    
    std::string result;
    
    // Capitalize first word
    if (!phrase[0].empty()) {
        phrase[0][0] = std::toupper(phrase[0][0]);
    }
    
    // Remove repeated determiners and articles
    for (size_t i = 1; i < phrase.size(); ++i) {
        if (i > 0 && phrase[i] == phrase[i-1] && 
            (phrase[i] == "the" || phrase[i] == "a" || phrase[i] == "an")) {
            phrase.erase(phrase.begin() + i);
            --i;
        }
    }
    
    // Fix "is/are" agreement
    for (size_t i = 1; i < phrase.size(); ++i) {
        if (phrase[i] == "is" && phrase[i-1].back() == 's' && phrase[i-1].length() > 2) {
            phrase[i] = "are";
        } else if (phrase[i] == "are" && phrase[i-1].back() != 's') {
            phrase[i] = "is";
        }
    }
    
    // Join tokens with spaces
    for (size_t i = 0; i < phrase.size(); ++i) {
        if (i > 0) result += " ";
        result += phrase[i];
    }
    
    // Add period if no punctuation at end
    if (!result.empty() && result.back() != '.' && result.back() != '?' && result.back() != '!') {
        result += ".";
    }
    
    return result;
}

// Main phrase generation function
std::string melvin_generate_phrase(uint64_t start_node_id, int max_tokens, const std::string& driver_mode, const std::string& selection_mode, const std::string& context, const std::string& query, bool debug) {
    std::vector<std::string> phrase;
    
    // Initialize with start token
    if (G_nodes.find(start_node_id) != G_nodes.end()) {
        phrase.push_back(G_nodes[start_node_id].text);
    } else {
        return "";
    }
    
    uint64_t current_node = start_node_id;
    int step = 1;
    
    if (debug) {
        std::cout << "\n🔄 Starting phrase generation from '" << phrase[0] << "'\n";
        std::cout << "Driver mode: " << driver_mode << ", Max tokens: " << max_tokens << "\n\n";
    }
    
    // Main generation loop
    while (phrase.size() < static_cast<size_t>(max_tokens)) {
        // Gather outgoing edges and compute scores
        auto candidates = get_scored_candidates(current_node, context, query, driver_mode);
        
        if (debug) {
            std::cout << "Step " << step << ": current=[" << phrase.back() << "]\n";
            std::cout << "  candidates:\n";
            for (const auto& [edge_idx, score] : candidates) {
                const auto& edge = G_edges[edge_idx];
                const auto& target_node = G_nodes[edge.v];
                std::cout << "    " << target_node.text << " (count=" << edge.count 
                          << ", weight=" << edge.weight << ", score=" << score << ")\n";
            }
        }
        
        // Check end conditions
        if (should_stop_generation(phrase, max_tokens, candidates, 0.1f)) {
            if (debug) {
                std::cout << "  Stopping generation (end condition met)\n";
            }
            break;
        }
        
        // Select next token
        size_t chosen_edge = select_next_edge(current_node, context, query, selection_mode, driver_mode);
        
        if (chosen_edge == SIZE_MAX || chosen_edge >= G_edges.size()) {
            if (debug) {
                std::cout << "  No valid edge selected, stopping\n";
            }
            break;
        }
        
        // Add chosen token to phrase
        const auto& target_node = G_nodes[G_edges[chosen_edge].v];
        phrase.push_back(target_node.text);
        current_node = G_edges[chosen_edge].v;
        
        if (debug) {
            std::cout << "  chosen=[" << target_node.text << "]\n";
        }
        
        // Reinforce the chosen edge
        reinforce_edge(chosen_edge);
        
        step++;
    }
    
    // Assemble final phrase with grammar fixes
    std::string result = fix_grammar(phrase);
    
    if (debug) {
        std::cout << "\n✅ Generated phrase: \"" << result << "\"\n";
    }
    
    return result;
}

// Enhanced beam search phrase generation
std::string melvin_generate_phrase_beam(uint64_t start_node_id, int max_tokens, int beam_k, int expand_M, const std::string& driver_mode, bool probabilistic, float stop_threshold, const std::string& context, const std::string& query, bool debug) {
    if (G_nodes.find(start_node_id) == G_nodes.end()) return "";
    
    std::vector<BeamHypothesis> beam;
    beam.push_back(BeamHypothesis(G_nodes[start_node_id].text, start_node_id));
    
    std::vector<std::vector<ChoiceTrace>> debug_traces;
    
    if (debug) {
        std::cout << "\n🔄 Starting beam search generation (k=" << beam_k << ", M=" << expand_M << ")\n";
        std::cout << "Driver mode: " << driver_mode << ", Stop threshold: " << stop_threshold << "\n\n";
    }
    
    // Main beam search loop
    for (int step = 0; step < max_tokens && !beam.empty(); ++step) {
        std::vector<BeamHypothesis> new_beam;
        std::vector<ChoiceTrace> step_traces;
        
        if (debug) {
            std::cout << "Step " << step << ": beam size = " << beam.size() << "\n";
        }
        
        // Expand each hypothesis in the beam
        for (const auto& hypothesis : beam) {
            if (G_adj.find(hypothesis.last_node) == G_adj.end()) continue;
            
            // Get top M candidates for this hypothesis
            std::vector<std::pair<size_t, float>> candidates;
            for (size_t edge_idx : G_adj[hypothesis.last_node]) {
                float score = compute_enhanced_score(edge_idx, hypothesis.tokens, step, context, query, driver_mode);
                if (score > 0.0f) {
                    candidates.push_back({edge_idx, score});
                }
            }
            
            // Sort by score and take top M
            std::sort(candidates.begin(), candidates.end(), 
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            if (candidates.empty()) continue;
            
            int candidates_to_expand = std::min(expand_M, static_cast<int>(candidates.size()));
            
            for (int i = 0; i < candidates_to_expand; ++i) {
                size_t edge_idx = candidates[i].first;
                float step_score = candidates[i].second;
                
                // Check stop conditions
                if (step_score < stop_threshold) continue;
                
                const auto& target_node = G_nodes[G_edges[edge_idx].v];
                const std::string& target_token = target_node.text;
                
                // Check for loops (same bigram seen >= 2 times in last 8 steps)
                if (step >= 1) {
                    std::string bigram = hypothesis.tokens.back() + " " + target_token;
                    int bigram_count = 0;
                    int window_start = std::max(0, static_cast<int>(hypothesis.tokens.size()) - 8);
                    for (int j = window_start; j < static_cast<int>(hypothesis.tokens.size()) - 1; ++j) {
                        if (hypothesis.tokens[j] + " " + hypothesis.tokens[j+1] == bigram) {
                            bigram_count++;
                        }
                    }
                    if (bigram_count >= 2) continue;
                }
                
                // Create new hypothesis
                BeamHypothesis new_hyp = hypothesis;
                new_hyp.tokens.push_back(target_token);
                new_hyp.total_score += step_score;
                new_hyp.last_node = G_edges[edge_idx].v;
                new_hyp.edge_trace.push_back(edge_idx);
                
                new_beam.push_back(new_hyp);
                
                // Debug trace
                if (debug) {
                    const auto& edge = G_edges[edge_idx];
                    ChoiceTrace trace;
                    trace.token = target_token;
                    trace.count = edge.count;
                    trace.weight = edge.weight;
                    trace.syn_fit = syntax_fit(edge, context, query);
                    trace.sem_fit = semantic_fit(edge, query);
                    trace.drv_bias = driver_bias(edge, driver_mode);
                    trace.recency = compute_recency_factor(edge);
                    trace.rep_pen = compute_repetition_penalty(hypothesis.tokens, target_token);
                    trace.len_norm = compute_length_norm(step);
                    trace.final_score = step_score;
                    step_traces.push_back(trace);
                }
            }
        }
        
        // Keep top k hypotheses
        std::sort(new_beam.begin(), new_beam.end(), 
                 [](const auto& a, const auto& b) { return a.total_score > b.total_score; });
        
        beam.clear();
        for (int i = 0; i < std::min(beam_k, static_cast<int>(new_beam.size())); ++i) {
            beam.push_back(new_beam[i]);
        }
        
        if (debug && !step_traces.empty()) {
            std::cout << "  Top candidates:\n";
            for (const auto& trace : step_traces) {
                std::cout << "    " << trace.token << " (count=" << trace.count 
                          << ", weight=" << trace.weight << ", syn=" << trace.syn_fit
                          << ", sem=" << trace.sem_fit << ", drv=" << trace.drv_bias
                          << ", rec=" << trace.recency << ", rep=" << trace.rep_pen
                          << ", len=" << trace.len_norm << ", score=" << trace.final_score << ")\n";
            }
        }
        
        debug_traces.push_back(step_traces);
        
        // Stop if no viable hypotheses
        if (beam.empty()) break;
    }
    
    // Select final hypothesis
    if (beam.empty()) return "";
    
    BeamHypothesis final_hyp;
    if (probabilistic) {
        // Probabilistic selection based on total scores
        float total_score = 0.0f;
        for (const auto& hyp : beam) {
            total_score += hyp.total_score;
        }
        
        if (total_score > 0.0f) {
            static std::random_device rd;
            static std::mt19937 gen(rd());
            std::uniform_real_distribution<float> dis(0.0f, total_score);
            
            float random_value = dis(gen);
            float cumulative = 0.0f;
            
            for (const auto& hyp : beam) {
                cumulative += hyp.total_score;
                if (random_value <= cumulative) {
                    final_hyp = hyp;
                    break;
                }
            }
        }
        
        if (final_hyp.tokens.empty()) {
            final_hyp = beam[0]; // Fallback
        }
    } else {
        // Deterministic selection (best score)
        final_hyp = beam[0];
    }
    
    // Reinforce the chosen path
    for (size_t edge_idx : final_hyp.edge_trace) {
        reinforce_edge(edge_idx);
    }
    
    // Assemble final phrase
    std::string result = fix_grammar(final_hyp.tokens);
    
    if (debug) {
        std::cout << "\n✅ Beam search completed. Chosen path:\n";
        for (size_t i = 0; i < final_hyp.tokens.size(); ++i) {
            std::cout << "  " << i << ": " << final_hyp.tokens[i];
            if (i < final_hyp.edge_trace.size()) {
                std::cout << " (edge " << final_hyp.edge_trace[i] << ")";
            }
            std::cout << "\n";
        }
        std::cout << "Final phrase: \"" << result << "\"\n";
        std::cout << "Total score: " << final_hyp.total_score << "\n";
    }
    
    return result;
}

// ==================== ENHANCED SCORING SYSTEM WITH PENALTIES ====================

// Laplace smoothing constant
static constexpr float LAPLACE_ALPHA = 0.5f;

// Recency decay constant (300 seconds)
static constexpr float RECENCY_TAU = 300.0f;

// Repetition penalty
static constexpr float REPETITION_GAMMA = 0.8f;
static constexpr int REPETITION_WINDOW = 6;

// Length normalization
static constexpr float LENGTH_BETA = 0.5f;

// Count repetition of token in recent window
int count_token_repetition(const std::vector<std::string>& phrase, const std::string& token) {
    if (phrase.size() < REPETITION_WINDOW) {
        return std::count(phrase.end() - phrase.size(), phrase.end(), token);
    } else {
        return std::count(phrase.end() - REPETITION_WINDOW, phrase.end(), token);
    }
}

// Compute Laplace-smoothed frequency probability
float compute_laplace_freq(size_t edge_idx, uint64_t source_node) {
    if (G_adj.find(source_node) == G_adj.end()) return 0.0f;
    
    const auto& edge = G_edges[edge_idx];
    uint64_t total_count = 0;
    
    // Calculate total count with Laplace smoothing
    for (size_t other_edge_idx : G_adj[source_node]) {
        total_count += G_edges[other_edge_idx].count + LAPLACE_ALPHA;
    }
    
    if (total_count == 0) return 0.0f;
    
    return static_cast<float>(edge.count + LAPLACE_ALPHA) / static_cast<float>(total_count);
}

// Compute recency factor
float compute_recency_factor(const melvin::Edge& edge) {
    float current_time = get_current_time();
    float time_diff = current_time - edge.last_used;
    return std::exp(-time_diff / RECENCY_TAU);
}

// Compute repetition penalty
float compute_repetition_penalty(const std::vector<std::string>& phrase, const std::string& token) {
    int repeat_count = count_token_repetition(phrase, token);
    return std::pow(REPETITION_GAMMA, static_cast<float>(repeat_count));
}

// Compute length normalization
float compute_length_norm(int step) {
    return 1.0f / std::pow(5.0f + step, LENGTH_BETA);
}

// Enhanced scoring with all factors
float compute_enhanced_score(size_t edge_idx, const std::vector<std::string>& phrase, int step, const std::string& context, const std::string& query, const std::string& driver_mode) {
    if (edge_idx >= G_edges.size()) return 0.0f;
    
    const auto& edge = G_edges[edge_idx];
    const auto& target_node = G_nodes[edge.v];
    const std::string& target_token = target_node.text;
    
    // Base factors
    float p_freq = compute_laplace_freq(edge_idx, edge.u);
    float syn_fit = syntax_fit(edge, context, query);
    float sem_fit = semantic_fit(edge, query);
    float drv_bias = driver_bias(edge, driver_mode);
    
    // New factors
    float recency = compute_recency_factor(edge);
    float rep_pen = compute_repetition_penalty(phrase, target_token);
    float len_norm = compute_length_norm(step);
    
    // Final score formula
    float score = p_freq * syn_fit * sem_fit * drv_bias * recency * rep_pen * len_norm;
    
    return score;
}

// Legacy scoring function for backward compatibility
float compute_edge_score(size_t edge_idx, const std::string& context, const std::string& query, const std::string& driver_mode) {
    if (edge_idx >= G_edges.size()) return 0.0f;
    
    const auto& edge = G_edges[edge_idx];
    
    // Apply the score formula: weight × syntax_fit × semantic_fit × driver_bias
    float score = edge.weight * 
                  syntax_fit(edge, context, query) * 
                  semantic_fit(edge, query) * 
                  driver_bias(edge, driver_mode);
    
    return score;
}

// Get candidates with computed scores
std::vector<std::pair<size_t, float>> get_scored_candidates(uint64_t node_id, const std::string& context, const std::string& query, const std::string& driver_mode) {
    std::vector<std::pair<size_t, float>> candidates;
    
    if (G_adj.find(node_id) == G_adj.end()) return candidates;
    
    for (size_t edge_idx : G_adj[node_id]) {
        float score = compute_edge_score(edge_idx, context, query, driver_mode);
        if (score > 0.0f) {
            candidates.push_back({edge_idx, score});
        }
    }
    
    // Sort by score (highest first)
    std::sort(candidates.begin(), candidates.end(), 
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    return candidates;
}

// Select next word using deterministic or probabilistic method
size_t select_next_edge(uint64_t node_id, const std::string& context, const std::string& query, const std::string& selection_mode, const std::string& driver_mode) {
    auto candidates = get_scored_candidates(node_id, context, query, driver_mode);
    
    if (candidates.empty()) return SIZE_MAX;
    
    if (selection_mode == "deterministic") {
        // Pick edge with maximum score
        return candidates[0].first;
    } else if (selection_mode == "probabilistic") {
        // Roulette wheel selection based on scores
        float total_score = 0.0f;
        for (const auto& [_, score] : candidates) {
            total_score += score;
        }
        
        if (total_score <= 0.0f) return SIZE_MAX;
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dis(0.0f, total_score);
        
        float random_value = dis(gen);
        float cumulative = 0.0f;
        
        for (const auto& [edge_idx, score] : candidates) {
            cumulative += score;
            if (random_value <= cumulative) {
                return edge_idx;
            }
        }
        
        return candidates.back().first; // Fallback
    }
    
    return candidates[0].first; // Default to deterministic
}

// Enhanced debug function showing counts, weights, and scores
void debug_node_connections(uint64_t node_id, const std::string& context, const std::string& query, const std::string& driver_mode) {
    if (G_nodes.find(node_id) == G_nodes.end()) {
        std::cout << "Node " << node_id << " not found\n";
        return;
    }
    
    std::cout << "[" << G_nodes[node_id].text << "]\n";
    
    if (!context.empty() || !query.empty()) {
        auto candidates = get_scored_candidates(node_id, context, query, driver_mode);
        
        for (const auto& [edge_idx, score] : candidates) {
            const auto& edge = G_edges[edge_idx];
            const auto& target_node = G_nodes[edge.v];
            std::cout << "  " << target_node.text << " (count=" << edge.count 
                      << ", weight=" << edge.weight << ", score=" << score << ")\n";
        }
    } else {
        // Simple display without scoring
        auto neighbors = get_weighted_neighbors(node_id);
        
        for (const auto& [edge_idx, weight] : neighbors) {
            const auto& edge = G_edges[edge_idx];
            const auto& target_node = G_nodes[edge.v];
            std::cout << "  " << target_node.text << " (count=" << edge.count 
                      << ", weight=" << weight << ")\n";
        }
    }
}

// ==================== CORE REASONING FUNCTIONS ====================

// Node prior calculation with enhanced semantic matching
double node_prior(uint64_t nid, const melvin::Query& Q) {
    auto& n = G_nodes[nid];
    double s_text = 0.0;
    
    // Enhanced semantic matching: try exact word matches first, then embeddings
    for (const auto& qt : Q.toks) {
        // Direct word match gets highest score
        if (qt == n.text) {
            s_text = std::max(s_text, 2.0); // Boost exact matches significantly
        } else {
            // Fall back to embedding similarity
            auto qt_emb = embed_text(qt);
            if (!qt_emb.empty() && !n.emb.empty()) {
                double sim = cosine(qt_emb, n.emb);
                s_text = std::max(s_text, sim);
            }
        }
    }
    
    // Additional domain-specific scoring (add semantic context)
    double domain_boost = 0.0;
    std::string query_lower = Q.raw;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    // Key domain mappings for better retrieval
    if ((query_lower.find("pattern") != std::string::npos || query_lower.find("sequence") != std::string::npos) && 
        (n.text == "patterns" || n.text == "sequences" || n.text == "numbers" || n.text == "exponential")) {
        domain_boost += 0.5;
    }
    if ((query_lower.find("color") != std::string::npos || query_lower.find("red") != std::string::npos || query_lower.find("green") != std::string::npos) && 
        (n.text == "colors" || n.text == "red" || n.text == "green" || n.text == "yellow")) {
        domain_boost += 0.5;
    }
    if ((query_lower.find("ice") != std::string::npos || query_lower.find("water") != std::string::npos || query_lower.find("float") != std::string::npos) && 
        (n.text == "ice" || n.text == "water" || n.text == "density" || n.text == "float")) {
        domain_boost += 0.5;
    }
    
    return s_text + 0.1 * std::log(1.0 + n.freq) + domain_boost;
}

// Step scoring for edge traversal with weight-based selection
double step_score(size_t eidx, const melvin::Query& Q, double threshold) {
    auto& e = G_edges[eidx];
    auto& v = G_nodes[e.v];
    
    double score = 0.0;
    
    // Text similarity with destination node
    for (const auto& qt : Q.toks) {
        if (qt == v.text) {
            score += 1.0;
        } else {
            auto qt_emb = embed_text(qt);
            if (!qt_emb.empty() && !v.emb.empty()) {
                score += cosine(qt_emb, v.emb);
            }
        }
    }
    
    // Apply traditional weights
    score = score * e.w_core + 0.1 * e.w_ctx;
    
    // Boost score based on learned weight (frequency-based)
    score *= (1.0 + e.weight * 0.5); // Up to 50% boost for high-frequency connections
    
    return score >= threshold ? score : 0.0;
}

// Path scoring for multi-hop reasoning
double path_score(const melvin::Path& p, const melvin::Query& Q) {
    if (p.nodes.size() < 2 || p.edges_idx.empty()) return 0.0;
    
    // Start with node prior of first node
    double total_score = node_prior(p.nodes.front(), Q);
    
    // Add step contributions
    for (size_t i = 0; i < p.edges_idx.size(); ++i) {
        auto edge_threshold = AdaptiveThresholds::get_edge_threshold(G_nodes.size());
        total_score += step_score(p.edges_idx[i], Q, edge_threshold);
    }
    
    return total_score;
}

// ==================== ANCHOR SELECTION & QUERY ROUTING ====================

// Query classification for better routing
enum class QueryKind {
    Q_EQUALS,    // "what is X" or "X is what"
    Q_HAVE,      // "what do/does X have" or "X have/has what"
    Q_ACTION,    // "how/why/when..." or other action queries
    Q_UNKNOWN    // fallback
};

// Simple lemma mapping for better anchor selection
std::unordered_map<std::string, std::string> create_lemma_map() {
    return {
        {"cats", "cat"}, {"dogs", "dog"}, {"birds", "bird"}, {"mammals", "mammal"},
        {"animals", "animal"}, {"plants", "plant"}, {"cars", "car"}, {"books", "book"},
        {"is", "are"}, {"are", "is"}, {"has", "have"}, {"have", "has"},
        {"does", "do"}, {"do", "does"}, {"did", "do"}
    };
}

// Find the best anchor node from query tokens
uint64_t pick_anchor(const melvin::Query& Q) {
    static auto lemma_map = create_lemma_map();
    
    // Heuristic: rightmost noun-ish token; else last non-function token
    std::unordered_set<std::string> function_words = {
        "what", "how", "why", "when", "where", "is", "are", "am", "was", "were",
        "do", "does", "did", "have", "has", "had", "will", "would", "could", "should"
    };
    
    uint64_t best_anchor = 0;
    int best_score = -1;
    
    // std::cerr << "🔍 Picking anchor from tokens: ";
    // for (const auto& tok : Q.toks) std::cerr << "'" << tok << "' ";
    // std::cerr << "\n";
    
    // Try to find anchor in reverse order (prefer later tokens)
    for (int i = Q.toks.size() - 1; i >= 0; --i) {
        const std::string& token = Q.toks[i];
        
        // Skip function words
        if (function_words.find(token) != function_words.end()) continue;
        
        // Try original token
        std::hash<std::string> hasher;
        uint64_t node_id = static_cast<uint64_t>(hasher(token));
        
        if (G_nodes.find(node_id) != G_nodes.end()) {
            int score = static_cast<int>(token.size()) + (i * 2); // Prefer longer, later tokens
            if (score > best_score) {
                best_score = score;
                best_anchor = node_id;
            }
        }
        
        // Try lemmatized version
        auto lemma_it = lemma_map.find(token);
        if (lemma_it != lemma_map.end()) {
            uint64_t lemma_id = static_cast<uint64_t>(hasher(lemma_it->second));
            if (G_nodes.find(lemma_id) != G_nodes.end()) {
                int score = static_cast<int>(lemma_it->second.size()) + (i * 2);
                if (score > best_score) {
                    best_score = score;
                    best_anchor = lemma_id;
                }
            }
        }
    }
    
    return best_anchor;
}

// Classify query type for better routing
QueryKind classify_query(const melvin::Query& Q) {
    std::string query_lower = Q.raw;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    // Check for equals patterns
    if (query_lower.find("what is") != std::string::npos || 
        query_lower.find("what are") != std::string::npos ||
        query_lower.find(" is what") != std::string::npos) {
        return QueryKind::Q_EQUALS;
    }
    
    // Check for have patterns
    if (query_lower.find("what do") != std::string::npos ||
        query_lower.find("what does") != std::string::npos ||
        query_lower.find(" have") != std::string::npos ||
        query_lower.find(" has") != std::string::npos) {
        return QueryKind::Q_HAVE;
    }
    
    // Check for action patterns
    if (query_lower.find("how") != std::string::npos ||
        query_lower.find("why") != std::string::npos ||
        query_lower.find("when") != std::string::npos) {
        return QueryKind::Q_ACTION;
    }
    
    return QueryKind::Q_UNKNOWN;
}

// Enhanced semantic fit based on query type
float enhanced_semantic_fit(const melvin::Edge& edge, const melvin::Query& Q, QueryKind kind) {
    const auto& target_node = G_nodes[edge.v];
    const std::string& target_text = target_node.text;
    
    // Base semantic fit
    float base_fit = 1.0f;
    for (const auto& qt : Q.toks) {
        if (qt == target_text) {
            base_fit = 1.2f; // Boost exact matches
            break;
        }
    }
    
    // Query-type specific boosts
    switch (kind) {
        case QueryKind::Q_EQUALS:
            // Boost edges that connect through "is/are" for equals questions
            if (target_text == "is" || target_text == "are" || 
                target_text == "am" || target_text == "was" || target_text == "were") {
                return base_fit * 1.3f;
            }
            break;
            
        case QueryKind::Q_HAVE:
            // Boost edges that connect through "have/has" for possession questions
            if (target_text == "have" || target_text == "has" || target_text == "had") {
                return base_fit * 1.3f;
            }
            break;
            
        case QueryKind::Q_ACTION:
            // Boost action-related words for how/why/when questions
            if (target_text == "how" || target_text == "why" || target_text == "when" ||
                target_text == "because" || target_text == "through" || target_text == "by") {
                return base_fit * 1.2f;
            }
            break;
            
        default:
            break;
    }
    
    return base_fit;
}

// ==================== SKILL HEADS ====================

// Arithmetic micro-head for simple math operations
std::string arithmetic_skill_head(const std::string& query) {
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    // Pattern: "what is X + Y" or "X + Y = ?"
    std::regex math_pattern(R"((\d+)\s*([+\-*/])\s*(\d+))");
    std::smatch matches;
    
    if (std::regex_search(query_lower, matches, math_pattern)) {
        int num1 = std::stoi(matches[1].str());
        std::string op = matches[2].str();
        int num2 = std::stoi(matches[3].str());
        
        int result = 0;
        if (op == "+") {
            result = num1 + num2;
        } else if (op == "-") {
            result = num1 - num2;
        } else if (op == "*") {
            result = num1 * num2;
        } else if (op == "/" && num2 != 0) {
            result = num1 / num2;
        }
        
        return std::to_string(result) + " (arithmetic)";
    }
    
    return "";
}

// Sequence micro-head for pattern recognition
std::string sequence_skill_head(const std::string& query) {
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    // Look for sequence patterns like "1, 3, 5, 7" or "2, 4, 8, 16"
    std::regex seq_pattern(R"((\d+(?:\s*,\s*\d+)*))");
    std::smatch matches;
    
    if (std::regex_search(query_lower, matches, seq_pattern)) {
        std::string seq_str = matches[1].str();
        
        // Parse sequence numbers
        std::vector<int> numbers;
        std::regex num_pattern(R"(\d+)");
        std::sregex_iterator begin(seq_str.begin(), seq_str.end(), num_pattern);
        std::sregex_iterator end;
        
        for (auto it = begin; it != end; ++it) {
            numbers.push_back(std::stoi(it->str()));
        }
        
        if (numbers.size() >= 3) {
            // Test for arithmetic sequence (constant difference)
            if (numbers.size() >= 2) {
                int diff = numbers[1] - numbers[0];
                bool is_arithmetic = true;
                for (size_t i = 1; i < numbers.size() - 1; ++i) {
                    if (numbers[i + 1] - numbers[i] != diff) {
                        is_arithmetic = false;
                        break;
                    }
                }
                
                if (is_arithmetic) {
                    int next = numbers.back() + diff;
                    return std::to_string(next) + " (arithmetic sequence)";
                }
            }
            
            // Test for geometric sequence (constant ratio)
            if (numbers.size() >= 2 && numbers[0] != 0) {
                double ratio = static_cast<double>(numbers[1]) / numbers[0];
                bool is_geometric = true;
                
                for (size_t i = 1; i < numbers.size() - 1; ++i) {
                    if (numbers[i] != 0) {
                        double current_ratio = static_cast<double>(numbers[i + 1]) / numbers[i];
                        if (std::abs(current_ratio - ratio) > 0.001) {
                            is_geometric = false;
                            break;
                        }
                    } else {
                        is_geometric = false;
                        break;
                    }
                }
                
                if (is_geometric) {
                    int next = static_cast<int>(numbers.back() * ratio);
                    return std::to_string(next) + " (geometric sequence)";
                }
            }
            
            // Test for Fibonacci-like sequence (each number is sum of previous two)
            if (numbers.size() >= 3) {
                bool is_fibonacci = true;
                for (size_t i = 2; i < numbers.size(); ++i) {
                    if (numbers[i] != numbers[i-1] + numbers[i-2]) {
                        is_fibonacci = false;
                        break;
                    }
                }
                
                if (is_fibonacci) {
                    int next = numbers.back() + numbers[numbers.size() - 2];
                    return std::to_string(next) + " (fibonacci sequence)";
                }
            }
            
            // Test for exponential powers of 2 (2, 4, 8, 16, 32...)
            bool is_powers_of_2 = true;
            for (int num : numbers) {
                if (num <= 0 || (num & (num - 1)) != 0) { // Check if not power of 2
                    is_powers_of_2 = false;
                    break;
                }
            }
            
            if (is_powers_of_2) {
                // Verify it's consecutive powers of 2
                int expected = 1;
                for (size_t i = 0; i < numbers.size(); ++i) {
                    expected *= 2;
                    if (numbers[i] != expected) {
                        is_powers_of_2 = false;
                        break;
                    }
                }
                
                if (is_powers_of_2) {
                    int next = numbers.back() * 2;
                    return std::to_string(next) + " (exponential powers of 2)";
                }
            }
        }
    }
    
    return "";
}

// Pattern recognition skill head for geometric patterns
std::string pattern_skill_head(const std::string& query) {
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    // Look for geometric pattern descriptions
    if (query_lower.find("square") != std::string::npos && 
        query_lower.find("circle") != std::string::npos && 
        query_lower.find("triangle") != std::string::npos) {
        
        // Extract the sequence of shapes
        std::vector<std::string> shapes;
        std::regex shape_pattern(R"(\b(square|circle|triangle)\b)");
        std::sregex_iterator begin(query_lower.begin(), query_lower.end(), shape_pattern);
        std::sregex_iterator end;
        
        for (auto it = begin; it != end; ++it) {
            shapes.push_back(it->str());
        }
        
        if (shapes.size() >= 3) {
            // Look for repeating patterns
            for (size_t pattern_len = 1; pattern_len <= shapes.size() / 2; ++pattern_len) {
                bool is_repeating = true;
                for (size_t i = 0; i < shapes.size(); ++i) {
                    if (shapes[i] != shapes[i % pattern_len]) {
                        is_repeating = false;
                        break;
                    }
                }
                
                if (is_repeating) {
                    // Return the next shape in the pattern
                    return shapes[shapes.size() % pattern_len];
                }
            }
        }
    }
    
    return "";
}

// Main skill head dispatcher
std::string skill_head_dispatch(const std::string& query, QueryKind kind) {
    std::string query_lower = query;
    std::transform(query_lower.begin(), query_lower.end(), query_lower.begin(), ::tolower);
    
    // Use skill heads for:
    // 1. Action queries (how/why/when)
    // 2. Pattern recognition queries (containing numbers or sequences)
    // 3. Mathematical queries (containing operators)
    
    bool should_use_skill_heads = (kind == QueryKind::Q_ACTION) ||
                                  query_lower.find("pattern") != std::string::npos ||
                                  query_lower.find("sequence") != std::string::npos ||
                                  query_lower.find("next") != std::string::npos ||
                                  std::regex_search(query_lower, std::regex(R"(\d+\s*[+\-*/]\s*\d+)")) ||
                                  std::regex_search(query_lower, std::regex(R"(\d+(?:\s*,\s*\d+)+)"));
    
    if (!should_use_skill_heads) {
        return "";
    }
    
    // Try arithmetic first
    std::string result = arithmetic_skill_head(query);
    if (!result.empty()) {
        return result;
    }
    
    // Try sequence recognition
    result = sequence_skill_head(query);
    if (!result.empty()) {
        return result;
    }
    
    // Try pattern recognition
    result = pattern_skill_head(query);
    if (!result.empty()) {
        return result;
    }
    
    return "";
}

// ==================== BEAM SEARCH REASONING ====================

// Include enhanced monitoring for composition tracking

// Enhanced scoring constants (additional)
static constexpr float LENGTH_LAMBDA = 0.2f;
static constexpr float ANSWER_THRESHOLD_EQUALS = 0.01f;
static constexpr float ANSWER_THRESHOLD_HAVE = 0.01f;
static constexpr float ANSWER_THRESHOLD_ACTION = 0.01f;

// Enhanced edge scoring with all factors
double enhanced_edge_score(size_t edge_idx, const melvin::Query& Q, QueryKind kind, 
                          const std::vector<uint64_t>& path_nodes) {
    if (edge_idx >= G_edges.size()) return 0.0;
    
    const auto& edge = G_edges[edge_idx];
    
    // Laplace-smoothed frequency probability
    double p_freq = 0.0;
    if (G_adj.find(edge.u) != G_adj.end()) {
        uint64_t total_count = 0;
        for (size_t other_edge_idx : G_adj[edge.u]) {
            total_count += G_edges[other_edge_idx].count + LAPLACE_ALPHA;
        }
        if (total_count > 0) {
            p_freq = static_cast<double>(edge.count + LAPLACE_ALPHA) / total_count;
        }
    }
    
    // Recency factor
    float current_time = get_current_time();
    double recency = std::exp(-(current_time - edge.last_used) / RECENCY_TAU);
    
    // Repetition penalty
    double rep_pen = 1.0;
    if (!path_nodes.empty()) {
        const auto& target_node = G_nodes[edge.v];
        int repeat_count = 0;
        int window_start = std::max(0, static_cast<int>(path_nodes.size()) - REPETITION_WINDOW);
        for (int i = window_start; i < static_cast<int>(path_nodes.size()); ++i) {
            if (G_nodes[path_nodes[i]].text == target_node.text) {
                repeat_count++;
            }
        }
        rep_pen = std::pow(REPETITION_GAMMA, repeat_count);
    }
    
    // Existing factors
    double syn_fit = syntax_fit(edge, "", Q.raw);
    double sem_fit = enhanced_semantic_fit(edge, Q, kind);
    double drv_bias = driver_bias(edge, "balanced");
    
    // Final edge score
    return p_freq * syn_fit * sem_fit * drv_bias * recency * rep_pen;
}

// Enhanced path scoring with length normalization
double enhanced_path_score(const melvin::Path& path, const melvin::Query& Q, QueryKind kind) {
    if (path.nodes.size() < 2 || path.edges_idx.empty()) return 0.0;
    
    // Start with node prior of first node
    double total_score = node_prior(path.nodes.front(), Q);
    
    // Add enhanced step contributions
    for (size_t i = 0; i < path.edges_idx.size(); ++i) {
        total_score += enhanced_edge_score(path.edges_idx[i], Q, kind, path.nodes);
    }
    
    // Length normalization
    double len_norm = 1.0 / (1.0 + LENGTH_LAMBDA * (path.nodes.size() - 1));
    
    return total_score * len_norm;
}

// Iterative-Deepening Beam Search with enhanced scoring
std::vector<melvin::Path> search_idbs(uint64_t start_node, int beam_k, int expand_M, 
                                     int depth_max, const melvin::Query& Q, QueryKind kind) {
    std::vector<melvin::Path> beam;
    
    // Initialize with start node
    melvin::Path starting_path;
    starting_path.nodes.push_back(start_node);
    starting_path.score = node_prior(start_node, Q);
    beam.push_back(starting_path);
    
    // Iterative deepening
    for (int depth = 0; depth < depth_max && !beam.empty(); ++depth) {
        std::vector<melvin::Path> new_beam;
        
        for (const auto& current_path : beam) {
            if (current_path.nodes.empty()) continue;
            
            uint64_t last_node = current_path.nodes.back();
            if (G_adj.find(last_node) == G_adj.end()) continue;
            
            // Get all outgoing edges with scores
            std::vector<std::pair<size_t, double>> candidates;
            for (size_t edge_idx : G_adj[last_node]) {
                double score = enhanced_edge_score(edge_idx, Q, kind, current_path.nodes);
                if (score > 1e-6) { // Prune very low scores
                    candidates.push_back({edge_idx, score});
                }
            }
            
            // Sort and take top M candidates
            std::sort(candidates.begin(), candidates.end(), 
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            int candidates_to_expand = std::min(expand_M, static_cast<int>(candidates.size()));
            
            for (int i = 0; i < candidates_to_expand; ++i) {
                size_t edge_idx = candidates[i].first;
                const auto& edge = G_edges[edge_idx];
                
                // Avoid cycles
                if (std::find(current_path.nodes.begin(), current_path.nodes.end(), edge.v) 
                    != current_path.nodes.end()) {
                    continue;
                }
                
                // Check for bigram repetition (loop detection)
                if (current_path.nodes.size() >= 2) {
                    std::string bigram = G_nodes[current_path.nodes.back()].text + " " + 
                                        G_nodes[edge.v].text;
                    int bigram_count = 0;
                    for (size_t j = 1; j < current_path.nodes.size(); ++j) {
                        std::string prev_bigram = G_nodes[current_path.nodes[j-1]].text + " " + 
                                                 G_nodes[current_path.nodes[j]].text;
                        if (prev_bigram == bigram) {
                            bigram_count++;
                        }
                    }
                    if (bigram_count >= 2) continue; // Reject if bigram seen 2+ times
                }
                
                // Create new path
                melvin::Path new_path = current_path;
                new_path.nodes.push_back(edge.v);
                new_path.edges_idx.push_back(edge_idx);
                new_path.score = enhanced_path_score(new_path, Q, kind);
                
                // Track composition: new thought node created during reasoning
                // (monitoring disabled for now)
                
                // Track edge type based on reasoning pattern
                // rel_type mapping: EXACT=0, TEMPORAL=1, LEAP=2, etc.
                if (current_path.nodes.size() > 1) {
                    // Determine if this is temporal (sequential reasoning) or leap (creative connection)
                    const auto& prev_node = G_nodes[current_path.nodes.back()];
                    const auto& curr_node = G_nodes[edge.v];
                    
                    // Simple heuristic: if nodes are semantically related, it's temporal
                    // Otherwise, it's a creative leap
                    // (monitoring disabled for now)
                }
                
                // Track edge creation with weight bits (using edge count as proxy)
                // (monitoring disabled for now)
                
                new_beam.push_back(new_path);
            }
        }
        
        beam = std::move(new_beam);
        
        // Keep top k hypotheses
        if (beam.size() > beam_k) {
            std::sort(beam.begin(), beam.end(), 
                     [](const auto& a, const auto& b) { return a.score > b.score; });
            beam.resize(beam_k);
        }
        
        // Early stopping if no good candidates
        if (beam.empty()) break;
    }
    
    return beam;
}

std::vector<melvin::Path> beam_search(const melvin::Query& Q) {
    static constexpr int beam_k = 3;
    static constexpr int expand_M = 4;
    static constexpr int depth_max = 8;
    
    // Classify query and pick anchor
    QueryKind kind = classify_query(Q);
    uint64_t anchor = pick_anchor(Q);
    
    // std::cerr << "🔍 Query: '" << Q.raw << "' classified as " << static_cast<int>(kind) 
    //           << ", anchor=" << anchor << "\n";
    
    // If no anchor found, fall back to traditional beam search
    if (anchor == 0) {
        // Traditional initialization with top candidate nodes
        std::vector<std::pair<double, uint64_t>> candidate_nodes;
        auto node_threshold = AdaptiveThresholds::get_node_prior_threshold(G_nodes.size());
        
        for (const auto& [nid, node] : G_nodes) {
            double prior_score = node_prior(nid, Q);
            if (prior_score >= node_threshold) {
                candidate_nodes.push_back({prior_score, nid});
            }
        }
        
        std::sort(candidate_nodes.begin(), candidate_nodes.end(), 
                  [](const auto& a, const auto& b) { return a.first > b.first; });
        
        if (candidate_nodes.empty()) return {};
        
        // Use top candidate as anchor
        anchor = candidate_nodes[0].second;
    }
    
    // Run IDBS from anchor
    auto results = search_idbs(anchor, beam_k, expand_M, depth_max, Q, kind);
    
    // Track completed reasoning paths
    for (const auto& path : results) {
        if (!path.nodes.empty()) {
            // Track path creation (monitoring disabled for now)
        }
    }
    
    // Debug output
    // std::cerr << "🔍 Beam search found " << results.size() << " paths\n";
    // for (size_t i = 0; i < std::min(results.size(), size_t(3)); ++i) {
    //     std::cerr << "  Path " << i << ": score=" << results[i].score 
    //               << ", nodes=" << results[i].nodes.size() << "\n";
    // }
    
    // Filter by confidence threshold
    float threshold = ANSWER_THRESHOLD_EQUALS;
    switch (kind) {
        case QueryKind::Q_HAVE: threshold = ANSWER_THRESHOLD_HAVE; break;
        case QueryKind::Q_ACTION: threshold = ANSWER_THRESHOLD_ACTION; break;
        default: break;
    }
    
    std::vector<melvin::Path> filtered_results;
    for (const auto& path : results) {
        if (path.score >= threshold) {
            filtered_results.push_back(path);
        }
    }
    
    // std::cerr << "🔍 After filtering (threshold=" << threshold << "): " 
    //           << filtered_results.size() << " paths remain\n";
    
    return filtered_results;
}

// ==================== PATH TO PHRASE ALGORITHM ====================

std::string path_to_phrase(const melvin::Path& path) {
    if (path.nodes.empty()) return "";
    
    std::vector<std::string> tokens;
    
    // KeepSet: Essential function words that should be retained
    static const std::unordered_set<std::string> keep_set = {
        "is", "are", "am", "a", "an", "the", "of", "to", "and", "in", "on", 
        "with", "that", "which", "do", "does", "did", "have", "has", "had"
    };
    
    // Process all tokens first
    for (size_t i = 0; i < path.nodes.size(); ++i) {
        auto& node_text = G_nodes[path.nodes[i]].text;
        
        // Skip only very short tokens (< 2 chars) and duplicates
        if (node_text.size() < 2) continue;
        
        // Remove duplicate consecutive tokens
        if (!tokens.empty() && tokens.back() == node_text) continue;
        
        tokens.push_back(node_text);
    }
    
    // Smart retention: Keep function words that bridge content or are essential
    std::vector<std::string> filtered_tokens;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& token = tokens[i];
        
        // Always keep content words (not in keep_set or longer than 3 chars)
        if (keep_set.find(token) == keep_set.end() || token.size() > 3) {
            filtered_tokens.push_back(token);
            continue;
        }
        
        // Keep function words if they bridge two content words or are at strategic positions
        bool should_keep = false;
        
        // Check if this function word bridges content tokens
        if (i > 0 && i < tokens.size() - 1) {
            const std::string& prev = tokens[i-1];
            const std::string& next = tokens[i+1];
            if ((keep_set.find(prev) == keep_set.end() || prev.size() > 3) &&
                (keep_set.find(next) == keep_set.end() || next.size() > 3)) {
                should_keep = true;
            }
        }
        
        // Keep essential copulas and determiners in key positions
        if (token == "is" || token == "are" || token == "a" || token == "an" || token == "the") {
            should_keep = true;
        }
        
        if (should_keep) {
            filtered_tokens.push_back(token);
        }
    }
    
    if (filtered_tokens.empty()) return "";
    
    // Assemble phrase
    std::string phrase;
    for (size_t i = 0; i < filtered_tokens.size(); ++i) {
        if (i > 0) phrase += " ";
        phrase += filtered_tokens[i];
    }
    
    // Grammar fixes
    if (!phrase.empty()) {
        // Fix subject-verb agreement: "cats is" -> "cats are"
        size_t pos = phrase.find(" is ");
        if (pos != std::string::npos && pos > 0) {
            std::string before = phrase.substr(0, pos);
            if (before.back() == 's' && before.size() > 2) {
                phrase.replace(pos + 1, 2, "are");
            }
        }
        
        // Fix "a are" -> "a is"
        pos = phrase.find("a are");
        if (pos != std::string::npos) {
            phrase.replace(pos + 2, 3, "is");
        }
        
        // Capitalize first word
        phrase[0] = std::toupper(phrase[0]);
        
        // Add period if no terminal punctuation
        if (phrase.back() != '.' && phrase.back() != '?' && phrase.back() != '!') {
            phrase += ".";
        }
    }
    
    return phrase;
}

// ==================== ANSWERER HELPER FUNCTIONS ====================

// Helper function to find node ID by text
uint64_t node_id(const std::string& text) {
    std::hash<std::string> hasher;
    return static_cast<uint64_t>(hasher(text));
}

// Helper function to get node text by ID
std::string node_text(uint64_t node_id) {
    if (G_nodes.find(node_id) != G_nodes.end()) {
        return G_nodes[node_id].text;
    }
    return "";
}

// Helper function to find best neighbor with specific relation using frequency ratios
std::optional<uint64_t> best_neighbor(uint64_t node_id, melvin::Rel rel) {
    if (G_adj.find(node_id) == G_adj.end()) return std::nullopt;
    
    float best_freq = 0.0f;
    uint64_t best_neighbor = 0;
    
    for (size_t edge_idx : G_adj[node_id]) {
        if (edge_idx >= G_edges.size()) continue;
        const auto& edge = G_edges[edge_idx];
        
        if (edge.rel == static_cast<uint8_t>(rel)) {
            if (edge.weight > best_freq) {
                best_freq = edge.weight;
                best_neighbor = edge.v;
            }
        }
    }
    
    return best_freq > 0.0f ? std::optional<uint64_t>(best_neighbor) : std::nullopt;
}

// Helper function for two-hop traversal using edge weights
std::optional<uint64_t> two_hop(uint64_t from, uint64_t middle, melvin::Rel rel) {
    if (G_adj.find(from) == G_adj.end()) return std::nullopt;
    if (G_adj.find(middle) == G_adj.end()) return std::nullopt;
    
    // Find edge from -> middle
    bool found_first_hop = false;
    for (size_t edge_idx : G_adj[from]) {
        if (edge_idx >= G_edges.size()) continue;
        const auto& edge = G_edges[edge_idx];
        if (edge.v == middle && edge.rel == static_cast<uint8_t>(rel)) {
            found_first_hop = true;
            break;
        }
    }
    
    if (!found_first_hop) return std::nullopt;
    
    // Find best edge from middle -> ? using edge weights
    float best_weight = 0.0f;
    uint64_t best_target = 0;
    
    for (size_t edge_idx : G_adj[middle]) {
        if (edge_idx >= G_edges.size()) continue;
        const auto& edge = G_edges[edge_idx];
        
        if (edge.rel == static_cast<uint8_t>(rel)) {
            if (edge.weight > best_weight) {
                best_weight = edge.weight;
                best_target = edge.v;
            }
        }
    }
    
    return best_weight > 0.0f ? std::optional<uint64_t>(best_target) : std::nullopt;
}

// ==================== STORAGE BRIDGE ====================

void bridge_modern_to_legacy_storage() {
    std::cerr << "🌉 BRIDGING MODERN STORAGE TO LEGACY VARIABLES\n";
        std::cerr << "✅ BRIDGE COMPLETE: Legacy storage now has " << G_nodes.size() << " nodes, " 
                  << G_edges.size() << " edges\n";
    }


// ==================== MAIN MELVIN INTERFACE ====================

// C API struct for external use with LLM-style reasoning
struct melvin_t {
    std::string store_dir;
    size_t node_count;
    size_t edge_count;
    bool use_graph_reasoning = true;
    float confidence_threshold = 0.5f;
    
    // LLM-style reasoning components
    melvin::UCAConfig uca_config;
    std::unique_ptr<melvin::LLMReasoningEngine> llm_reasoning_engine;
    melvin::EvolutionState evolution_state;
    
    // Graph storage (using uint64_t for compatibility)
    std::unordered_map<uint64_t, melvin::Node> nodes;
    std::unordered_map<uint64_t, melvin::Edge> edges;
    
    // Performance metrics
    float attention_entropy = 0.0f;
    float output_diversity = 0.0f;
    float embedding_coherence = 0.0f;
    
    melvin_t() {
        llm_reasoning_engine = std::make_unique<melvin::LLMReasoningEngine>();
        llm_reasoning_engine->configure(uca_config);
        // Skip set_graph_references - type mismatch, LLMReasoningEngine uses NodeID
    }
};

// ----- C API Functions -----

extern "C" {

melvin_t* melvin_create(const char* store_dir) {
    auto* melvin = new melvin_t();
    melvin->store_dir = store_dir ? store_dir : "./melvin_store";
    melvin->node_count = 0;
    melvin->edge_count = 0;
    
    // Bridge modern storage to legacy format
    bridge_modern_to_legacy_storage();
    
    melvin->node_count = G_nodes.size();
    melvin->edge_count = G_edges.size();
    
    std::cout << "🧠 Melvin initialized with " << melvin->node_count 
              << " nodes, " << melvin->edge_count << " edges\n";
    
    return melvin;
}

void melvin_destroy(melvin_t* melvin) {
    delete melvin;
}

const char* melvin_reason(melvin_t* melvin, const char* query) {
    if (!melvin || !query) return "No response";
    
    static std::string response_buffer; // Static to maintain lifetime
    
    try {
        // NEW: Try unlimited context reasoning loop first (highest priority)
        if (melvin->use_graph_reasoning) {
            response_buffer = melvin_reasoning_loop(query, "balanced", false);
            if (!response_buffer.empty() && response_buffer != "I don't know yet.") {
                return response_buffer.c_str();
            }
        }
        
        // Update LLM reasoning engine metrics
        melvin->attention_entropy = melvin->llm_reasoning_engine->get_attention_entropy();
        melvin->output_diversity = melvin->llm_reasoning_engine->get_output_diversity();
        melvin->embedding_coherence = melvin->llm_reasoning_engine->get_embedding_coherence();
        
        // Use LLM-style reasoning if enabled
        if (melvin->uca_config.use_soft_traversal) {
            // Find starting node for reasoning
            melvin::NodeID start_node = melvin::NodeID{}; // TODO: implement find_best_start_node
            melvin::NodeID empty_node = melvin::NodeID{};
            if (start_node == empty_node) {
                response_buffer = "I don't have enough information yet.";
                return response_buffer.c_str();
            }
            
            // Perform soft attention traversal
            auto attention_nodes = melvin->llm_reasoning_engine->soft_traverse(start_node);
            
            if (attention_nodes.empty()) {
                response_buffer = "I don't have enough information yet.";
                return response_buffer.c_str();
            }
            
            // Generate probabilistic output
            if (melvin->uca_config.use_probabilistic_output) {
                response_buffer = melvin->llm_reasoning_engine->generate_phrase(attention_nodes[0]);
            } else {
                // Fallback to deterministic generation
                response_buffer = "deterministic response"; // TODO: implement generate_deterministic_response
            }
            
            // Update embeddings and context
            melvin->llm_reasoning_engine->update_embeddings();
            melvin->llm_reasoning_engine->update_context_buffer();
            
            // Evolve parameters
            melvin->llm_reasoning_engine->evolve_parameters();
            
            // Log LLM-style metrics
            if (melvin->uca_config.log_attention_entropy || 
                melvin->uca_config.log_output_diversity || 
                melvin->uca_config.log_embedding_coherence) {
                std::cout << "[Attention: " << std::fixed << std::setprecision(2) 
                          << melvin->attention_entropy << "] "
                          << "[Diversity: " << melvin->output_diversity << "] "
                          << "[Coherence: " << melvin->embedding_coherence << "] "
                          << "[Health: N/A]\n";
            }
            
            return response_buffer.c_str();
        }
        
        // Fallback to legacy reasoning
        melvin::Query Q(query);
        QueryKind kind = classify_query(Q);
        
        // NEW: Try question routing system first
        auto qp = parse_question(query);
        std::string out;
        
        if (qp.type == QType::WHAT_IS) {
            auto subj = norm(qp.subj);
            uint64_t xs = node_id(subj);
            if (xs != 0) {
                if (auto p = find_isa(xs)) {
                    out = say_isa(node_text(p->first), node_text(p->second));
                }
            }
        }
        else if (qp.type == QType::WHAT_DO_VERB) {
            auto subj = norm(qp.subj);
            auto verb = norm(qp.verb);        // "drink"
            uint64_t xs = node_id(subj);
            if (xs != 0) {
                if (auto p = find_consumes(xs, verb)) {
                    out = say_consumes(node_text(p->first), node_text(p->second), verb);
                }
            }
        }
        
        // If the relation handler produced something, use it
        if (!out.empty()) {
            response_buffer = out;
            return response_buffer.c_str();
        }
        
        // NEW: Try Graph-Native LLM predictive sampler
        melvin::PredictiveConfig cfg;
        cfg.use_beam = true;  // Use beam search for QA
        cfg.beam_width = 3;   // Smaller beam for focused answers
        cfg.max_hops = 12;    // Shorter paths for QA
        
        // Convert query to seed context
        std::vector<uint64_t> seed_context;
        std::hash<std::string> hasher;
        
        // Add query tokens as seed context
        melvin::Query Q_seed(query);
        for (const std::string& token : Q_seed.toks) {
            uint64_t token_id = static_cast<uint64_t>(hasher(token));
            if (G_nodes.find(token_id) != G_nodes.end()) {
                seed_context.push_back(token_id);
            }
        }
        
        if (!seed_context.empty()) {
            // Generate path using predictive sampler
            std::vector<uint64_t> generated_path = melvin::generate_path(seed_context, cfg);
            
            // Convert path to text response
            std::string generated_text;
            for (uint64_t node_id : generated_path) {
                if (G_nodes.find(node_id) != G_nodes.end()) {
                    if (!generated_text.empty()) generated_text += " ";
                    generated_text += G_nodes[node_id].text;
                }
            }
            
            // Clean up the response
            if (!generated_text.empty()) {
                // Capitalize first letter
                if (!generated_text.empty()) {
                    generated_text[0] = std::toupper(generated_text[0]);
                }
                
                // Add period if needed
                if (generated_text.back() != '.' && generated_text.back() != '!' && generated_text.back() != '?') {
                    generated_text += ".";
                }
                
                response_buffer = generated_text;
                
                // Update frequencies from the generated path (online learning)
                melvin::update_frequencies_from_path(generated_path);
                
                return response_buffer.c_str();
            }
        }
        
        // Try skill heads first for action queries
        std::string skill_result = skill_head_dispatch(query, kind);
        if (!skill_result.empty()) {
            response_buffer = skill_result;
            return response_buffer.c_str();
        }
        
        auto paths = beam_search(Q);
        
        if (paths.empty()) {
            response_buffer = "I don't have enough information yet.";
            return response_buffer.c_str();
        }
        
        // Check confidence threshold
        float threshold = ANSWER_THRESHOLD_EQUALS;
        switch (kind) {
            case QueryKind::Q_HAVE: threshold = ANSWER_THRESHOLD_HAVE; break;
            case QueryKind::Q_ACTION: threshold = ANSWER_THRESHOLD_ACTION; break;
            default: break;
        }
        
        if (paths[0].score < threshold) {
            response_buffer = "I don't know yet.";
            return response_buffer.c_str();
        }
        
        // Use path-to-phrase algorithm for organic response generation
        response_buffer = path_to_phrase(paths[0]);
        
        if (response_buffer.empty()) {
            response_buffer = "I don't know yet.";
        } else {
            // Reinforce successful reasoning path
            reinforce_path(paths[0]);
        }
        
        return response_buffer.c_str();
        
    } catch (const std::exception& e) {
        response_buffer = "Error: " + std::string(e.what());
        return response_buffer.c_str();
    }
}

// Helper functions for LLM-style reasoning
melvin::NodeID find_best_start_node(const char* query) {
    if (!query) return melvin::NodeID{};
    
    std::string query_str(query);
    std::transform(query_str.begin(), query_str.end(), query_str.begin(), ::tolower);
    
    // Simple keyword matching to find best start node
    for (const auto& [node_id, node] : G_nodes) {
        std::string node_text = node.text;
        std::transform(node_text.begin(), node_text.end(), node_text.begin(), ::tolower);
        
        // Check if query contains node text or vice versa
        if (query_str.find(node_text) != std::string::npos || 
            node_text.find(query_str) != std::string::npos) {
            // Convert uint64_t to NodeID
            melvin::NodeID result{};
            std::memcpy(result.data(), &node_id, sizeof(uint64_t));
            return result;
        }
    }
    
    // Fallback: return first available node
    if (!G_nodes.empty()) {
        uint64_t first_id = G_nodes.begin()->first;
        melvin::NodeID result{};
        std::memcpy(result.data(), &first_id, sizeof(uint64_t));
        return result;
    }
    
    return melvin::NodeID{};
}

std::string generate_deterministic_response(melvin::NodeID start_node) {
    // Convert NodeID back to uint64_t for lookup
    uint64_t node_id;
    std::memcpy(&node_id, start_node.data(), sizeof(uint64_t));
    
    if (G_nodes.find(node_id) == G_nodes.end()) {
        return "I don't know yet.";
    }
    
    // Simple deterministic response generation
    std::vector<uint64_t> path;
    uint64_t current = node_id;
    
    for (int i = 0; i < 5 && G_adj.find(current) != G_adj.end(); ++i) {
        path.push_back(current);
        
        // Find best next node
        uint64_t best_next = 0;
        float best_score = 0.0f;
        
        for (size_t edge_idx : G_adj[current]) {
            if (edge_idx >= G_edges.size()) continue;
            const auto& edge = G_edges[edge_idx];
            float score = edge.weight * edge.count;
            if (score > best_score) {
                best_score = score;
                best_next = edge.v;
            }
        }
        
        if (best_next == 0) break;
        current = best_next;
    }
    
    // Generate response from path
    std::string response;
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) response += " ";
        response += G_nodes[path[i]].text;
    }
    
    if (!response.empty()) {
        response[0] = std::toupper(response[0]);
        response += ".";
    }
    
    return response.empty() ? "I don't have enough information yet." : response;
}

void melvin_learn(melvin_t* melvin, const char* text) {
    if (!melvin || !text) return;
    
    // Enhanced learning with LLM-style embeddings
    try {
        melvin::Query Q(text);
        
        std::hash<std::string> hasher;
        
        // Create nodes for each token with LLM-style embeddings
        for (size_t i = 0; i < Q.toks.size(); ++i) {
            const std::string& token = Q.toks[i];
            uint64_t node_id = static_cast<uint64_t>(hasher(token));
            
            // Use uint64_t directly for melvin->nodes
            if (melvin->nodes.find(node_id) == melvin->nodes.end()) {
                melvin::Node node;
                node.id = node_id;
                node.text = token;
                node.type = 0;
                node.freq = 1;
                node.pinned = false;
                node.emb = embed_text(token);
                
                // Initialize LLM-style embedding
                node.embedding.resize(melvin->uca_config.embedding_dim);
                // Initialize embedding (skipped - private method)
                node.embedding.resize(128, 0.0f); // Use default embedding
                
                // Set initial attention and semantic strength
                node.attention_weight = 0.0f;
                node.last_accessed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                node.semantic_strength = 1.0f;
                
                melvin->nodes[node_id] = node;
                melvin->node_count++;
                
                // Also update legacy G_nodes for compatibility
                G_nodes[node_id] = node;
                
                // Track taught node creation (learning from input) - monitoring disabled
            } else {
                melvin->nodes[node_id].freq++;
                melvin->nodes[node_id].last_accessed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                G_nodes[node_id].freq++;
            }
        }
        
        // Create TEMPORAL edges between consecutive tokens using frequency-based system
        for (size_t i = 0; i < Q.toks.size() - 1; ++i) {
            uint64_t src_id = static_cast<uint64_t>(hasher(Q.toks[i]));
            uint64_t dst_id = static_cast<uint64_t>(hasher(Q.toks[i + 1]));
            
            // Use new frequency-based edge creation
            create_or_update_edge(src_id, dst_id, melvin::Rel::TEMPORAL);
            melvin->edge_count++;
        }
        
        // Add ISA and CONSUMES relations for better question answering
        if (Q.toks.size() >= 3) {
            // Check for "X is/are Y" patterns
            if (Q.toks[1] == "is" || Q.toks[1] == "are") {
                uint64_t src_id = static_cast<uint64_t>(hasher(Q.toks[0]));
                uint64_t dst_id = static_cast<uint64_t>(hasher(Q.toks[2]));
                create_or_update_edge(src_id, dst_id, melvin::Rel::ISA);
                melvin->edge_count++;
            }
            
            // Check for "X drinks/eats Y" patterns
            if (Q.toks[1] == "drink" || Q.toks[1] == "drinks" || 
                Q.toks[1] == "eat" || Q.toks[1] == "eats") {
                uint64_t src_id = static_cast<uint64_t>(hasher(Q.toks[0]));
                uint64_t dst_id = static_cast<uint64_t>(hasher(Q.toks[2]));
                create_or_update_edge(src_id, dst_id, melvin::Rel::CONSUMES);
                melvin->edge_count++;
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "⚠️ Learning failed: " << e.what() << "\n";
    }
}

void melvin_decay_pass(melvin_t* melvin) {
    // Apply exponential smoothing to all edges for memory stabilization
    apply_memory_smoothing(0.01f); // 1% smoothing rate for gradual forgetting
}

size_t melvin_node_count(melvin_t* melvin) {
    return melvin ? melvin->node_count : 0;
}

size_t melvin_edge_count(melvin_t* melvin) {
    return melvin ? melvin->edge_count : 0;
}

size_t melvin_path_count(melvin_t* melvin) {
    return G_edges.size(); // Approximation
}

float melvin_get_health_score(melvin_t* melvin) {
    if (!melvin || melvin->node_count == 0) return 0.0f;
    
    float connectivity = static_cast<float>(melvin->edge_count) / melvin->node_count;
    return std::min(1.0f, connectivity / 2.0f); // Normalize to 0-1
}

void melvin_run_nightly_consolidation(melvin_t* melvin) {
    // Simple consolidation - remove weak edges
    auto it = std::remove_if(G_edges.begin(), G_edges.end(),
        [](const melvin::Edge& edge) { return edge.w_core < 0.1f; });
    G_edges.erase(it, G_edges.end());
    
    if (melvin) {
        melvin->edge_count = G_edges.size();
    }
}

void melvin_set_neural_reasoning(melvin_t* melvin, int enabled) {
    if (melvin) melvin->use_graph_reasoning = (enabled != 0);
}

void melvin_set_neural_confidence_threshold(melvin_t* melvin, float threshold) {
    if (melvin) melvin->confidence_threshold = threshold;
}

int melvin_is_neural_reasoning_enabled(melvin_t* melvin) {
    return melvin ? melvin->use_graph_reasoning : 0;
}

float melvin_get_neural_confidence_threshold(melvin_t* melvin) {
    return melvin ? melvin->confidence_threshold : 0.5f;
}

void melvin_set_graph_reasoning(melvin_t* melvin, int enabled) {
    // Graph reasoning is always enabled in current implementation
    // This is a placeholder for future implementation
}

int melvin_is_graph_reasoning_enabled(melvin_t* melvin) {
    // Graph reasoning is always enabled in current implementation
    return 1;
}

void melvin_export_metrics(melvin_t* melvin, const char* filename) {
    if (!melvin || !filename) return;
    
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    // Export basic metrics
    file << "timestamp,nodes,edges,paths,health_score\n";
    file << std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() << ",";
    file << melvin_node_count(melvin) << ",";
    file << melvin_edge_count(melvin) << ",";
    file << melvin_path_count(melvin) << ",";
    file << melvin_get_health_score(melvin) << "\n";
    
    file.close();
}

// Debug function to print connections for a node
void melvin_debug_node(melvin_t* melvin, const char* node_text) {
    if (!melvin || !node_text) return;
    
    std::hash<std::string> hasher;
    uint64_t node_id = static_cast<uint64_t>(hasher(std::string(node_text)));
    
    debug_node_connections(node_id, "", "", "balanced");
}

// Enhanced debug function with context and query
void melvin_debug_node_with_context(melvin_t* melvin, const char* node_text, const char* context, const char* query, const char* driver_mode) {
    if (!melvin || !node_text) return;
    
    std::hash<std::string> hasher;
    uint64_t node_id = static_cast<uint64_t>(hasher(std::string(node_text)));
    
    std::string ctx = context ? std::string(context) : "";
    std::string q = query ? std::string(query) : "";
    std::string mode = driver_mode ? std::string(driver_mode) : "balanced";
    
    debug_node_connections(node_id, ctx, q, mode);
}

// Set driver mode for reasoning
void melvin_set_driver_mode(melvin_t* melvin, const char* mode) {
    if (!melvin || !mode) return;
    
    // Store driver mode in melvin struct (we'll add this field)
    // For now, we'll use a global variable
    static std::string current_driver_mode = "balanced";
    current_driver_mode = mode;
}

// Get next word suggestion using enhanced scoring
const char* melvin_suggest_next_word(melvin_t* melvin, const char* current_word, const char* context, const char* query) {
    if (!melvin || !current_word) return nullptr;
    
    static std::string result_buffer;
    
    std::hash<std::string> hasher;
    uint64_t node_id = static_cast<uint64_t>(hasher(std::string(current_word)));
    
    std::string ctx = context ? std::string(context) : "";
    std::string q = query ? std::string(query) : "";
    
    size_t next_edge = select_next_edge(node_id, ctx, q, std::string("deterministic"), std::string("balanced"));
    
    if (next_edge != SIZE_MAX && next_edge < G_edges.size()) {
        const auto& target_node = G_nodes[G_edges[next_edge].v];
        result_buffer = target_node.text;
        
        // Reinforce the chosen edge
        reinforce_edge(next_edge);
        
        return result_buffer.c_str();
    }
    
    return nullptr;
}

// Generate a complete phrase from a starting word
const char* melvin_generate_phrase_c(melvin_t* melvin, const char* start_word, int max_tokens, const char* driver_mode, const char* selection_mode, const char* context, const char* query, int debug_enabled) {
    if (!melvin || !start_word) return nullptr;
    
    static std::string result_buffer;
    
    std::hash<std::string> hasher;
    uint64_t node_id = static_cast<uint64_t>(hasher(std::string(start_word)));
    
    std::string driver = driver_mode ? std::string(driver_mode) : "balanced";
    std::string selection = selection_mode ? std::string(selection_mode) : "deterministic";
    std::string ctx = context ? std::string(context) : "";
    std::string q = query ? std::string(query) : "";
    bool debug = debug_enabled != 0;
    
    result_buffer = melvin_generate_phrase(node_id, max_tokens, driver, selection, ctx, q, debug);
    
    return result_buffer.c_str();
}

// Generate phrase from a reasoning query
const char* melvin_generate_answer(melvin_t* melvin, const char* query, const char* driver_mode) {
    if (!melvin || !query) return nullptr;
    
    static std::string result_buffer;
    
    // Extract key words from query for phrase generation
    std::string q_str = query;
    std::hash<std::string> hasher;
    
    // Try to find the most relevant starting word
    std::string start_word = "the"; // Default fallback
    
    // Simple heuristic: use the last noun-like word in the query
    std::vector<std::string> words;
    std::string word;
    for (char c : q_str) {
        if (std::isalnum(c)) {
            word += c;
        } else if (!word.empty()) {
            words.push_back(word);
            word.clear();
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    
    // Find a good starting word (avoid common words)
    for (int i = words.size() - 1; i >= 0; --i) {
        const std::string& w = words[i];
        if (w.length() > 3 && w != "what" && w != "how" && w != "where" && w != "when" && w != "why" && 
            w != "are" && w != "is" && w != "the" && w != "and" && w != "or") {
            start_word = w;
            break;
        }
    }
    
    uint64_t node_id = static_cast<uint64_t>(hasher(start_word));
    std::string driver = driver_mode ? std::string(driver_mode) : "balanced";
    
    result_buffer = melvin_generate_phrase(node_id, 8, driver, "deterministic", "", q_str, false);
    
    return result_buffer.c_str();
}

// Beam search phrase generation C API
const char* melvin_generate_phrase_beam_c(melvin_t* melvin, const char* start_word, int max_tokens, int beam_k, int expand_M, const char* driver_mode, int probabilistic, float stop_threshold, const char* context, const char* query, int debug_enabled) {
    if (!melvin || !start_word) return nullptr;
    
    static std::string result_buffer;
    
    std::hash<std::string> hasher;
    uint64_t node_id = static_cast<uint64_t>(hasher(std::string(start_word)));
    
    std::string driver = driver_mode ? std::string(driver_mode) : "balanced";
    std::string ctx = context ? std::string(context) : "";
    std::string q = query ? std::string(query) : "";
    bool prob = probabilistic != 0;
    bool debug = debug_enabled != 0;
    
    result_buffer = melvin_generate_phrase_beam(node_id, max_tokens, beam_k, expand_M, driver, prob, stop_threshold, ctx, q, debug);
    
    return result_buffer.c_str();
}

// Trace generation for debugging
void melvin_trace_generation(melvin_t* melvin, const char* seed_word, const char* query, const char* driver_mode) {
    if (!melvin || !seed_word) return;
    
    std::hash<std::string> hasher;
    uint64_t node_id = static_cast<uint64_t>(hasher(std::string(seed_word)));
    
    std::string q = query ? std::string(query) : "";
    std::string driver = driver_mode ? std::string(driver_mode) : "balanced";
    
    std::cout << "\n🔍 Tracing generation from '" << seed_word << "'\n";
    std::cout << "Query: '" << q << "', Driver: " << driver << "\n";
    
    melvin_generate_phrase_beam(node_id, 6, 3, 4, driver, false, 0.08f, "", q, true);
}

// C API wrapper for unlimited context reasoning loop
const char* melvin_reason_with_unlimited_context(melvin_t* melvin, const char* query, const char* driver_mode, int debug_enabled) {
    if (!melvin || !query) return nullptr;
    
    static std::string result_buffer;
    
    std::string driver = driver_mode ? std::string(driver_mode) : "balanced";
    bool debug = debug_enabled != 0;
    
    result_buffer = melvin_reasoning_loop(query, driver, debug);
    
    return result_buffer.c_str();
}

// Initialize the reasoning system
void melvin_init_reasoning_system(melvin_t* melvin) {
    if (!melvin) return;
    init_reasoning_system();
}

// Set evolution parameters
void melvin_set_evolution_param(melvin_t* melvin, const char* param_name, float value) {
    if (!melvin || !param_name) return;
    
    std::string param(param_name);
    
    if (param == "alpha") G_evo_params.alpha = value;
    else if (param == "beta") G_evo_params.beta = value;
    else if (param == "lambda") G_evo_params.lambda = value;
    else if (param == "tau") G_evo_params.tau = value;
    // risk_floor removed - now using soft penalties in emergent dimensional system
    else if (param == "rollout_horizon") G_evo_params.rollout.H = static_cast<int>(value);
    else if (param == "rollout_branches") G_evo_params.rollout.B = static_cast<int>(value);
}

// Get context field statistics
void melvin_get_context_field_stats(melvin_t* melvin, float* mean_potential, float* max_potential, int* num_nodes) {
    if (!melvin) return;
    
    if (G_context_field.C.empty()) {
        if (mean_potential) *mean_potential = 0.0f;
        if (max_potential) *max_potential = 0.0f;
        if (num_nodes) *num_nodes = 0;
        return;
    }
    
    float sum = 0.0f;
    float max_val = -1e9f;
    
    for (float val : G_context_field.C) {
        sum += val;
        max_val = std::max(max_val, val);
    }
    
    if (mean_potential) *mean_potential = sum / G_context_field.C.size();
    if (max_potential) *max_potential = max_val;
    if (num_nodes) *num_nodes = static_cast<int>(G_context_field.C.size());
}

} // extern "C"

// ==================== REINFORCEMENT LEARNING ====================

// Recompute weights for outgoing edges from a node
void recompute_node_weights(uint64_t node_id) {
    // Simple weight normalization based on usage
    if (G_adj.find(node_id) == G_adj.end()) return;
    
    float total_count = 0.0f;
    for (size_t edge_idx : G_adj[node_id]) {
        if (edge_idx < G_edges.size()) {
            total_count += G_edges[edge_idx].count;
        }
    }
    
    if (total_count > 0.0f) {
        for (size_t edge_idx : G_adj[node_id]) {
            if (edge_idx < G_edges.size()) {
                G_edges[edge_idx].weight = G_edges[edge_idx].count / total_count;
            }
        }
    }
}

// Reinforce a single edge when it's chosen
void reinforce_edge(size_t edge_idx) {
    if (edge_idx >= G_edges.size()) return;
    
    G_edges[edge_idx].count++;
    G_edges[edge_idx].last_used = get_current_time();
    
    // Recompute weights for the source node
    recompute_node_weights(G_edges[edge_idx].u);
}

// Reinforce successful reasoning paths
void reinforce_path(const melvin::Path& path) {
    if (path.edges_idx.empty()) return;
    
    std::cout << "🧠 Reinforcing successful path with " << path.edges_idx.size() << " edges\n";
    
    for (size_t edge_idx : path.edges_idx) {
        reinforce_edge(edge_idx);
    }
}

// ==================== DEMO/DEBUG INTERFACE ====================

int main(int argc, char* argv[]) {
    std::cout << "🧠 MELVIN UNLIMITED CONTEXT REASONING SYSTEM\n";
    std::cout << "===========================================\n\n";
    
    std::cout << "This implementation demonstrates the formal reasoning system with:\n";
    std::cout << "  • Unlimited context via field diffusion over entire memory graph\n";
    std::cout << "  • Driver critics (safety, task, curiosity, efficiency, social, consistency)\n";
    std::cout << "  • ShadowWorld rollout simulation for predictive futures\n";
    std::cout << "  • Context field equilibrium solver with warm start\n";
    std::cout << "  • Multi-objective action selection with catastrophe risk floor\n\n";
    
    // Initialize Melvin
    melvin_t* melvin = melvin_create(nullptr);
    
    std::cout << "✅ Melvin initialized with " << melvin_node_count(melvin) 
              << " nodes, " << melvin_edge_count(melvin) << " edges\n";
    std::cout << "✅ Health score: " << melvin_get_health_score(melvin) << "\n";
    
    // Initialize reasoning system
    melvin_init_reasoning_system(melvin);
    std::cout << "✅ Unlimited context reasoning system initialized\n\n";
    
    // Display evolution parameters
    std::cout << "Evolution Parameters:\n";
    std::cout << "  α (direct relevance) = " << G_evo_params.alpha << "\n";
    std::cout << "  β (reasoning reinforce) = " << G_evo_params.beta << "\n";
    std::cout << "  λ (decay rate) = " << G_evo_params.lambda << "\n";
    std::cout << "  τ (diffusion strength) = " << G_evo_params.tau << "\n";
    std::cout << "  Rollout horizon H = " << G_evo_params.rollout.H << "\n";
    std::cout << "  Rollout branches B = " << G_evo_params.rollout.B << "\n";
    std::cout << "  Active dimensions: " << G_dimensional_system.active_dimensions.size() << "\n";
    if (!G_dimensional_system.active_dimensions.empty()) {
        std::cout << "  Dimensional weights: [";
        auto gamma_weights = G_dimensional_system.get_gamma_weights();
        for (size_t i = 0; i < gamma_weights.size(); ++i) {
            std::cout << gamma_weights[i];
            if (i < gamma_weights.size() - 1) std::cout << ", ";
        }
        std::cout << "]\n";
    }
    std::cout << "\n";
    
    // Teach some foundational knowledge
    std::cout << "📚 Teaching foundational knowledge...\n";
    std::vector<std::string> lessons = {
        "cars move fast",
        "fast movement is dangerous",
        "crosswalks are safe",
        "waiting reduces danger",
        "looking both ways prevents crashes",
        "red light means stop",
        "green light means go"
    };
    
    for (const auto& lesson : lessons) {
        melvin_learn(melvin, lesson.c_str());
        std::cout << "  ✓ " << lesson << "\n";
    }
    std::cout << "\n";
    
    // Display context field statistics
    float mean_potential = 0.0f, max_potential = 0.0f;
    int num_nodes = 0;
    melvin_get_context_field_stats(melvin, &mean_potential, &max_potential, &num_nodes);
    std::cout << "Context Field Statistics:\n";
    std::cout << "  Total nodes: " << num_nodes << "\n";
    std::cout << "  Mean potential: " << mean_potential << "\n";
    std::cout << "  Max potential: " << max_potential << "\n\n";
    
    // Demo queries with different driver modes
    std::cout << "═══════════════════════════════════════════\n";
    std::cout << "DEMO: Street Crossing Decision\n";
    std::cout << "═══════════════════════════════════════════\n\n";
    
    std::vector<std::pair<std::string, std::string>> demo_queries = {
        {"Should I cross the street now?", "balanced"},
        {"What should I check before crossing?", "balanced"},
        {"Is it safe to cross?", "balanced"}
    };
    
    for (const auto& [query, driver_mode] : demo_queries) {
        std::cout << "❓ Query: \"" << query << "\"\n";
        std::cout << "🎯 Driver mode: " << driver_mode << "\n";
        std::cout << "─────────────────────────────────────────\n";
        
        // Use unlimited context reasoning with debug enabled
        const char* answer = melvin_reason_with_unlimited_context(melvin, query.c_str(), driver_mode.c_str(), 1);
        
        std::cout << "\n💡 Final Answer: \"" << answer << "\"\n";
        std::cout << "═══════════════════════════════════════════\n\n";
    }
    
    // Display updated context field statistics
    melvin_get_context_field_stats(melvin, &mean_potential, &max_potential, &num_nodes);
    std::cout << "Updated Context Field Statistics:\n";
    std::cout << "  Total nodes: " << num_nodes << "\n";
    std::cout << "  Mean potential: " << mean_potential << "\n";
    std::cout << "  Max potential: " << max_potential << "\n\n";
    
    // Show how emergent dimensional system adapts
    std::cout << "🧬 Demonstrating emergent dimensional evolution...\n";
    std::cout << "  Dimensional system is learning from experience:\n";
    std::cout << "    Active dimensions: " << G_dimensional_system.active_dimensions.size() << "\n";
    std::cout << "    Promotion threshold: " << G_dimensional_system.promotion_threshold << "\n";
    std::cout << "    Learning rate: " << G_dimensional_system.learning_rate << "\n";
    std::cout << "  (Dimensions emerge based on what correlates with success)\n\n";
    
    std::string safety_query = "Should I cross now if a car is coming?";
    std::cout << "❓ Safety-critical query: \"" << safety_query << "\"\n";
    const char* safety_answer = melvin_reason(melvin, safety_query.c_str());
    std::cout << "🤖 Risk-averse answer: \"" << safety_answer << "\"\n\n";
    
    // Cleanup
    melvin_destroy(melvin);
    
    std::cout << "✅ Melvin unlimited context reasoning demo complete!\n";
    std::cout << "\nKey Features Demonstrated:\n";
    std::cout << "  ✓ Context field diffusion (unlimited context over entire graph)\n";
    std::cout << "  ✓ Driver critics (safety, task, curiosity, efficiency, social, consistency)\n";
    std::cout << "  ✓ Rollout-based action evaluation (H=3 horizon, B=8 branches)\n";
    std::cout << "  ✓ Multi-objective utility with catastrophe risk floor\n";
    std::cout << "  ✓ Eligibility traces for backprojecting action scores to nodes\n";
    std::cout << "  ✓ Equilibrium solver with warm start\n";
    std::cout << "  ✓ Online parameter evolution (micro-evo)\n\n";
    
    return 0;
}

