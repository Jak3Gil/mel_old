#ifndef CONTINUOUS_MIND_H
#define CONTINUOUS_MIND_H

#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <deque>
#include "../fields/activation_field_unified.h"
#include "../feedback/three_channel_feedback.h"
#include "../metacognition/reflective_controller.h"
#include "../evolution/dynamic_genome.h"
#include "../cognitive/turn_taking_controller.h"
#include "../cognitive/emotional_modulator.h"
#include "../cognitive/conversation_goal_stack.h"
#include "../audio/audio_graph_layer.h"

namespace melvin {
namespace orchestrator {

/**
 * @brief Continuous Mind - Replaces 18-Step Serial Pipeline
 * 
 * Runs all cognitive processes as parallel asynchronous fields:
 * - Perception (Vision, Audio, Text, Motor) → Unified Field
 * - Attention → Continuous bias on field activations
 * - Reasoning → Hopfield + Spreading activation
 * - Output → Energy-driven generation
 * - Feedback → 3-channel system
 * - Reflection → Meta-cognitive monitoring
 * - Evolution → Population-based genome optimization
 * 
 * Each subsystem runs at its own natural frequency:
 * - Perception: 30-60 Hz (sensory bandwidth)
 * - Attention: 10-20 Hz (PFC-like)
 * - Reasoning: 10-30 Hz (cortical rhythms)
 * - Output: Variable (action-dependent)
 * - Feedback: 1-30 Hz (multi-channel)
 * - Reflection: 0.1-1 Hz (slow deliberation)
 * - Evolution: 0.001-0.01 Hz (generational)
 */

class ContinuousMind {
public:
    ContinuousMind();
    ~ContinuousMind();
    
    // Start/stop the mind
    void start();
    void stop();
    bool is_running() const { return running_.load(); }
    
    // Main loop (runs in background thread)
    void run();
    
    // Inject sensory input
    void inject_vision(const std::vector<float>& visual_features);
    void inject_audio(const std::vector<float>& audio_features);
    void inject_text(const std::string& text);
    void inject_motor_feedback(int motor_id, float position, float reward);
    
    // Audio learning (Phase 6)
    void learn_from_user_speech(
        const std::string& transcribed_text,
        const std::vector<float>& whisper_embedding,
        const std::vector<float>& mel_features
    );
    void learn_from_own_speech(
        const std::string& spoken_text,
        const std::vector<float>& mel_features
    );
    
    // Get output
    std::vector<int> get_active_concepts();
    std::string generate_text_output(int max_tokens = 50);
    std::vector<float> generate_audio_output(float duration_sec = 1.0f);
    std::vector<float> generate_motor_command(int motor_id);
    
    // Conversational methods
    std::string generate_conversational_response(
        const std::vector<int>& context_nodes,
        float confidence,
        float novelty,
        float coherence
    );
    bool should_speak() const;
    bool should_listen() const;
    std::string get_current_topic() const;
    
    // Access subsystems
    fields::UnifiedActivationField& get_field() { return field_; }
    feedback::FeedbackCoordinator& get_feedback() { return feedback_; }
    metacognition::ReflectiveController& get_reflection() { return reflection_; }
    cognitive::TurnTakingController& get_turn_taking() { return turn_taking_; }
    cognitive::EmotionalModulator& get_emotion() { return emotion_modulator_; }
    cognitive::ConversationGoalStack& get_goals() { return goal_stack_; }
    audio::AudioGraphLayer& get_audio_layer() { return audio_layer_; }
    
    // Set operational mode
    enum class Mode {
        LISTENING,   // High sensory input, low output
        THINKING,    // High internal reasoning, moderate I/O
        ACTING,      // High output, low reflection
        SLEEPING     // Consolidation only, no I/O
    };
    void set_mode(Mode mode);
    Mode get_mode() const { return mode_; }
    
    // Get statistics
    struct Stats {
        size_t total_nodes;
        size_t total_edges;
        size_t active_nodes;
        float total_energy;
        float coherence;
        float avg_prediction_error;
        float avg_reward;
        int evolution_generation;
        size_t reflections_count;
        Mode current_mode;
        float tick_rate;  // Actual Hz
        
        // Conversational stats
        bool is_speaking;
        bool is_listening;
        std::string current_topic;
        int active_goals;
        float emotional_confidence;
        float emotional_novelty;
        
        // Audio learning stats
        size_t audio_nodes_learned;
        size_t audio_semantic_links;
        float audio_learning_confidence;
        bool can_self_generate_audio;
    };
    Stats get_stats() const;
    
    // Save/load entire mind state
    void save_state(const std::string& path);
    void load_state(const std::string& path);
    
private:
    // Core subsystems
    fields::UnifiedActivationField field_;
    feedback::FeedbackCoordinator feedback_;
    metacognition::ReflectiveController reflection_;
    evolution::DynamicGenome genome_;  // Genome for parameter evolution
    
    // Conversational subsystems (Phase 2-4)
    cognitive::TurnTakingController turn_taking_;
    cognitive::EmotionalModulator emotion_modulator_;
    cognitive::ConversationGoalStack goal_stack_;
    
    // Audio learning subsystem (Phase 6 - Audio Learning)
    audio::AudioGraphLayer audio_layer_;
    
    // Thread control
    std::atomic<bool> running_;
    std::unique_ptr<std::thread> mind_thread_;
    
    // Operational mode
    std::atomic<Mode> mode_;
    
    // Timing
    std::chrono::high_resolution_clock::time_point last_tick_;
    float target_tick_rate_ = 30.0f;  // 30 Hz default
    
    // Subsystem tick rates (Hz)
    float perception_rate_ = 30.0f;
    float attention_rate_ = 10.0f;
    float reasoning_rate_ = 20.0f;
    float output_rate_ = 10.0f;
    float feedback_rate_ = 10.0f;
    float reflection_rate_ = 1.0f;
    float evolution_rate_ = 0.01f;
    
    // Last tick times for each subsystem
    std::chrono::high_resolution_clock::time_point last_perception_tick_;
    std::chrono::high_resolution_clock::time_point last_attention_tick_;
    std::chrono::high_resolution_clock::time_point last_reasoning_tick_;
    std::chrono::high_resolution_clock::time_point last_output_tick_;
    std::chrono::high_resolution_clock::time_point last_feedback_tick_;
    std::chrono::high_resolution_clock::time_point last_reflection_tick_;
    std::chrono::high_resolution_clock::time_point last_evolution_tick_;
    
    // Input/Output structures (forward declarations)
    struct SensoryInput {
        enum class Type { VISION, AUDIO, TEXT, MOTOR } type;
        std::vector<float> data;
        std::string text_data;
        int motor_id;
        std::chrono::high_resolution_clock::time_point timestamp;
    };
    
    struct Output {
        enum class Type { TEXT, AUDIO, MOTOR } type;
        std::vector<int> concept_ids;
        std::string text;
        std::vector<float> audio;
        int motor_id;
        std::vector<float> motor_command;
        std::chrono::high_resolution_clock::time_point timestamp;
        float confidence;
        float novelty;
    };
    
    // Subsystem update methods
    void update_perception(float dt);
    void update_attention(float dt);
    void update_reasoning(float dt);
    void update_output(float dt);
    void update_feedback(float dt);
    void update_reflection(float dt);
    void update_evolution(float dt);
    
    // Conversational methods (Phase 1-4)
    void update_turn_taking(float dt);
    void update_self_monitoring(float dt);
    void process_output_feedback(const Output& output);
    
    // Mode-specific behavior
    void configure_for_mode();
    
    // Input buffers (thread-safe queues)
    std::vector<SensoryInput> input_buffer_;
    std::mutex input_mutex_;
    
    // Output buffer
    std::vector<Output> output_buffer_;
    std::deque<Output> output_history_;  // For self-monitoring
    std::mutex output_mutex_;
    
    // Self-monitoring
    int last_predicted_output_;
    float self_monitoring_gain_;  // Corollary discharge attenuation
    
    // Helpers
    void process_input_buffer();
    void generate_outputs();
    
    // Statistics tracking
    mutable std::mutex stats_mutex_;
    size_t total_nodes_ = 0;
    size_t total_edges_ = 0;
    float actual_tick_rate_ = 0.0f;
    
    // Conversational state
    float last_confidence_ = 0.5f;
    float last_novelty_ = 0.5f;
    float last_coherence_ = 0.5f;
};

} // namespace orchestrator
} // namespace melvin

#endif // CONTINUOUS_MIND_H

