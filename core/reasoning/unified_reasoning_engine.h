#ifndef UNIFIED_REASONING_ENGINE_H
#define UNIFIED_REASONING_ENGINE_H

#include "spreading_activation.h"
#include "predictor.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <deque>
#include <unordered_set>

namespace melvin {
namespace reasoning {

class UnifiedReasoningEngine {
public:
    explicit UnifiedReasoningEngine(int embedding_dim = 128);
    ~UnifiedReasoningEngine();
    
    // Input processing
    void process_input(
        const std::vector<int>& input_nodes,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        const std::string& modality
    );
    
    // Prediction
    std::vector<PredictionResult> predict_next(
        const std::vector<int>& context_nodes,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        int top_k = 5,
        Predictor::Mode mode = Predictor::Mode::HYBRID
    );
    
    // Generation
    std::vector<int> generate_output(
        const std::vector<int>& prompt_nodes,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        float temperature = 0.7f,
        int max_length = 20
    );
    
    // Multi-hop reasoning
    std::vector<int> multi_hop_query(
        const std::vector<float>& query_embedding,
        const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
        const std::unordered_map<int, std::vector<float>>& embeddings,
        int max_hops = 100
    );
    
    // Shutdown
    void shutdown();
    
    // Accessors
    ActivationField& activation_field() { return activation_field_; }
    Predictor& predictor() { return predictor_; }
    
    // ADAPTIVE INTELLIGENCE: Metrics for self-evaluation
    struct ReasoningMetrics {
        float confidence = 0.5f;      // How confident in reasoning
        float coherence = 0.5f;       // How coherent outputs are
        float novelty = 0.5f;         // How novel/creative
        float entropy = 0.5f;         // Chaos vs order
        float success_rate = 0.5f;    // Recent success rate
        
        float last_confidence = 0.5f;
        float last_coherence = 0.5f;
        float last_novelty = 0.5f;
    };
    
    // HUMAN-LEVEL ADAPTIVE INTELLIGENCE v4.0: World Model
    struct WorldModel {
        // Predicted next states based on current context
        std::vector<int> predicted_next_nodes;
        std::vector<float> prediction_confidence;
        
        // Prediction error tracking
        float recent_prediction_error = 0.5f;
        float avg_prediction_accuracy = 0.5f;
        
        // Causal model: action → outcome
        std::unordered_map<int, std::vector<std::pair<int, float>>> causal_edges;
        
        // Temporal expectations
        std::unordered_map<int, float> expected_activation_time;
        
        // TRUE GENERAL INTELLIGENCE v5.0: Probabilistic Extensions
        // Full probability distributions over next states
        std::unordered_map<int, float> state_distribution;
        float prediction_uncertainty = 0.5f;          // Entropy of distribution
        float expected_information_gain = 0.0f;       // For uncertainty-driven attention
        
        // Counterfactual imagination - simulated alternative futures
        struct CounterfactualScenario {
            std::vector<int> trajectory;
            float probability;
            float predicted_value;
        };
        std::vector<CounterfactualScenario> imagined_futures;
    };
    
    WorldModel& get_world_model() { return world_model_; }
    const WorldModel& get_world_model() const { return world_model_; }
    
    // Predict what should come next
    void predict_future_state(const std::vector<int>& current_context);
    
    // Update model based on prediction error
    void update_world_model(const std::vector<int>& actual_next, float learning_rate = 0.1f);
    
    // TRUE GENERAL INTELLIGENCE v5.0: Probabilistic reasoning
    void compute_state_distribution(const std::vector<int>& context);
    void simulate_counterfactual_futures(const std::vector<int>& context, int num_simulations = 10);
    float compute_prediction_uncertainty();
    
    ReasoningMetrics& get_metrics() { return metrics_; }
    const ReasoningMetrics& get_metrics() const { return metrics_; }
    
    // Update metrics after reasoning
    void update_metrics(float confidence, float coherence, float novelty);
    
    // ============================================================================
    // HUMAN-LIKE REASONING MECHANISMS (13 core systems)
    // ============================================================================
    
    // 1. GOAL-DRIVEN REASONING
    struct Goal {
        int target_node_id;
        std::vector<float> target_embedding;
        float priority;
        float relevance_threshold;
        std::string description;
        int parent_goal_index = -1;
    };
    
    struct GoalStack {
        std::vector<Goal> goals;
        int active_goal_index = -1;
    };
    
    // 2. CAUSAL REASONING
    enum class CausalType {
        CAUSES,      // A causes B
        ENABLES,     // A enables B
        INHIBITS,    // A inhibits B
        CORRELATES   // A correlates with B (no causal direction)
    };
    
    struct CausalEdge {
        int source;
        int target;
        CausalType type;
        float strength;
        float confidence;
    };
    
    // 3. WORKING MEMORY SCRATCHPAD
    struct WorkingMemorySlot {
        std::string variable_name;
        int node_id;
        float value;
        float confidence;
        int creation_tick;
        int last_access_tick;
    };
    
    struct WorkingMemory {
        std::vector<WorkingMemorySlot> slots;
        int max_slots = 20;
        int current_tick = 0;
        float decay_rate = 0.95f;
    };
    
    // 4. CONTEXT VECTOR
    struct ContextState {
        std::vector<float> context_vector;
        std::vector<int> recent_perception;
        std::vector<int> active_goals;
        float emotional_tone = 0.5f;
        int window_size = 20;
        std::deque<std::vector<float>> context_history;
    };
    
    // 5. UNCERTAINTY TRACKING
    struct BeliefState {
        std::unordered_map<int, float> mean_activation;  // Expected value
        std::unordered_map<int, float> variance;         // Uncertainty
        std::unordered_map<int, float> confidence;       // Belief confidence
    };
    
    // 6. INTERNAL DIALOGUE
    struct ReasoningStream {
        std::vector<int> hypothesis;
        float confidence;
        std::vector<std::string> justifications;
        float evidence_strength;
    };
    
    struct InternalDialogue {
        ReasoningStream proponent;
        ReasoningStream skeptic;
        std::vector<std::string> debate_history;
        int winner_stream = -1;  // 0=proponent, 1=skeptic
    };
    
    // 7. COHERENCE CHECKING
    struct Contradiction {
        int edge1_source, edge1_target;
        int edge2_source, edge2_target;
        float conflict_strength;
        std::string description;
    };
    
    // 8. META-REASONING (System 1 vs System 2)
    enum class ThinkingMode {
        FAST_ASSOCIATIVE,     // System 1: Quick, intuitive
        SLOW_ANALYTICAL,      // System 2: Deliberate, logical
        HYBRID                // Mixed mode
    };
    
    struct MetaReasoning {
        ThinkingMode current_mode = ThinkingMode::FAST_ASSOCIATIVE;
        float task_novelty = 0.0f;
        float confidence_in_mode = 0.5f;
        int cycles_in_mode = 0;
    };
    
    // 9. SCHEMA LEARNING
    struct Schema {
        int schema_node_id;
        std::vector<int> pattern_nodes;
        std::vector<std::pair<int,int>> pattern_edges;
        float activation_count = 0.0f;
        float reliability = 0.5f;
    };
    
    // 10. REFLECTIVE LEARNING
    struct ReasoningEpisode {
        std::vector<int> goal_stack;
        ThinkingMode strategy_used;
        float outcome_quality;
        float confidence_before;
        float confidence_after;
        float time_cost;
        int tick_count;
    };
    
    // 11. TEMPORAL AWARENESS
    enum class TemporalRelation {
        BEFORE, AFTER, DURING, UNTIL, SINCE, OVERLAP
    };
    
    struct TemporalEvent {
        int node_id;
        int timestamp;
        std::unordered_map<int, TemporalRelation> relations_to_other_events;
    };
    
    // 12. NARRATIVE MEMORY
    struct NarrativeEpisode {
        std::vector<int> reasoning_sequence;
        std::string summary;
        int start_tick;
        int end_tick;
        float coherence_score;
    };
    
    // 13. REASONING EVALUATION
    struct ReasoningEvaluation {
        float coherence;            // Semantic consistency
        float justification_depth;  // Number of causal links
        float efficiency;           // Goal progress / compute
        float introspective_accuracy; // Predicted vs actual success
        int reasoning_cycles;
        float goal_progress;
    };
    
    // Accessors for new systems
    GoalStack& get_goal_stack() { return goal_stack_; }
    WorkingMemory& get_working_memory() { return working_memory_; }
    ContextState& get_context_state() { return context_state_; }
    BeliefState& get_belief_state() { return belief_state_; }
    MetaReasoning& get_meta_reasoning() { return meta_reasoning_; }
    
    // Core reasoning methods
    void push_goal(const Goal& goal);
    void pop_goal();
    float compute_relevance_to_goal(int node_id, const std::unordered_map<int, std::vector<float>>& embeddings);
    bool should_stop_inference();
    
    void add_causal_edge(int source, int target, CausalType type, float strength);
    std::vector<int> simulate_causal_intervention(int intervention_node, bool enable);
    
    void update_working_memory(const std::string& var, int node_id, float value, float confidence);
    void decay_working_memory();
    
    void update_context_vector(const std::vector<int>& perception, const std::unordered_map<int, std::vector<float>>& embeddings);
    float context_relevance(int node_id, const std::unordered_map<int, std::vector<float>>& embeddings);
    
    void update_belief_bayesian(int node_id, float evidence_strength, bool positive);
    
    void start_internal_dialogue(const std::vector<int>& hypothesis);
    int resolve_dialogue();
    
    std::vector<Contradiction> detect_contradictions();
    void resolve_contradiction(const Contradiction& c);
    
    void select_thinking_mode(float task_novelty, float current_confidence);
    
    void learn_schema_from_pattern(const std::vector<int>& pattern);
    
    void record_reasoning_episode(const ReasoningEpisode& episode);
    
    void add_temporal_event(int node_id, int timestamp);
    void add_temporal_relation(int event1, int event2, TemporalRelation relation);
    
    void record_narrative_episode(const std::vector<int>& sequence, const std::string& summary);
    
    ReasoningEvaluation evaluate_reasoning_cycle(int cycle_start_tick);
    
private:
    int embedding_dim_;
    ActivationField activation_field_;
    Predictor predictor_;
    
    int reasoning_session_count_;
    std::chrono::time_point<std::chrono::high_resolution_clock> last_activity_;
    
    // ADAPTIVE: Track reasoning quality
    ReasoningMetrics metrics_;
    
    // HUMAN-LEVEL v4.0: Predictive world model
    WorldModel world_model_;
    
    // HUMAN-LIKE REASONING: 13 core systems
    GoalStack goal_stack_;
    std::vector<CausalEdge> causal_graph_;
    WorkingMemory working_memory_;
    ContextState context_state_;
    BeliefState belief_state_;
    InternalDialogue dialogue_;
    std::vector<Contradiction> detected_contradictions_;
    MetaReasoning meta_reasoning_;
    std::vector<Schema> learned_schemas_;
    std::vector<ReasoningEpisode> reasoning_history_;
    std::vector<TemporalEvent> temporal_timeline_;
    std::vector<NarrativeEpisode> narrative_memory_;
    ReasoningEvaluation current_evaluation_;
    
    int global_tick_ = 0;
};

} // namespace reasoning
} // namespace melvin

#endif // UNIFIED_REASONING_ENGINE_H
