#pragma once

#include "melvin_reasoning.h"
#include "melvin_focus.h"
#include <string>
#include <vector>

namespace melvin {
namespace uca {

// ============================================================================
// BIOLOGICAL ANALOG: Motor Cortex + Output Systems
// ============================================================================

/**
 * Output modality
 */
enum OutputModality {
    MOTOR_ACTION,      // Physical movement (M1/PM)
    SPEECH,            // Vocalization (Broca's area)
    GAZE,              // Eye movement (FEF/SC)
    INTERNAL           // Internal monologue (no external output)
};

/**
 * Motor action types
 */
enum MotorAction {
    NO_ACTION,
    MOVE_FORWARD,
    MOVE_BACKWARD,
    TURN_LEFT,
    TURN_RIGHT,
    REACH_TOWARD,
    GRASP_OBJECT,
    RELEASE_OBJECT,
    POINT_AT
};

/**
 * Output command structure
 */
struct OutputCommand {
    OutputModality modality;
    
    // Motor
    MotorAction action;
    float action_params[4];       // Action-specific parameters
    
    // Speech
    std::string speech_text;
    float speech_emotion;         // -1 (negative) to +1 (positive)
    
    // Gaze
    int gaze_x, gaze_y;           // Target coordinates
    
    // Internal
    std::string internal_thought;
    
    // Metadata
    float confidence;             // How certain about this action
    float urgency;                // How important/time-critical
    double timestamp;
    
    OutputCommand() : modality(INTERNAL), action(NO_ACTION),
                      speech_emotion(0), gaze_x(0), gaze_y(0),
                      confidence(0), urgency(0), timestamp(0) {
        action_params[0] = action_params[1] = action_params[2] = action_params[3] = 0;
    }
};

/**
 * Execution result
 */
struct ExecutionResult {
    bool success;
    std::string error_message;
    double execution_time_ms;
    
    // Observed outcome (for learning)
    std::string observed_result;
    float outcome_quality;        // How good was the result?
    
    ExecutionResult() : success(false), execution_time_ms(0), outcome_quality(0) {}
};

// ============================================================================
// MELVIN OUTPUT - Motor Cortex & Action Generation
// ============================================================================

/**
 * MelvinOutput: Motor cortex analog + output generation
 * 
 * Implements:
 * - Action selection from reasoning
 * - Motor command generation
 * - Speech synthesis
 * - Gaze control
 * - Internal monologue
 * - Action sequencing
 * 
 * Biological parallels:
 * - generate_output() → Supplementary motor area (SMA) planning
 * - execute_action() → Primary motor cortex (M1) execution
 * - generate_speech() → Broca's area
 * - control_gaze() → Frontal eye fields (FEF)
 * - think_internally() → Inner speech (left inferior frontal)
 */
class MelvinOutput {
public:
    struct Config {
        // Enabled modalities
        bool enable_motor;            // Physical actions
        bool enable_speech;           // Vocalization
        bool enable_gaze;             // Eye movement control
        bool enable_internal;         // Internal thoughts
        
        // Speech generation
        float speech_rate;            // Words per minute (default: 150)
        bool speech_verbose;          // Include reasoning (default: false)
        
        // Motor control
        float motor_speed;            // Action speed multiplier (default: 1.0)
        bool motor_safety_check;      // Validate before execution (default: true)
        
        // Action selection
        float confidence_threshold;   // Min confidence to act (default: 0.5)
        bool allow_concurrent;        // Multiple outputs at once (default: false)
        
        Config() : enable_motor(false), enable_speech(true),
                   enable_gaze(true), enable_internal(true),
                   speech_rate(150), speech_verbose(false),
                   motor_speed(1.0f), motor_safety_check(true),
                   confidence_threshold(0.5f), allow_concurrent(false) {}
    };
    
    MelvinOutput(const Config& config = Config());
    ~MelvinOutput();
    
    // ========================================================================
    // OUTPUT GENERATION
    // ========================================================================
    
    /**
     * Generate output from reasoning result
     * Converts thoughts into actions, speech, or internal monologue
     */
    std::vector<OutputCommand> generate_output(
        const ReasoningResult& reasoning,
        const FocusTarget& current_focus
    );
    
    /**
     * Generate speech from thought
     * Converts subject-predicate-object into natural language
     */
    OutputCommand generate_speech(const Thought& thought);
    
    /**
     * Generate motor action
     * Based on current goal and focus
     */
    OutputCommand generate_motor_action(
        const std::string& goal,
        const FocusTarget& focus
    );
    
    /**
     * Generate gaze command
     * Direct eyes/camera toward focus
     */
    OutputCommand generate_gaze_command(const FocusTarget& focus);
    
    /**
     * Generate internal thought
     * No external output, just conscious awareness
     */
    OutputCommand generate_internal_thought(const Thought& thought);
    
    // ========================================================================
    // ACTION EXECUTION
    // ========================================================================
    
    /**
     * Execute output command
     * Routes to appropriate subsystem (motor, TTS, etc.)
     */
    ExecutionResult execute(const OutputCommand& command);
    
    /**
     * Execute action sequence
     * For multi-step plans
     */
    std::vector<ExecutionResult> execute_sequence(
        const std::vector<OutputCommand>& sequence
    );
    
    /**
     * Emergency stop (cancel all outputs)
     */
    void emergency_stop();
    
    // ========================================================================
    // ACTION PLANNING
    // ========================================================================
    
    /**
     * Plan action sequence to achieve goal
     * Simple temporal planning
     */
    std::vector<OutputCommand> plan_action_sequence(
        const std::string& goal,
        int max_steps = 5
    );
    
    /**
     * Evaluate action before execution
     * Safety check + outcome prediction
     */
    struct ActionEvaluation {
        bool is_safe;
        float expected_quality;
        std::vector<std::string> risks;
        std::string recommendation;
    };
    
    ActionEvaluation evaluate_action(const OutputCommand& command);
    
    // ========================================================================
    // SPEECH SYNTHESIS
    // ========================================================================
    
    /**
     * Convert thought to natural language sentence
     */
    std::string thought_to_sentence(const Thought& thought);
    
    /**
     * Add emotion to speech
     */
    std::string add_emotional_tone(const std::string& text, float emotion);
    
    /**
     * Generate explanation of reasoning
     */
    std::string generate_explanation(const ReasoningResult& reasoning);
    
    // ========================================================================
    // MOTOR CONTROL INTERFACE
    // ========================================================================
    
    /**
     * Connect to motor system (RobStride, servos, etc.)
     */
    bool connect_motor_system(const std::string& interface);
    
    /**
     * Send motor command
     */
    bool send_motor_command(MotorAction action, const float* params);
    
    /**
     * Get motor status
     */
    struct MotorStatus {
        bool connected;
        bool ready;
        std::string error;
    };
    MotorStatus get_motor_status() const;
    
    // ========================================================================
    // FEEDBACK & LEARNING
    // ========================================================================
    
    /**
     * Record action outcome for learning
     * Used to improve action selection
     */
    void record_outcome(
        const OutputCommand& command,
        const ExecutionResult& result
    );
    
    /**
     * Get action history
     */
    struct ActionHistory {
        OutputCommand command;
        ExecutionResult result;
        double timestamp;
    };
    std::vector<ActionHistory> get_action_history(int count = 10) const;
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct OutputStats {
        uint64_t total_commands;
        uint64_t motor_actions;
        uint64_t speech_outputs;
        uint64_t gaze_shifts;
        uint64_t internal_thoughts;
        uint64_t successful_executions;
        uint64_t failed_executions;
        float avg_confidence;
        float avg_outcome_quality;
    };
    
    OutputStats get_stats() const { return stats_; }
    void print_stats() const;
    
private:
    // ========================================================================
    // INTERNAL HELPERS
    // ========================================================================
    
    // Select best output modality for situation
    OutputModality select_modality(
        const ReasoningResult& reasoning,
        const FocusTarget& focus
    );
    
    // Validate motor action safety
    bool is_action_safe(const OutputCommand& command);
    
    // Format speech with proper grammar
    std::string format_speech(const std::string& text);
    
    // ========================================================================
    // MOTOR INTERFACE (Stub for now)
    // ========================================================================
    
    struct MotorInterface {
        bool connected;
        std::string type;  // "robstride", "servo", "simulator"
        void* handle;      // Interface handle
    };
    MotorInterface motor_;
    
    // ========================================================================
    // STATE
    // ========================================================================
    
    Config config_;
    
    // Action history
    std::deque<ActionHistory> action_history_;
    
    // Current executing command
    OutputCommand current_command_;
    bool is_executing_;
    
    // Statistics
    OutputStats stats_;
};

} // namespace uca
} // namespace melvin


