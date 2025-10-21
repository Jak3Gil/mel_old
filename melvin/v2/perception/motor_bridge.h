#pragma once

#include "../core/types_v2.h"
#include "../memory/semantic_bridge.h"
#include <vector>
#include <memory>
#include <cmath>

namespace melvin::v2::perception {

// ============================================================================
// MOTOR BRIDGE - Tokenize Motor State into Graph
// ============================================================================

/**
 * MotorBridge tokenizes motor sensory feedback into graph concepts.
 * 
 * Robstride motors provide continuous streams of:
 * - Position: -12.5 to +12.5 radians
 * - Velocity: -45 to +45 rad/s
 * - Torque: -18 to +18 Nm
 * 
 * These get discretized into NodeIDs that represent motor state concepts.
 * Connections glue them together to form motor percepts.
 * 
 * Example tokenization:
 *   Motor13 --[has_position]--> Position_0.5rad
 *           --[has_velocity]--> Velocity_2.3rad_s
 *           --[has_torque]----> Torque_0.8Nm
 * 
 * This allows:
 * - Motor state to participate in reasoning
 * - Learning motor patterns in semantic memory
 * - Prediction of motor consequences
 * - Integration with visual/audio perception
 */
class MotorBridge {
public:
    struct MotorConfig {
        // Discretization bins (how many concepts per dimension)
        int position_bins;           // Number of position concepts (e.g. 25 = 0.5 rad resolution)
        int velocity_bins;           // Number of velocity concepts (e.g. 18 = 5 rad/s resolution)
        int torque_bins;             // Number of torque concepts (e.g. 18 = 2 Nm resolution)
        
        // Saliency weights (what's important to pay attention to)
        float velocity_weight;       // How important is motion?
        float torque_weight;         // How important is force?
        float position_change_weight; // How important is position change?
        
        // Update thresholds (when to create new concepts)
        float min_position_change;   // Minimum position change to notice (rad)
        float min_velocity_change;   // Minimum velocity change to notice (rad/s)
        float min_torque_change;     // Minimum torque change to notice (Nm)
        
        MotorConfig()
            : position_bins(25), velocity_bins(18), torque_bins(18),
              velocity_weight(0.4f), torque_weight(0.3f), position_change_weight(0.3f),
              min_position_change(0.1f), min_velocity_change(1.0f), min_torque_change(0.5f) {}
    };
    
    struct MotorState {
        uint8_t motor_id;            // Which physical motor (13, 14, etc)
        float position;              // Current position (rad)
        float velocity;              // Current velocity (rad/s)
        float torque;                // Current torque (Nm)
        Timestamp sensed_at;         // When this was read
    };
    
    struct MotorPercept {
        NodeID motor_node;           // Node for "Motor13" or "Motor14"
        NodeID position_node;        // Discretized position concept
        NodeID velocity_node;        // Discretized velocity concept
        NodeID torque_node;          // Discretized torque concept
        
        MotorState raw_state;        // Keep raw values for control
        float salience;              // How interesting is this percept?
        float novelty;               // How unexpected?
        
        MotorPercept() 
            : motor_node(0), position_node(0), velocity_node(0), torque_node(0),
              salience(0.0f), novelty(0.0f) {}
    };
    
    /**
     * Construct motor bridge
     * - semantic_bridge: For creating/querying motor concepts
     * - config: Tokenization parameters
     */
    MotorBridge(memory::SemanticBridge* semantic_bridge, const MotorConfig& config = MotorConfig());
    ~MotorBridge();
    
    // ========================================================================
    // TOKENIZATION (Sensory → Graph)
    // ========================================================================
    
    /**
     * Tokenize motor state into graph concepts
     * 
     * Takes raw motor feedback (position, velocity, torque) and:
     * 1. Discretizes values into bins
     * 2. Creates/retrieves NodeIDs for each bin
     * 3. Creates edges connecting motor → states
     * 4. Computes salience (how interesting?)
     * 5. Computes novelty (how unexpected?)
     * 
     * Returns MotorPercept ready for working memory / global workspace
     */
    MotorPercept tokenize_state(const MotorState& state);
    
    /**
     * Tokenize multiple motors at once
     */
    std::vector<MotorPercept> tokenize_motors(const std::vector<MotorState>& states);
    
    // ========================================================================
    // DETOKENIZATION (Graph → Motor Commands)
    // ========================================================================
    
    /**
     * Convert desired motor concept nodes back to target values
     * 
     * E.g., if reasoning produces:
     *   Motor13 --[should_be_at]--> Position_2.5rad
     * 
     * This converts back to actual target: 2.5 rad
     */
    MotorState detokenize_command(NodeID motor_node, 
                                   NodeID target_position_node,
                                   NodeID target_velocity_node = 0,
                                   NodeID target_torque_node = 0);
    
    // ========================================================================
    // CONCEPT MANAGEMENT
    // ========================================================================
    
    /**
     * Initialize motor concept in semantic memory
     * - Creates node for "Motor13", "Motor14", etc
     */
    NodeID register_motor(uint8_t motor_id, const std::string& label = "");
    
    /**
     * Get or create position concept for given value
     * - Returns NodeID for discretized position bin
     * - Creates "Position_X.Xrad" concept if doesn't exist
     */
    NodeID get_position_concept(float position_rad);
    
    /**
     * Get or create velocity concept
     */
    NodeID get_velocity_concept(float velocity_rad_s);
    
    /**
     * Get or create torque concept
     */
    NodeID get_torque_concept(float torque_nm);
    
    /**
     * Get value from concept (reverse lookup)
     */
    float get_position_value(NodeID position_node);
    float get_velocity_value(NodeID velocity_node);
    float get_torque_value(NodeID torque_node);
    
    // ========================================================================
    // PATTERN LEARNING
    // ========================================================================
    
    /**
     * Record motor transition in semantic memory
     * 
     * Creates/strengthens edges:
     *   Position(t) --[leads_to]--> Position(t+1)
     *   Velocity(t) --[causes]--> Torque(t)
     * 
     * This allows prediction of motor consequences!
     */
    void record_transition(const MotorPercept& from_state, const MotorPercept& to_state);
    
    /**
     * Predict next motor state from current percept
     * - Uses learned transitions in semantic memory
     * - Returns predicted position/velocity/torque nodes
     */
    MotorPercept predict_next_state(const MotorPercept& current_state);
    
    // ========================================================================
    // SALIENCY & NOVELTY
    // ========================================================================
    
    /**
     * Compute how interesting this motor state is
     * - Based on velocity (motion), torque (force), position change
     */
    float compute_saliency(const MotorState& state);
    
    /**
     * Compute how unexpected this state is
     * - Compares to predicted state from learned patterns
     */
    float compute_novelty(const MotorPercept& percept);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    struct Stats {
        size_t states_tokenized;
        size_t commands_detokenized;
        size_t position_concepts_created;
        size_t velocity_concepts_created;
        size_t torque_concepts_created;
        size_t transitions_recorded;
        float avg_salience;
        float avg_novelty;
    };
    
    Stats get_stats() const;
    void reset_stats();
    
    /**
     * Get configuration
     */
    MotorConfig get_config() const { return config_; }
    
private:
    memory::SemanticBridge* semantic_bridge_;  // Access to semantic memory
    MotorConfig config_;
    Stats stats_;
    
    // Previous states for computing novelty
    std::vector<MotorState> prev_states_;
    
    // Motor node cache (motor_id -> NodeID)
    std::vector<std::pair<uint8_t, NodeID>> motor_nodes_;
    
    // Concept caches for fast lookup
    struct ConceptBin {
        float center_value;
        NodeID node_id;
    };
    std::vector<ConceptBin> position_concepts_;
    std::vector<ConceptBin> velocity_concepts_;
    std::vector<ConceptBin> torque_concepts_;
    
    // Helpers
    NodeID get_motor_node(uint8_t motor_id);
    MotorState* get_prev_state(uint8_t motor_id);
    void update_prev_state(const MotorState& state);
    
    float discretize_position(float position);
    float discretize_velocity(float velocity);
    float discretize_torque(float torque);
    
    std::string format_position_label(float position);
    std::string format_velocity_label(float velocity);
    std::string format_torque_label(float torque);
};

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

/**
 * Create motor state from Robstride motor feedback
 */
inline MotorBridge::MotorState create_motor_state(uint8_t motor_id,
                                                   float position,
                                                   float velocity,
                                                   float torque) {
    MotorBridge::MotorState state;
    state.motor_id = motor_id;
    state.position = position;
    state.velocity = velocity;
    state.torque = torque;
    state.sensed_at = get_timestamp_ns();
    return state;
}

} // namespace melvin::v2::perception





