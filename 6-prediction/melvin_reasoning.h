#pragma once

#include "melvin_graph.h"
#include <vector>
#include <string>
#include <memory>

namespace melvin {
namespace uca {

// ============================================================================
// BIOLOGICAL ANALOG: Prefrontal Cortex (PFC)
// ============================================================================

/**
 * Reasoning types matching PFC subregions
 */
enum ReasoningMode {
    REACTIVE,         // Fast, graph-only (dorsal PFC)
    DELIBERATIVE,     // Slow, multi-hop (ventral PFC)
    PREDICTIVE,       // Forward simulation (mPFC)
    METACOGNITIVE     // Self-reflection (aPFC)
};

/**
 * Thought structure: Subject-Predicate-Object triple
 */
struct Thought {
    std::string subject;
    std::string predicate;
    std::string object;
    
    float confidence;
    ReasoningMode mode;
    
    std::vector<uint64_t> graph_path;  // Supporting nodes
    std::vector<std::string> evidence; // Explanation
    
    std::string to_string() const {
        return subject + " " + predicate + " " + object;
    }
};

/**
 * Reasoning query
 */
struct Query {
    std::string text;
    std::vector<std::string> keywords;
    ReasoningMode preferred_mode;
    int max_hops;
    float min_confidence;
    
    Query() : preferred_mode(DELIBERATIVE), max_hops(10), min_confidence(0.3f) {}
};

/**
 * Reasoning result
 */
struct ReasoningResult {
    std::vector<Thought> thoughts;
    std::string explanation;
    float overall_confidence;
    int total_paths_explored;
    double reasoning_time_ms;
    
    ReasoningResult() : overall_confidence(0), total_paths_explored(0), reasoning_time_ms(0) {}
};

// ============================================================================
// MELVIN REASONING - Hybrid Graph + Transformer
// ============================================================================

/**
 * MelvinReasoning: Prefrontal cortex analog
 * 
 * Implements:
 * 1. Graph-based reasoning (symbolic path finding)
 * 2. Transformer-like sequence prediction (pattern completion)
 * 3. Hybrid fusion: P(next) = α·P_graph + β·P_transformer
 * 4. Meta-reasoning (confidence estimation)
 * 
 * Biological parallels:
 * - Graph reasoning → Explicit reasoning (dorsolateral PFC)
 * - Pattern completion → Implicit associations (ventral PFC)
 * - Hybrid fusion → Integration (frontopolar cortex)
 * - Meta-cognition → Self-monitoring (anterior PFC)
 */
class MelvinReasoning {
public:
    struct Config {
        // Hybrid weights
        float alpha_graph;        // Graph contribution (default: 0.6)
        float beta_transformer;   // Transformer contribution (default: 0.4)
        
        // Graph reasoning
        int max_reasoning_hops;   // Maximum path length (default: 10)
        float min_edge_confidence; // Threshold for traversal (default: 0.2)
        bool allow_leap_edges;    // Use inferred edges (default: true)
        
        // Transformer (simplified for now)
        bool enable_transformer;  // Use pattern completion (default: false)
        int context_window;       // Previous thoughts to consider (default: 5)
        
        // Meta-reasoning
        bool explain_reasoning;   // Generate explanations (default: true)
        bool estimate_confidence; // Compute uncertainty (default: true)
        
        Config() : alpha_graph(0.6f), beta_transformer(0.4f),
                   max_reasoning_hops(10), min_edge_confidence(0.2f),
                   allow_leap_edges(true), enable_transformer(false),
                   context_window(5), explain_reasoning(true),
                   estimate_confidence(true) {}
    };
    
    MelvinReasoning(MelvinGraph* graph, const Config& config = Config());
    ~MelvinReasoning();
    
    // ========================================================================
    // MAIN REASONING INTERFACE
    // ========================================================================
    
    /**
     * Answer a query using hybrid graph + transformer reasoning
     */
    ReasoningResult reason(const Query& query);
    
    /**
     * Generate next thought given current context
     * Used for continuous thinking / internal monologue
     */
    Thought generate_next_thought(const std::vector<Thought>& context);
    
    /**
     * Validate a statement against graph knowledge
     * Returns confidence that statement is true
     */
    float validate_statement(const std::string& statement);
    
    /**
     * Explain why a thought is generated
     * Traces graph paths and provides natural language explanation
     */
    std::string explain_thought(const Thought& thought);
    
    // ========================================================================
    // REASONING MODES
    // ========================================================================
    
    /**
     * REACTIVE: Fast single-hop lookup
     * Like System 1 (fast, automatic)
     */
    ReasoningResult reason_reactive(const Query& query);
    
    /**
     * DELIBERATIVE: Multi-hop graph search
     * Like System 2 (slow, effortful)
     */
    ReasoningResult reason_deliberative(const Query& query);
    
    /**
     * PREDICTIVE: Forward simulation
     * "What if X happens?"
     */
    ReasoningResult reason_predictive(const Query& query);
    
    /**
     * METACOGNITIVE: Reason about reasoning
     * "How confident am I? What am I uncertain about?"
     */
    ReasoningResult reason_metacognitive(const Query& query);
    
    // ========================================================================
    // GRAPH-BASED REASONING
    // ========================================================================
    
    /**
     * Multi-hop path finding with energy decay
     * Returns paths from source to target concepts
     */
    std::vector<CorticalPath> find_reasoning_paths(
        uint64_t source_node,
        uint64_t target_node,
        int max_hops
    );
    
    /**
     * Analogical reasoning: A:B :: C:?
     * Find D such that relationship(A,B) ≈ relationship(C,D)
     */
    std::vector<uint64_t> reason_by_analogy(
        uint64_t a, uint64_t b, uint64_t c
    );
    
    /**
     * Causal reasoning: What causes X? What does X cause?
     */
    struct CausalChain {
        std::vector<uint64_t> causes;      // What leads to X
        std::vector<uint64_t> effects;     // What X leads to
        float confidence;
    };
    CausalChain find_causal_chain(uint64_t concept_node);
    
    /**
     * Counterfactual reasoning: "What if NOT X?"
     * Find contradictions and alternatives
     */
    std::vector<uint64_t> find_alternatives(uint64_t concept_node);
    
    // ========================================================================
    // TRANSFORMER-LIKE REASONING (Future)
    // ========================================================================
    
    /**
     * Pattern-based completion
     * Given: "fire is hot, ice is ___"
     * Complete based on learned patterns
     */
    std::vector<std::string> complete_pattern(const std::string& pattern);
    
    /**
     * Sequence prediction using temporal patterns
     */
    std::vector<uint64_t> predict_sequence(
        const std::vector<uint64_t>& sequence,
        int num_predictions = 1
    );
    
    // ========================================================================
    // HYBRID FUSION
    // ========================================================================
    
    /**
     * Combine graph and transformer predictions
     * P(next) = α * P_graph(next) + β * P_transformer(next)
     * Graph can veto impossible predictions
     */
    struct HybridPrediction {
        uint64_t node_id;
        float graph_confidence;
        float transformer_confidence;
        float combined_confidence;
        bool graph_vetoed;
    };
    
    std::vector<HybridPrediction> hybrid_predict(
        const std::vector<uint64_t>& context_nodes,
        int top_k = 10
    );
    
    // ========================================================================
    // META-REASONING
    // ========================================================================
    
    /**
     * Estimate confidence in current reasoning
     * Based on path strength, consistency, coverage
     */
    float estimate_confidence(const ReasoningResult& result);
    
    /**
     * Identify knowledge gaps
     * "What do I not know?"
     */
    struct KnowledgeGap {
        std::string missing_concept;
        std::string context;
        float importance;
    };
    std::vector<KnowledgeGap> identify_gaps(const Query& query);
    
    /**
     * Generate questions to fill gaps
     * Active learning / curiosity
     */
    std::vector<std::string> generate_questions(
        const std::vector<KnowledgeGap>& gaps
    );
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct ReasoningStats {
        uint64_t total_queries;
        uint64_t reactive_queries;
        uint64_t deliberative_queries;
        uint64_t predictive_queries;
        double avg_reasoning_time_ms;
        double avg_confidence;
        uint64_t total_paths_explored;
    };
    
    ReasoningStats get_stats() const { return stats_; }
    void print_stats() const;
    
private:
    // ========================================================================
    // INTERNAL REASONING HELPERS
    // ========================================================================
    
    // Parse query into graph nodes
    std::vector<uint64_t> parse_query_to_nodes(const Query& query);
    
    // Generate thought from graph path
    Thought path_to_thought(const CorticalPath& path);
    
    // Combine multiple thoughts into coherent answer
    std::string synthesize_answer(const std::vector<Thought>& thoughts);
    
    // Check consistency between thoughts
    bool are_thoughts_consistent(const Thought& a, const Thought& b);
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    MelvinGraph* graph_;
    Config config_;
    
    // Recent reasoning context
    std::vector<Thought> thought_history_;
    
    // Statistics
    ReasoningStats stats_;
};

} // namespace uca
} // namespace melvin


