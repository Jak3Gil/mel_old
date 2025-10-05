# Melvin's Mathematical Foundations

## Overview
Melvin is built on a sophisticated mathematical framework that combines graph theory, probability theory, information theory, and evolutionary algorithms to create a unified cognitive architecture. Here are the key mathematical components:

## 1. Graph-Based Knowledge Representation

### Binary 2-Bit Relation System
Melvin uses a compact 4-relation system encoded in 2 bits:

```
Rel::EXACT = 0        (00) - Direct semantic match, definition, identity
Rel::TEMPORAL = 1     (01) - Sequential, causal, temporal ordering  
Rel::LEAP = 2         (10) - Inference, hypothesis, creative connection
Rel::GENERALIZATION = 3 (11) - Abstraction, pattern, category membership
```

### Content-Addressed IDs
All entities use 32-byte content-addressed identifiers:
- `NodeID = std::array<uint8_t, 32>`
- `EdgeID = std::array<uint8_t, 32>` 
- `PathID = std::array<uint8_t, 32>`

## 2. Edge Scoring Mathematics

### Core Edge Score Formula
```cpp
float score_edge(const EdgeRec& edge) {
    // Context/core mix with proper weighting
    float m = lambda_mix_ * edge.w_ctx + (1.0f - lambda_mix_) * edge.w_core;
    
    // Apply size-relative scaling
    float size_scaling = get_size_scaling_factor();
    m *= size_scaling;
    
    // Recency factor (exponential decay)
    float dt = (now - edge.ts_last) / 1e9f; // Convert to seconds
    float rho = exp(-dt / tau_rec_);
    
    // Provenance trust (based on count and flags)
    float pi = min(1.0f, edge.count / 10.0f);
    if (edge.flags & EDGE_DEPRECATED) pi *= 0.1f;
    if (edge.flags & EDGE_GENERALIZED) pi *= 1.2f;
    
    // Curiosity-driven exploration bonus
    float curiosity_bonus = 1.0f + (drivers_.curiosity - 0.5f) * 0.2f;
    
    return m * rho * pi * curiosity_bonus;
}
```

### Mathematical Components:
- **λ-mixing**: `m = λ·w_ctx + (1-λ)·w_core` (context vs core weight balance)
- **Recency decay**: `ρ = e^(-Δt/τ)` where τ = recency time constant
- **Trust scaling**: `π = min(1.0, count/10)` with flag-based modifiers
- **Exploration bonus**: `1.0 + (curiosity - 0.5) × 0.2`

## 3. Path Scoring Mathematics

### Path Score Formula
```cpp
float score_path(const Path& path) {
    // Geometric mean of per-edge scores
    float product = 1.0f;
    for (float score : edge_scores) {
        product *= max(score, 0.001f); // Avoid zero
    }
    float G = pow(product, 1.0f / edge_scores.size());
    
    // Continuity bonus (γ_c = 1.1)
    float continuity_bonus = 1.0f + gamma_c_ * C;
    
    // Coverage bonus (γ_v = 1.1) - reward diverse subgraphs
    float coverage_bonus = 1.0f + gamma_v_ * log(1.0f + path.edges.size());
    
    // Exploration bonus - reward paths with inferred edges
    float exploration_bonus = 1.0f + (drivers_.curiosity - 0.5f) * 0.2f * inferred_count;
    
    // Confidence-based scoring
    float confidence_bonus = 1.0f + path.confidence * 0.1f;
    
    return G * continuity_bonus * coverage_bonus * exploration_bonus * confidence_bonus;
}
```

### Mathematical Components:
- **Geometric mean**: `G = (∏score_i)^(1/n)` for n edges
- **Continuity bonus**: `1.0 + γ_c × C` where C = consecutive same-relation count
- **Coverage bonus**: `1.0 + γ_v × log(1 + |edges|)` 
- **Exploration bonus**: `1.0 + (curiosity - 0.5) × 0.2 × inferred_count`

## 4. Confidence Calibration

### Logistic Confidence Function
```cpp
float compute_confidence(float path_score) {
    // Logistic function for confidence calibration
    float a = 5.0f;  // Steepness
    float b = -3.0f; // Offset
    return 1.0f / (1.0f + exp(-(a * path_score + b)));
}
```

**Formula**: `confidence = 1 / (1 + e^(-(a·score + b)))`
- **a = 5.0**: Controls steepness of the sigmoid
- **b = -3.0**: Controls horizontal offset

## 5. Beam Search Mathematics

### Beam Search Algorithm
Melvin uses beam search with probabilistic selection:

```cpp
// Probabilistic selection based on total scores
float total_score = 0.0f;
for (const auto& hyp : beam) {
    total_score += hyp.total_score;
}

// Roulette wheel selection
float random_value = uniform_real_distribution(0.0f, total_score);
float cumulative = 0.0f;
for (const auto& hyp : beam) {
    cumulative += hyp.total_score;
    if (random_value <= cumulative) {
        return hyp; // Selected hypothesis
    }
}
```

### Temperature-Based Sampling
```cpp
// Apply temperature for exploration vs exploitation
weights.push_back(pow(score, 1.0f / tau_exp_));
```

**Formula**: `weight = score^(1/τ)` where τ = exploration temperature

## 6. Frequency and Probability Calculations

### Laplace Smoothing
```cpp
float compute_laplace_freq(size_t edge_idx, uint64_t source_node) {
    const auto& edge = G_edges[edge_idx];
    double total_count = 0.0;
    
    // Sum all outgoing edges from source with Laplace smoothing
    for (size_t other_edge_idx : G_adj[source_node]) {
        total_count += G_edges[other_edge_idx].count + LAPLACE_ALPHA;
    }
    
    return (edge.count + LAPLACE_ALPHA) / total_count;
}
```

**Formula**: `P(edge) = (count + α) / (total_count + α)`
- **α = 0.5**: Laplace smoothing constant

### Mathematical Constants
```cpp
static constexpr float LAPLACE_ALPHA = 0.5f;      // Laplace smoothing
static constexpr float RECENCY_TAU = 300.0f;      // Recency time constant (5 min)
static constexpr float REPETITION_GAMMA = 0.8f;   // Repetition penalty factor
static constexpr float LENGTH_BETA = 0.5f;        // Length normalization exponent
```

## 7. Enhanced Scoring Formula

### Multi-Factor Scoring
```cpp
float compute_enhanced_score(size_t edge_idx, const vector<string>& phrase, 
                            int step, const string& context, const string& query, 
                            const string& driver_mode) {
    // Base factors
    float p_freq = compute_laplace_freq(edge_idx, edge.u);
    float syn_fit = syntax_fit(edge, context, query);
    float sem_fit = semantic_fit(edge, query);
    float drv_bias = driver_bias(edge, driver_mode);
    
    // Additional factors
    float recency = compute_recency_factor(edge);
    float rep_pen = compute_repetition_penalty(phrase, target_token);
    float len_norm = compute_length_norm(step);
    
    // Final multiplicative score
    return p_freq * syn_fit * sem_fit * drv_bias * recency * rep_pen * len_norm;
}
```

### Individual Factor Formulas:

**Recency Factor**: `recency = e^(-Δt/τ)` where τ = 300 seconds

**Repetition Penalty**: `rep_pen = γ^repeat_count` where γ = 0.8

**Length Normalization**: `len_norm = 1 / (5 + step)^β` where β = 0.5

## 8. Adaptive Thresholds

### Dynamic Threshold Scaling
```cpp
float calculate_adaptive_threshold(float base_threshold, size_t node_count) {
    if (node_count <= 100) { 
        return base_threshold * 0.3f; 
    }
    else if (node_count <= 1000) { 
        return base_threshold * 0.5f; 
    }
    else if (node_count <= 10000) { 
        return base_threshold * min(SCALE_FACTOR_MEDIUM, 
                                   1.0f + log10(node_count / 1000.0f)); 
    }
    else if (node_count <= 100000) {
        float log_scale = 1.0f + log10(node_count / SCALE_LOG_BASE);
        return base_threshold * min(log_scale, SCALE_FACTOR_HIGH);
    } 
    else { 
        return base_threshold * SCALE_FACTOR_HIGH; 
    }
}
```

**Scaling Factors**:
- **Small graphs** (≤100): 0.3× threshold
- **Medium graphs** (≤1K): 0.5× threshold  
- **Large graphs** (≤10K): `min(1.5, 1 + log₁₀(n/1000))`× threshold
- **Very large** (≤100K): `min(2.5, 1 + log₁₀(n/10))`× threshold
- **Massive** (>100K): 2.5× threshold

## 9. Evolutionary Mathematics

### Fitness Evaluation
```cpp
float evaluate_fitness(const DynamicGenome& genome) {
    float fitness = 0.0f;
    
    // Reward balanced confidence thresholds
    if (genome.values.count("confidence_threshold")) {
        float conf = genome.values.at("confidence_threshold");
        fitness += 1.0f - abs(conf - 0.5f); // Peak at 0.5
    }
    
    // Reward reasonable depth settings
    if (genome.values.count("max_depth")) {
        float depth = genome.values.at("max_depth");
        fitness += 1.0f - abs(depth - 6.0f) / 6.0f; // Peak at 6
    }
    
    // Reward moderate reinforcement rates
    if (genome.values.count("reinforcement_rate")) {
        float rate = genome.values.at("reinforcement_rate");
        fitness += 1.0f - abs(rate - 0.1f) / 0.1f; // Peak at 0.1
    }
    
    return fitness;
}
```

### Stagnation Detection
```cpp
ReflectReport reflect(const deque<float>& confidences, const deque<float>& rewards) {
    float c_avg = average(confidences);
    float r_avg = average(rewards);
    
    // Simple stagnation heuristic: both recent confidence & reward are low
    if (c_avg < 0.18f && r_avg < 0.10f) {
        return {true, "low confidence & reward moving averages", new_genome};
    }
    return {false, "", {}};
}
```

## 10. Information Theory Components

### Cosine Similarity
```cpp
float cosine_similarity(const vector<float>& a, const vector<float>& b) {
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    return dot / (sqrt(norm_a) * sqrt(norm_b));
}
```

**Formula**: `cos_sim(a,b) = (a·b) / (||a|| × ||b||)`

## 11. Neural-Graph Hybrid Mathematics

### Dual Weight System
Each edge maintains two weights:
- **w_core**: Core semantic weight (stable, learned)
- **w_ctx**: Contextual weight (dynamic, situational)

**Mixing Formula**: `w_final = λ·w_ctx + (1-λ)·w_core`

### Thought Trace Replayability
```cpp
struct ThoughtTrace {
    vector<EdgeID> edges;        // Replayable reasoning chain
    float confidence;           // Overall confidence
    uint64_t timestamp;         // Creation time
    bool is_durable;           // Persistence flag
};
```

## 12. Mathematical Properties

### Convergence Guarantees
- **Beam search**: Guaranteed to find optimal path within beam width
- **Laplace smoothing**: Prevents zero probabilities, maintains probability axioms
- **Logistic confidence**: Bounded between 0 and 1, differentiable everywhere
- **Exponential decay**: Ensures recency effects diminish smoothly

### Computational Complexity
- **Edge scoring**: O(1) per edge
- **Path scoring**: O(k) where k = path length  
- **Beam search**: O(B × M × D) where B = beam size, M = max edges, D = depth
- **Frequency calculation**: O(out_degree) per node

### Memory Efficiency
- **32-byte IDs**: Content-addressed, collision-resistant
- **2-bit relations**: Minimal storage for 4 relation types
- **Compressed storage**: Only essential edge data persisted

This mathematical foundation provides Melvin with robust, theoretically-grounded algorithms for knowledge representation, reasoning, learning, and adaptation while maintaining computational efficiency and mathematical rigor.
