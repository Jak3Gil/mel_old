#ifndef UNIFIED_COGNITIVE_SYSTEM_H
#define UNIFIED_COGNITIVE_SYSTEM_H

#include "field_architecture.h"
#include "global_activation_field.h"
#include "hierarchical_temporal_memory.h"
#include "three_channel_feedback.h"
#include "reflective_controller.h"
#include "continuous_reasoning.h"
#include "population_evolution.h"

#include <memory>
#include <atomic>

namespace melvin {
namespace cognitive_field {

/**
 * Unified Cognitive System
 * 
 * Integrates all cognitive fields into a single coherent system:
 * - Asynchronous parallel fields (no sequential pipeline)
 * - Global activation substrate (multi-modal resonance)
 * - Hierarchical temporal memory (no context reset)
 * - Three-channel feedback (sensory, cognitive, evolutionary)
 * - Reflective meta-cognition (thinks about thinking)
 * - Continuous reasoning dynamics (30Hz Hopfield + Energy)
 * - Population-based evolution (stable adaptation)
 * 
 * This replaces the 18-step serial pipeline with a biological cognitive architecture.
 */

class UnifiedCognitiveSystem {
public:
    UnifiedCognitiveSystem();
    ~UnifiedCognitiveSystem();
    
    // ========================================================================
    // System Lifecycle
    // ========================================================================
    
    /**
     * Initialize all subsystems
     */
    void initialize();
    
    /**
     * Start all cognitive fields (async processing begins)
     */
    void start();
    
    /**
     * Stop all cognitive fields
     */
    void stop();
    
    /**
     * Check if system is running
     */
    bool is_running() const;
    
    // ========================================================================
    // Input Interface (All Modalities)
    // ========================================================================
    
    /**
     * Inject sensory input (any modality)
     * @param modality - 0=text, 1=vision, 2=audio, 3=motor, 4=proprioception
     * @param tokens - Token/feature nodes
     * @param embeddings - Semantic embeddings for each token
     * @param energy_per_token - Energy to inject for each token
     */
    void inject_input(int modality,
                     const std::vector<int>& tokens,
                     const std::vector<std::vector<float>>& embeddings,
                     const std::vector<float>& energy_per_token);
    
    /**
     * Convenience wrappers for specific modalities
     */
    void inject_text(const std::vector<int>& word_nodes,
                    const std::vector<std::vector<float>>& embeddings);
    
    void inject_vision(const std::vector<int>& visual_feature_nodes,
                      const std::vector<std::vector<float>>& embeddings);
    
    void inject_audio(const std::vector<int>& audio_feature_nodes,
                     const std::vector<std::vector<float>>& embeddings);
    
    void inject_motor_state(const std::vector<int>& motor_nodes,
                           const std::vector<float>& joint_angles);
    
    // ========================================================================
    // Output Interface
    // ========================================================================
    
    /**
     * Generate output from current reasoning state
     * @param modality - Target output modality
     * @param max_length - Maximum output length
     * @return sequence of node IDs
     */
    std::vector<int> generate_output(int modality, size_t max_length = 50);
    
    /**
     * Generate motor command
     */
    std::vector<float> generate_motor_command(int num_joints);
    
    // ========================================================================
    // Feedback Interface
    // ========================================================================
    
    /**
     * Provide sensory feedback (consequences of actions)
     */
    void provide_sensory_feedback(const SensoryFeedback& feedback);
    
    /**
     * Provide prediction error feedback
     */
    void provide_prediction_error(int predicted_node, int actual_node,
                                  float confidence, float error);
    
    // ========================================================================
    // Query Interface
    // ========================================================================
    
    /**
     * Get current activation state
     */
    std::vector<std::pair<int, float>> get_top_active_nodes(size_t k) const;
    
    /**
     * Get current context vector
     */
    std::vector<float> get_context_vector() const;
    
    /**
     * Get working memory buffer
     */
    std::vector<GlobalActivationField::WorkingConcept> get_working_memory() const;
    
    /**
     * Query recent temporal memory
     */
    HierarchicalMemory::TemporalContext get_recent_context(int lookback_sec) const;
    
    /**
     * Get recent reflections (meta-cognitive insights)
     */
    std::vector<std::string> get_recent_insights(size_t count) const;
    
    // ========================================================================
    // System Health
    // ========================================================================
    
    struct SystemStatus {
        bool is_running;
        float total_energy;
        size_t active_nodes;
        float reasoning_convergence;
        float prediction_accuracy;
        float cognitive_coherence;
        float evolutionary_fitness;
        size_t generation;
        
        // Field tick rates
        float perception_hz;
        float attention_hz;
        float reasoning_hz;
        float memory_hz;
        float output_hz;
        float reflection_hz;
        float evolution_hz;
    };
    
    SystemStatus get_system_status() const;
    
    /**
     * Get detailed statistics from all subsystems
     */
    struct DetailedStats {
        GlobalActivationField::Stats activation_field;
        HierarchicalMemory::Stats hierarchical_memory;
        FeedbackManager::SystemHealth feedback_health;
        ReflectiveController::Stats reflection;
        ContinuousReasoning::Stats reasoning;
        PopulationEvolution::Stats evolution;
    };
    
    DetailedStats get_detailed_stats() const;
    
    // ========================================================================
    // Persistence
    // ========================================================================
    
    void save_state(const std::string& directory) const;
    void load_state(const std::string& directory);
    
    // ========================================================================
    // Direct Access (Advanced)
    // ========================================================================
    
    GlobalActivationField& get_activation_field() { return *activation_field_; }
    HierarchicalMemory& get_hierarchical_memory() { return *hierarchical_memory_; }
    FeedbackManager& get_feedback_manager() { return *feedback_manager_; }
    ReflectiveController& get_reflective_controller() { return *reflective_controller_; }
    ContinuousReasoning& get_continuous_reasoning() { return *continuous_reasoning_; }
    PopulationEvolution& get_population_evolution() { return *population_evolution_; }
    
private:
    // Core subsystems
    std::unique_ptr<GlobalActivationField> activation_field_;
    std::unique_ptr<HierarchicalMemory> hierarchical_memory_;
    std::unique_ptr<FeedbackManager> feedback_manager_;
    std::unique_ptr<ReflectiveController> reflective_controller_;
    std::unique_ptr<ContinuousReasoning> continuous_reasoning_;
    std::unique_ptr<PopulationEvolution> population_evolution_;
    
    // System state
    std::atomic<bool> running_;
    std::chrono::high_resolution_clock::time_point system_start_time_;
    
    // Main processing thread (coordinates all fields)
    std::thread coordinator_thread_;
    void coordinator_loop();
    
    // Integration helpers
    void process_frame();  // Called at master Hz (30Hz)
    void update_attention();
    void update_reasoning();
    void update_memory();
    void check_reflections();
    void check_evolution();
};

} // namespace cognitive_field
} // namespace melvin

#endif // UNIFIED_COGNITIVE_SYSTEM_H

