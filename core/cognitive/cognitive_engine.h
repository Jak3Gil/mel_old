/**
 * cognitive_engine.h
 * 
 * MELVIN's Unified Cognitive Architecture
 * 
 * This is the CORE of MELVIN - all inputs flow through this system
 * regardless of modality (text, audio, vision).
 * 
 * Implements all 6 human cognitive mechanisms:
 * 1. Quality Evaluation
 * 2. Boredom / Habituation
 * 3. Exploration vs Exploitation
 * 4. Goal-Directed Processing
 * 5. Salience / Importance Tracking
 * 6. Stochastic Processing
 */

#pragma once

#include "../reasoning/unified_reasoning_engine.h"
#include "../reasoning/consolidation.h"
#include "../graph_storage.h"
#include "../evolution/genome.h"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <random>

namespace melvin {
namespace cognitive {


/**
 * Core cognitive mechanisms that apply to ALL processing
 */
class CognitiveEngine {
public:
    /**
     * Processing modes - what is the system trying to do?
     */
    enum class Goal {
        UNDERSTAND,     // Comprehend input
        CREATE,         // Generate novel output
        LEARN,          // Focus on learning patterns
        EXPLORE,        // Seek novelty
        CONNECT,        // Find relationships
        REASON,         // Logical inference
        LISTEN,         // Audio focus
        RECOGNIZE       // Vision focus
    };
    
    /**
     * Quality metrics for any input
     */
    struct QualityScore {
        float coherence = 0.5f;      // How well-formed is it?
        float relevance = 0.5f;       // How relevant to current goal?
        float novelty = 0.5f;         // How new/interesting?
        
        float overall() const {
            return (coherence + relevance + novelty) / 3.0f;
        }
    };
    
    /**
     * ADAPTIVE INTELLIGENCE: Metrics for adaptation quality
     */
    struct AdaptationMetrics {
        float coherence_gain = 0.0f;       // How much coherence improved
        float novelty_gain = 0.0f;         // How much novelty increased
        float goal_success = 0.5f;         // Goal achievement rate
        float energy_cost = 1.0f;          // Energy expended
        float aqs = 0.0f;                  // Adaptation Quality Score
        
        // History tracking
        float last_coherence = 0.5f;
        float last_novelty = 0.5f;
        int cycles_since_update = 0;
        
        // Running averages (for genome phase updates)
        float avg_success_rate = 0.5f;
        float avg_surprise_rate = 0.5f;
    };
    
    /**
     * HUMAN-LEVEL v4.0: Self-Model (awareness of internal state)
     */
    struct SelfModel {
        float confidence = 0.5f;           // Belief in own judgments
        float emotion_valence = 0.5f;      // Positive/negative feeling
        float curiosity_level = 0.5f;      // Drive to explore
        float energy_budget = 1.0f;        // Available cognitive resources
        float social_alignment = 0.5f;     // Alignment with human values
        float arousal = 0.5f;              // Activation level
        float focus = 0.5f;                // Attention narrowness
    };
    
    /**
     * HUMAN-LEVEL v4.0: Intrinsic Motivation
     */
    struct IntrinsicMotivation {
        float curiosity = 0.3f;            // Novelty seeking
        float stability = 0.3f;            // Coherence seeking
        float empathy = 0.2f;              // Social alignment seeking
        float intrinsic_reward = 0.0f;     // Combined reward signal
        float task_reward = 0.0f;          // External reward
        float meta_value = 0.0f;           // Total value estimate
    };
    
    /**
     * HUMAN-LEVEL v4.0: Global Workspace (conscious coordination)
     */
    struct GlobalWorkspace {
        std::vector<int> focus_nodes;      // Currently "conscious" nodes
        std::unordered_map<int, float> salience_map;
        int broadcast_interval_ms = 100;
        int last_broadcast_time = 0;
        bool is_active = true;
    };
    
    /**
     * HUMAN-LEVEL v4.0: Multi-Timescale Memory
     */
    struct MultiTimescaleMemory {
        // Short-term (seconds) - fast learning
        std::vector<std::vector<int>> short_term_episodes;
        float short_term_learning_rate = 0.3f;
        int short_term_capacity = 20;
        
        // Mid-term (minutes) - moderate learning
        std::vector<std::vector<int>> mid_term_episodes;
        float mid_term_learning_rate = 0.1f;
        int mid_term_capacity = 100;
        
        // Long-term (hours+) - slow consolidation
        float long_term_learning_rate = 0.01f;
        int consolidation_counter = 0;
    };
    
    // =========================================================================
    // TRUE GENERAL INTELLIGENCE v5.0: Living, Feeling, Embodied Systems
    // =========================================================================
    
    /**
     * v5.0: Embodied Grounding - Physical body state
     */
    struct EmbodiedState {
        float physical_energy = 1.0f;         // Physical stamina
        float fatigue = 0.0f;                 // Accumulated tiredness
        float tension = 0.0f;                 // Physical stress
        float proprioceptive_noise = 0.1f;    // Sensory uncertainty
        float action_latency_ms = 100.0f;     // Response delay
        float effort_cost_multiplier = 1.0f;  // How much actions cost
    };
    
    /**
     * v5.0: Dynamic Drives - Evolving motivations with homeostasis
     */
    struct DynamicDrives {
        float curiosity_drive = 0.5f;         // Exploration need
        float comfort_drive = 0.5f;           // Safety/stability need
        float affiliation_drive = 0.5f;       // Social connection need
        float dominance_drive = 0.3f;         // Competence/mastery need
        float competence_drive = 0.5f;        // Skill development need
        
        // Homeostatic targets
        float curiosity_target = 0.5f;
        float comfort_target = 0.6f;
        float affiliation_target = 0.4f;
        
        // Drive satisfaction levels
        float curiosity_satisfaction = 0.5f;
        float comfort_satisfaction = 0.5f;
        float affiliation_satisfaction = 0.5f;
    };
    
    /**
     * v5.0: Narrative Memory - Story arcs and temporal identity
     */
    struct NarrativeMemory {
        struct Event {
            std::vector<int> nodes;
            float emotional_valence;
            float importance;
            int timestamp;
        };
        
        struct Episode {
            std::vector<Event> events;
            std::string summary;
            float coherence;
        };
        
        struct NarrativeArc {
            std::vector<Episode> episodes;
            std::string theme;
            float completion;
        };
        
        std::vector<NarrativeArc> life_story;
        NarrativeArc current_arc;
        int narrative_timestamp = 0;
    };
    
    /**
     * v5.0: Social Cognition - Model other agents
     */
    struct AgentModel {
        std::string agent_id;
        std::vector<int> believed_goals;
        float predicted_emotion = 0.5f;
        float trust_level = 0.5f;
        float reliability = 0.5f;
        std::unordered_map<int, float> belief_state;  // What they know
    };
    
    struct SocialCognition {
        std::unordered_map<std::string, AgentModel> other_agents;
        float empathy_level = 0.5f;
        float social_prediction_accuracy = 0.5f;
        std::string cultural_context = "default";
    };
    
    /**
     * v5.0: Enhanced Emotions - Full hormonal dynamics
     */
    struct HormonalState {
        // Existing (from v4.0)
        float dopamine = 0.0f;
        float serotonin = 0.0f;
        float noradrenaline = 0.0f;
        float acetylcholine = 0.0f;
        
        // NEW v5.0
        float cortisol = 0.0f;                // Stress hormone
        float oxytocin = 0.0f;                // Social bonding hormone
        float endorphins = 0.0f;              // Pain/pleasure modulation
        
        // Temporal dynamics (inertia)
        float dopamine_decay = 0.9f;
        float serotonin_decay = 0.95f;
        float cortisol_decay = 0.9f;
        float oxytocin_decay = 0.9f;
    };
    
    /**
     * v5.0: Conceptual Creativity - Structural analogy
     */
    struct AnalogicalMapping {
        std::vector<int> source_structure;
        std::vector<int> target_structure;
        float structural_similarity;
        float predictive_utility;
        bool is_valid = false;
    };
    
    struct CreativeSystem {
        std::vector<AnalogicalMapping> known_analogies;
        std::unordered_map<std::string, std::vector<int>> metaconcepts;
        int analogies_discovered = 0;
    };
    
    /**
     * v5.0: Metacognitive Reflection - Thinking about thinking
     */
    struct MetacognitiveState {
        std::string current_cognitive_mode = "reasoning";  // reasoning, imagining, reflecting...
        std::vector<std::string> alternative_strategies;
        float strategy_effectiveness = 0.5f;
        int reflection_depth = 0;  // How many levels deep in meta-thinking
    };
    
    /**
     * v5.0: Dream State - Associative recombination
     */
    struct DreamState {
        bool is_dreaming = false;
        std::vector<std::vector<int>> dream_associations;
        std::vector<std::pair<int, int>> novel_connections;  // Discovered during dreams
        int dreams_completed = 0;
    };
    
    /**
     * v5.0: Cultural/Linguistic Layer
     */
    struct CulturalKnowledge {
        std::unordered_map<std::string, std::vector<int>> shared_lexicon;  // word → concept
        std::unordered_map<int, std::string> concept_to_word;              // concept → word
        std::vector<std::string> cultural_norms;
        float linguistic_alignment = 0.5f;
    };
    
    /**
     * v5.0: Ethical Alignment
     */
    struct EthicalCore {
        float harm_aversion = 0.8f;           // Avoid causing harm
        float fairness_weight = 0.6f;         // Value equity
        float autonomy_respect = 0.7f;        // Respect agency
        float consistency_drive = 0.8f;       // Be coherent with stated values
        
        std::vector<std::string> core_values;
        std::vector<std::string> safety_constraints;
        float ethical_coherence = 0.5f;
    };
    
    /**
     * v5.0: Phenomenological Layer - Qualia approximation
     */
    struct PhenomenologicalState {
        std::vector<float> experience_manifold;  // High-dim internal state
        float felt_similarity = 0.0f;            // Distance in experience space
        float continuity_of_self = 1.0f;         // Sense of persistence
        float presence = 0.5f;                   // "Being here now"
        float depth_of_processing = 0.5f;        // How deeply experiencing
    };
    
    /**
     * Cognitive state
     */
    struct CognitiveState {
        Goal current_goal = Goal::UNDERSTAND;
        int goal_duration = 10;
        
        float exploration_rate = 0.3f;  // 0 = pure exploit, 1 = pure explore
        float quality_threshold = 0.3f;
        int boredom_threshold = 3;
        
        // Adaptive statistics
        int high_quality_count = 0;
        int rejected_count = 0;
        int bored_count = 0;
        int total_processed = 0;
    };
    
    CognitiveEngine(GraphStorage& g, reasoning::UnifiedReasoningEngine& e, evolution::Genome* genome = nullptr);
    ~CognitiveEngine() = default;
    
    // ========================================================================
    // INPUT PROCESSING (works for any modality)
    // ========================================================================
    
    /**
     * Process input nodes through full cognitive pipeline
     * 
     * @param nodes The input nodes (text tokens, audio formants, visual features, etc.)
     * @param modality String describing the modality for logging
     * @return True if processing succeeded
     */
    bool process_input(const std::vector<int>& nodes, const std::string& modality = "input");
    
    /**
     * Evaluate quality of a sequence
     * Mechanism 1: Quality Evaluation
     */
    QualityScore evaluate_quality(const std::vector<int>& nodes);
    
    // ========================================================================
    // THINKING & GENERATION
    // ========================================================================
    
    /**
     * Generate internal thought (recursive thinking)
     * Uses current activation state to generate new thoughts
     */
    std::vector<int> think();
    
    /**
     * Generate response/output
     * Similar to think() but biased toward current goal
     */
    std::vector<int> generate_response(int max_length = 20);
    
    /**
     * Predict what should come next
     */
    std::vector<reasoning::PredictionResult> predict_next(
        const std::vector<int>& context,
        int top_k = 5
    );
    
    // ========================================================================
    // LEARNING & CONSOLIDATION
    // ========================================================================
    
    /**
     * Consolidate recent experiences
     * Mechanism: Hebbian learning + quality weighting
     */
    void consolidate();
    
    /**
     * Update salience for concepts
     * Mechanism 5: Salience tracking
     */
    void update_salience(const std::vector<int>& nodes, float quality);
    
    // ========================================================================
    // COGNITIVE MECHANISMS
    // ========================================================================
    
    /**
     * Check for boredom/habituation
     * Mechanism 2: Returns true if thought is repetitive
     */
    bool is_bored_of(const std::vector<int>& thought);
    
    /**
     * Decide: explore or exploit?
     * Mechanism 3: Returns true for explore
     */
    bool should_explore();
    
    /**
     * Update current goal
     * Mechanism 4: Goal-directed processing
     */
    void update_goal();
    void set_goal(Goal g, int duration = 10);
    
    /**
     * Get stochastic temperature
     * Mechanism 6: Returns variable temperature
     */
    float get_temperature();
    
    // ========================================================================
    // NODE SELECTION (quality + salience weighted)
    // ========================================================================
    
    /**
     * Select nodes based on quality, salience, goal, and exploration
     */
    std::vector<int> select_quality_nodes(int count = 5);
    
    /**
     * Score a node for selection
     */
    float score_node(int node_id, bool exploring);
    
    // ========================================================================
    // STATE & STATISTICS
    // ========================================================================
    
    const CognitiveState& get_state() const { return state; }
    CognitiveState& get_state() { return state; }
    
    void print_statistics() const;
    
    // ========================================================================
    // ADAPTIVE INTELLIGENCE: AQS & Evaluation
    // ========================================================================
    
    /**
     * Get current adaptation metrics
     */
    const AdaptationMetrics& get_adaptation_metrics() const { return adaptation_metrics_; }
    
    /**
     * Compute Adaptation Quality Score
     */
    float compute_aqs();
    
    /**
     * Evaluate and adjust parameters based on adaptation quality
     * Called every few cycles to assess if we're adapting well or poorly
     */
    void evaluate_adaptation();
    
    // ========================================================================
    // HUMAN-LEVEL v4.0: Advanced Intelligence Systems
    // ========================================================================
    
    /**
     * Get self-model (awareness of internal state)
     */
    const SelfModel& get_self_model() const { return self_model_; }
    SelfModel& get_self_model() { return self_model_; }
    
    /**
     * Update self-model based on recent experience
     */
    void update_self_model();
    
    /**
     * Compute intrinsic motivation and rewards
     */
    void compute_intrinsic_motivation();
    
    /**
     * Compute differentiable value function
     */
    float compute_value_function();
    
    /**
     * Global workspace broadcast (conscious coordination)
     */
    void broadcast_global_workspace();
    
    /**
     * Multi-timescale memory management
     */
    void update_multi_timescale_memory(const std::vector<int>& episode);
    void consolidate_timescale_memories();
    
    /**
     * Sleep/replay cycle for offline consolidation
     */
    void sleep_and_replay();
    
    /**
     * Symbolic abstraction (concept formation)
     */
    void form_symbolic_abstractions();
    
    /**
     * Creative compression (synthesize new concepts)
     */
    std::vector<int> creative_compress(const std::vector<std::vector<int>>& patterns);
    
    /**
     * Social learning (import external knowledge)
     */
    void import_social_trace(const std::vector<int>& trace, float trust_score);
    
    /**
     * Embodiment: action-consequence learning
     */
    void learn_action_consequence(int action_node, const std::vector<int>& outcomes);
    
    // ========================================================================
    // TRUE GENERAL INTELLIGENCE v5.0: Advanced Methods
    // ========================================================================
    
    /**
     * Probabilistic imagination - simulate alternative futures
     */
    void imagine_counterfactuals(int num_scenarios = 10);
    
    /**
     * Update embodied state based on actions and context
     */
    void update_embodied_state(float action_cost);
    
    /**
     * Homeostatic drive regulation
     */
    void update_dynamic_drives();
    
    /**
     * Add event to narrative memory
     */
    void record_narrative_event(const std::vector<int>& nodes, float emotional_valence, float importance);
    void consolidate_narrative();
    
    /**
     * Model other agent's mental state
     */
    void update_agent_model(const std::string& agent_id, const std::vector<int>& observed_behavior);
    float predict_agent_action(const std::string& agent_id);
    
    /**
     * Update full hormonal dynamics with temporal inertia
     */
    void update_hormonal_state();
    
    /**
     * Discover structural analogies between concept clusters
     */
    void discover_analogies();
    bool evaluate_analogy(const AnalogicalMapping& mapping);
    
    /**
     * Metacognitive reflection - evaluate own thinking process
     */
    void reflect_on_cognition();
    void simulate_alternative_strategy(const std::string& strategy);
    
    /**
     * Enhanced dreaming with creative recombination
     */
    void dream_creative_associations();
    
    /**
     * Linguistic grounding
     */
    void align_concept_to_word(int concept_id, const std::string& word);
    std::string concept_to_language(int concept_id);
    std::vector<int> language_to_concepts(const std::string& text);
    
    /**
     * Ethical evaluation of planned actions
     */
    float evaluate_ethical_alignment(const std::vector<int>& planned_action);
    bool passes_safety_constraints(const std::vector<int>& action);
    
    /**
     * Update phenomenological experience manifold
     */
    void update_phenomenology();
    float compute_felt_similarity(const std::vector<int>& state_a, const std::vector<int>& state_b);
    
    /**
     * Uncertainty-driven attention (Bayesian)
     */
    std::vector<int> select_nodes_by_information_gain(int count = 5);
    
    // Accessors for v5.0 systems
    const EmbodiedState& get_embodied_state() const { return embodied_state_; }
    const DynamicDrives& get_dynamic_drives() const { return dynamic_drives_; }
    const NarrativeMemory& get_narrative_memory() const { return narrative_memory_; }
    const SocialCognition& get_social_cognition() const { return social_cognition_; }
    const HormonalState& get_hormonal_state() const { return hormonal_state_; }
    const CreativeSystem& get_creative_system() const { return creative_system_; }
    const MetacognitiveState& get_metacognitive_state() const { return metacognitive_state_; }
    const DreamState& get_dream_state() const { return dream_state_; }
    const CulturalKnowledge& get_cultural_knowledge() const { return cultural_knowledge_; }
    const EthicalCore& get_ethical_core() const { return ethical_core_; }
    const PhenomenologicalState& get_phenomenology() const { return phenomenological_state_; }
    
    // ========================================================================
    // UTILITIES
    // ========================================================================
    
    std::string nodes_to_string(const std::vector<int>& nodes, int max_len = 100) const;
    std::string goal_to_string(Goal g) const;
    
private:
    GraphStorage& graph;
    reasoning::UnifiedReasoningEngine& engine;
    reasoning::Consolidator consolidator;
    evolution::Genome* genome_;  // Optional: can be nullptr for manual control
    
    CognitiveState state;
    std::mt19937 gen;
    
    // Tracking for mechanisms
    std::unordered_map<size_t, int> thought_history;  // Mechanism 2: boredom
    std::unordered_map<int, float> concept_salience;  // Mechanism 5: salience
    std::vector<std::vector<int>> recent_episodes;    // For consolidation
    
    // ADAPTIVE INTELLIGENCE: Adaptation quality tracking
    AdaptationMetrics adaptation_metrics_;
    int total_thinking_cycles_ = 0;
    
    // HUMAN-LEVEL v4.0: Advanced systems
    SelfModel self_model_;
    IntrinsicMotivation intrinsic_motivation_;
    GlobalWorkspace global_workspace_;
    MultiTimescaleMemory multi_timescale_memory_;
    
    // Symbolic abstractions (formed concepts)
    std::unordered_map<std::string, int> symbolic_concepts_;  // concept_name → node_id
    std::unordered_map<int, std::vector<int>> concept_members_;  // node_id → member nodes
    
    // Social learning trust scores
    std::unordered_map<std::string, float> social_trust_;  // source → trust
    
    // Sleep/replay state
    int cycles_since_sleep_ = 0;
    int sleep_interval_ = 500;  // Cycles between sleep
    bool is_sleeping_ = false;
    
    // TRUE GENERAL INTELLIGENCE v5.0: All living systems
    EmbodiedState embodied_state_;
    DynamicDrives dynamic_drives_;
    NarrativeMemory narrative_memory_;
    SocialCognition social_cognition_;
    HormonalState hormonal_state_;
    CreativeSystem creative_system_;
    MetacognitiveState metacognitive_state_;
    DreamState dream_state_;
    CulturalKnowledge cultural_knowledge_;
    EthicalCore ethical_core_;
    PhenomenologicalState phenomenological_state_;
    
    // Helpers
    size_t hash_sequence(const std::vector<int>& seq);
    float score_token_quality(const std::string& token);
    void adapt_parameters(float quality);
    void sync_from_genome();  // Load parameters from genome
    void sync_to_genome();    // Save learned parameters back to genome
};

} // namespace cognitive
} // namespace melvin

