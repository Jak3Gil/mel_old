#include "motor_bridge.h"
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace melvin::v2::perception {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

MotorBridge::MotorBridge(memory::SemanticBridge* semantic_bridge, const MotorConfig& config)
    : semantic_bridge_(semantic_bridge), config_(config), stats_()
{
    if (!semantic_bridge_) {
        throw std::runtime_error("MotorBridge requires valid SemanticBridge");
    }
    
    // Initialize concept caches
    position_concepts_.reserve(config_.position_bins);
    velocity_concepts_.reserve(config_.velocity_bins);
    torque_concepts_.reserve(config_.torque_bins);
}

MotorBridge::~MotorBridge() = default;

// ============================================================================
// TOKENIZATION (Sensory → Graph)
// ============================================================================

MotorBridge::MotorPercept MotorBridge::tokenize_state(const MotorState& state) {
    MotorPercept percept;
    percept.raw_state = state;
    
    // Get motor node (or create if first time seeing this motor)
    percept.motor_node = get_motor_node(state.motor_id);
    
    // Discretize and get concept nodes
    percept.position_node = get_position_concept(state.position);
    percept.velocity_node = get_velocity_concept(state.velocity);
    percept.torque_node = get_torque_concept(state.torque);
    
    // Create edges in semantic memory: Motor --[has_X]--> Value
    // Relation types: 1=has_position, 2=has_velocity, 3=has_torque
    semantic_bridge_->add_relation(percept.motor_node, percept.position_node, 1, 1.0f);
    semantic_bridge_->add_relation(percept.motor_node, percept.velocity_node, 2, 1.0f);
    semantic_bridge_->add_relation(percept.motor_node, percept.torque_node, 3, 1.0f);
    
    // Compute perceptual properties
    percept.salience = compute_saliency(state);
    percept.novelty = compute_novelty(percept);
    
    // Update statistics
    stats_.states_tokenized++;
    stats_.avg_salience = (stats_.avg_salience * (stats_.states_tokenized - 1) + percept.salience) / stats_.states_tokenized;
    stats_.avg_novelty = (stats_.avg_novelty * (stats_.states_tokenized - 1) + percept.novelty) / stats_.states_tokenized;
    
    // Store for novelty computation next time
    update_prev_state(state);
    
    return percept;
}

std::vector<MotorBridge::MotorPercept> MotorBridge::tokenize_motors(const std::vector<MotorState>& states) {
    std::vector<MotorPercept> percepts;
    percepts.reserve(states.size());
    
    for (const auto& state : states) {
        percepts.push_back(tokenize_state(state));
    }
    
    return percepts;
}

// ============================================================================
// DETOKENIZATION (Graph → Motor Commands)
// ============================================================================

MotorBridge::MotorState MotorBridge::detokenize_command(NodeID motor_node,
                                                         NodeID target_position_node,
                                                         NodeID target_velocity_node,
                                                         NodeID target_torque_node) {
    MotorState command;
    
    // Find which motor this is
    uint8_t motor_id = 0;
    for (const auto& [id, node] : motor_nodes_) {
        if (node == motor_node) {
            motor_id = id;
            break;
        }
    }
    command.motor_id = motor_id;
    
    // Convert concept nodes back to values
    command.position = target_position_node ? get_position_value(target_position_node) : 0.0f;
    command.velocity = target_velocity_node ? get_velocity_value(target_velocity_node) : 0.0f;
    command.torque = target_torque_node ? get_torque_value(target_torque_node) : 0.0f;
    command.sensed_at = get_timestamp_ns();
    
    stats_.commands_detokenized++;
    
    return command;
}

// ============================================================================
// CONCEPT MANAGEMENT
// ============================================================================

NodeID MotorBridge::register_motor(uint8_t motor_id, const std::string& label) {
    // Check if already registered
    for (const auto& [id, node] : motor_nodes_) {
        if (id == motor_id) {
            return node;
        }
    }
    
    // Create motor concept
    std::string motor_label = label.empty() 
        ? ("Motor" + std::to_string(static_cast<int>(motor_id)))
        : label;
    
    NodeID motor_node = semantic_bridge_->create_concept(motor_label, 10); // Type 10 = motor
    motor_nodes_.push_back({motor_id, motor_node});
    
    return motor_node;
}

NodeID MotorBridge::get_position_concept(float position_rad) {
    float discretized = discretize_position(position_rad);
    
    // Check cache
    for (const auto& bin : position_concepts_) {
        if (std::abs(bin.center_value - discretized) < 0.01f) {
            return bin.node_id;
        }
    }
    
    // Create new concept
    std::string label = format_position_label(discretized);
    NodeID node = semantic_bridge_->create_concept(label, 11); // Type 11 = position
    
    position_concepts_.push_back({discretized, node});
    stats_.position_concepts_created++;
    
    return node;
}

NodeID MotorBridge::get_velocity_concept(float velocity_rad_s) {
    float discretized = discretize_velocity(velocity_rad_s);
    
    // Check cache
    for (const auto& bin : velocity_concepts_) {
        if (std::abs(bin.center_value - discretized) < 0.01f) {
            return bin.node_id;
        }
    }
    
    // Create new concept
    std::string label = format_velocity_label(discretized);
    NodeID node = semantic_bridge_->create_concept(label, 12); // Type 12 = velocity
    
    velocity_concepts_.push_back({discretized, node});
    stats_.velocity_concepts_created++;
    
    return node;
}

NodeID MotorBridge::get_torque_concept(float torque_nm) {
    float discretized = discretize_torque(torque_nm);
    
    // Check cache
    for (const auto& bin : torque_concepts_) {
        if (std::abs(bin.center_value - discretized) < 0.01f) {
            return bin.node_id;
        }
    }
    
    // Create new concept
    std::string label = format_torque_label(discretized);
    NodeID node = semantic_bridge_->create_concept(label, 13); // Type 13 = torque
    
    torque_concepts_.push_back({discretized, node});
    stats_.torque_concepts_created++;
    
    return node;
}

float MotorBridge::get_position_value(NodeID position_node) {
    // Find in cache
    for (const auto& bin : position_concepts_) {
        if (bin.node_id == position_node) {
            return bin.center_value;
        }
    }
    
    // Parse from label if not in cache
    std::string label = semantic_bridge_->get_label(position_node);
    // Label format: "Position_X.Xrad"
    size_t pos = label.find("_");
    if (pos != std::string::npos) {
        return std::stof(label.substr(pos + 1));
    }
    
    return 0.0f;
}

float MotorBridge::get_velocity_value(NodeID velocity_node) {
    for (const auto& bin : velocity_concepts_) {
        if (bin.node_id == velocity_node) {
            return bin.center_value;
        }
    }
    
    std::string label = semantic_bridge_->get_label(velocity_node);
    size_t pos = label.find("_");
    if (pos != std::string::npos) {
        return std::stof(label.substr(pos + 1));
    }
    
    return 0.0f;
}

float MotorBridge::get_torque_value(NodeID torque_node) {
    for (const auto& bin : torque_concepts_) {
        if (bin.node_id == torque_node) {
            return bin.center_value;
        }
    }
    
    std::string label = semantic_bridge_->get_label(torque_node);
    size_t pos = label.find("_");
    if (pos != std::string::npos) {
        return std::stof(label.substr(pos + 1));
    }
    
    return 0.0f;
}

// ============================================================================
// PATTERN LEARNING
// ============================================================================

void MotorBridge::record_transition(const MotorPercept& from_state, const MotorPercept& to_state) {
    // Only record if same motor
    if (from_state.motor_node != to_state.motor_node) {
        return;
    }
    
    // Record temporal transitions
    // Position(t) --[leads_to]--> Position(t+1)
    semantic_bridge_->add_relation(from_state.position_node, to_state.position_node, 20, 0.5f);
    semantic_bridge_->strengthen_relation(from_state.position_node, to_state.position_node, 0.1f);
    
    // Velocity(t) --[leads_to]--> Velocity(t+1)
    semantic_bridge_->add_relation(from_state.velocity_node, to_state.velocity_node, 20, 0.5f);
    semantic_bridge_->strengthen_relation(from_state.velocity_node, to_state.velocity_node, 0.1f);
    
    // Record causal patterns
    // Velocity(t) --[influences]--> Position(t+1)
    semantic_bridge_->add_relation(from_state.velocity_node, to_state.position_node, 21, 0.3f);
    
    // Torque(t) --[influences]--> Velocity(t+1)
    semantic_bridge_->add_relation(from_state.torque_node, to_state.velocity_node, 21, 0.3f);
    
    stats_.transitions_recorded++;
}

MotorBridge::MotorPercept MotorBridge::predict_next_state(const MotorPercept& current_state) {
    MotorPercept predicted = current_state;
    
    // Query semantic memory for learned transitions
    std::vector<NodeID> position_successors = semantic_bridge_->find_related(
        current_state.position_node, 0.3f);
    
    std::vector<NodeID> velocity_successors = semantic_bridge_->find_related(
        current_state.velocity_node, 0.3f);
    
    // Use strongest successor as prediction
    if (!position_successors.empty()) {
        predicted.position_node = position_successors[0];
        predicted.raw_state.position = get_position_value(position_successors[0]);
    }
    
    if (!velocity_successors.empty()) {
        predicted.velocity_node = velocity_successors[0];
        predicted.raw_state.velocity = get_velocity_value(velocity_successors[0]);
    }
    
    return predicted;
}

// ============================================================================
// SALIENCY & NOVELTY
// ============================================================================

float MotorBridge::compute_saliency(const MotorState& state) {
    // Saliency = weighted combination of:
    // - Velocity magnitude (motion is salient)
    // - Torque magnitude (force is salient)
    // - Position change rate (movement is salient)
    
    float velocity_saliency = std::abs(state.velocity) / 45.0f;  // Normalize to 0-1
    float torque_saliency = std::abs(state.torque) / 18.0f;      // Normalize to 0-1
    
    float position_change_saliency = 0.0f;
    MotorState* prev = get_prev_state(state.motor_id);
    if (prev) {
        float pos_change = std::abs(state.position - prev->position);
        position_change_saliency = std::min(pos_change / config_.min_position_change, 1.0f);
    }
    
    float salience = 
        config_.velocity_weight * velocity_saliency +
        config_.torque_weight * torque_saliency +
        config_.position_change_weight * position_change_saliency;
    
    return std::min(salience, 1.0f);
}

float MotorBridge::compute_novelty(const MotorPercept& percept) {
    // Predict what should happen next based on learned patterns
    MotorPercept predicted = predict_next_state(percept);
    
    // Novelty = difference between predicted and actual
    float position_error = std::abs(
        percept.raw_state.position - predicted.raw_state.position
    );
    
    float velocity_error = std::abs(
        percept.raw_state.velocity - predicted.raw_state.velocity
    );
    
    // Normalize errors
    float position_novelty = std::min(position_error / 2.0f, 1.0f);
    float velocity_novelty = std::min(velocity_error / 10.0f, 1.0f);
    
    return (position_novelty + velocity_novelty) / 2.0f;
}

// ============================================================================
// STATISTICS
// ============================================================================

MotorBridge::Stats MotorBridge::get_stats() const {
    return stats_;
}

void MotorBridge::reset_stats() {
    stats_ = Stats();
}

// ============================================================================
// PRIVATE HELPERS
// ============================================================================

NodeID MotorBridge::get_motor_node(uint8_t motor_id) {
    for (const auto& [id, node] : motor_nodes_) {
        if (id == motor_id) {
            return node;
        }
    }
    
    // Auto-register if not found
    return register_motor(motor_id);
}

MotorBridge::MotorState* MotorBridge::get_prev_state(uint8_t motor_id) {
    for (auto& state : prev_states_) {
        if (state.motor_id == motor_id) {
            return &state;
        }
    }
    return nullptr;
}

void MotorBridge::update_prev_state(const MotorState& state) {
    // Update or add
    for (auto& prev : prev_states_) {
        if (prev.motor_id == state.motor_id) {
            prev = state;
            return;
        }
    }
    
    // New motor
    prev_states_.push_back(state);
}

float MotorBridge::discretize_position(float position) {
    // Range: -12.5 to +12.5 rad
    // Bin size = 25.0 / position_bins
    float bin_size = 25.0f / config_.position_bins;
    float normalized = position + 12.5f;  // Shift to 0-25
    int bin_idx = static_cast<int>(normalized / bin_size);
    bin_idx = std::clamp(bin_idx, 0, config_.position_bins - 1);
    
    // Return center of bin
    return (bin_idx * bin_size + bin_size / 2.0f) - 12.5f;
}

float MotorBridge::discretize_velocity(float velocity) {
    // Range: -45 to +45 rad/s
    float bin_size = 90.0f / config_.velocity_bins;
    float normalized = velocity + 45.0f;
    int bin_idx = static_cast<int>(normalized / bin_size);
    bin_idx = std::clamp(bin_idx, 0, config_.velocity_bins - 1);
    
    return (bin_idx * bin_size + bin_size / 2.0f) - 45.0f;
}

float MotorBridge::discretize_torque(float torque) {
    // Range: -18 to +18 Nm
    float bin_size = 36.0f / config_.torque_bins;
    float normalized = torque + 18.0f;
    int bin_idx = static_cast<int>(normalized / bin_size);
    bin_idx = std::clamp(bin_idx, 0, config_.torque_bins - 1);
    
    return (bin_idx * bin_size + bin_size / 2.0f) - 18.0f;
}

std::string MotorBridge::format_position_label(float position) {
    std::ostringstream oss;
    oss << "Position_" << std::fixed << std::setprecision(2) << position << "rad";
    return oss.str();
}

std::string MotorBridge::format_velocity_label(float velocity) {
    std::ostringstream oss;
    oss << "Velocity_" << std::fixed << std::setprecision(1) << velocity << "rad_s";
    return oss.str();
}

std::string MotorBridge::format_torque_label(float torque) {
    std::ostringstream oss;
    oss << "Torque_" << std::fixed << std::setprecision(2) << torque << "Nm";
    return oss.str();
}

} // namespace melvin::v2::perception





