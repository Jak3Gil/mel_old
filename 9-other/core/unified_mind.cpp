/*
 * Unified Mind - Complete cognitive loop implementation
 */

#include "unified_mind.h"
#include <iostream>
#include <algorithm>
#include <cmath>

namespace melvin {

// ============================================================================
// FEEDBACK BUS
// ============================================================================

FeedbackBus::FeedbackBus() {
}

FeedbackBus::~FeedbackBus() {
}

void FeedbackBus::process_thought(const Thought& thought) {
    // Extract keywords from thought
    for (const auto& keyword : thought.keywords) {
        // Increase bias for this keyword
        keyword_biases_[keyword] += 0.2f * thought.confidence;
        keyword_mentions_[keyword]++;
        
        // Cap bias at 0.5
        if (keyword_biases_[keyword] > 0.5f) {
            keyword_biases_[keyword] = 0.5f;
        }
    }
    
    // Also bias towards subject and object
    if (!thought.subject.empty()) {
        keyword_biases_[thought.subject] += 0.15f * thought.confidence;
    }
    if (!thought.object.empty()) {
        keyword_biases_[thought.object] += 0.15f * thought.confidence;
    }
}

std::unordered_map<std::string, float> FeedbackBus::get_attention_biases() const {
    return keyword_biases_;
}

void FeedbackBus::decay_biases(float decay_rate) {
    for (auto& pair : keyword_biases_) {
        pair.second *= decay_rate;
        
        // Remove if too small
        if (pair.second < 0.01f) {
            pair.second = 0.0f;
        }
    }
}

void FeedbackBus::clear() {
    keyword_biases_.clear();
    keyword_mentions_.clear();
}

// ============================================================================
// UNIFIED MIND
// ============================================================================

UnifiedMind::UnifiedMind() 
    : cycle_count_(0)
{
    graph_ = std::make_unique<AtomicGraph>();
    attention_ = std::make_unique<AttentionManager>();
    feedback_ = std::make_unique<FeedbackBus>();
    
    start_time_ = std::chrono::steady_clock::now();
    
    std::cout << "🧠 UnifiedMind initialized" << std::endl;
}

UnifiedMind::~UnifiedMind() {
}

// ============================================================================
// INITIALIZATION
// ============================================================================

bool UnifiedMind::load(const std::string& nodes_path, const std::string& edges_path) {
    try {
        graph_->load(nodes_path, edges_path);
        std::cout << "📂 Knowledge loaded: " 
                  << graph_->node_count() << " nodes, "
                  << graph_->edge_count() << " edges" << std::endl;
        return true;
    } catch (...) {
        std::cout << "📝 Starting with fresh knowledge" << std::endl;
        return false;
    }
}

bool UnifiedMind::save(const std::string& nodes_path, const std::string& edges_path) {
    graph_->save(nodes_path, edges_path);
    return true;
}

// ============================================================================
// MAIN COGNITIVE LOOP
// ============================================================================

OutputExpression UnifiedMind::unified_cycle(const SensoryFrame& input) {
    cycle_count_++;
    
    // === STAGE 1: PERCEPTION ===
    perception_stage(input);
    
    // === STAGE 2: ATTENTION ===
    FocusTarget focus = attention_stage();
    current_focus_ = focus;
    
    // === STAGE 3: REASONING ===
    Thought thought;
    if (focus.is_valid()) {
        thought = reasoning_stage(focus);
        last_thought_ = thought;
    }
    
    // === STAGE 4: OUTPUT ===
    OutputExpression output = output_stage(thought);
    
    // === STAGE 5: FEEDBACK ===
    if (focus.is_valid()) {
        feedback_stage(thought, focus);
    }
    
    // Decay old biases
    feedback_->decay_biases(0.98f);
    
    return output;
}

// ============================================================================
// PIPELINE STAGES
// ============================================================================

void UnifiedMind::perception_stage(const SensoryFrame& input) {
    // Collect all candidates from all modalities
    current_candidates_.clear();
    
    // Add visual candidates
    for (const auto& candidate : input.visual_candidates) {
        current_candidates_.push_back(candidate);
    }
    
    // Add audio candidates
    for (const auto& candidate : input.audio_candidates) {
        current_candidates_.push_back(candidate);
    }
    
    // Apply feedback biases to candidates
    auto biases = feedback_->get_attention_biases();
    
    for (auto& candidate : current_candidates_) {
        // Check if this candidate matches any biased keyword
        for (const auto& [keyword, bias] : biases) {
            if (candidate.label.find(keyword) != std::string::npos) {
                // Boost relevance based on feedback
                candidate.R = std::min(1.0f, candidate.R + bias);
            }
        }
    }
}

FocusTarget UnifiedMind::attention_stage() {
    FocusTarget target;
    
    if (current_candidates_.empty()) {
        return target;  // Invalid target
    }
    
    // Clear previous candidates
    attention_->clear_candidates();
    
    // Add all candidates
    for (const auto& candidate : current_candidates_) {
        attention_->add_candidate(candidate);
    }
    
    // Select focus
    auto now = std::chrono::steady_clock::now();
    double current_time = std::chrono::duration<double>(now - start_time_).count();
    
    FocusCandidate selected = attention_->select_focus_target(current_time);
    
    if (!selected.label.empty()) {
        target.id = selected.label;
        target.type = selected.source;
        target.candidate = selected;
    }
    
    return target;
}

Thought UnifiedMind::reasoning_stage(const FocusTarget& focus) {
    Thought thought;
    
    if (!focus.is_valid()) {
        return thought;
    }
    
    thought.timestamp = focus.candidate.timestamp;
    thought.confidence = focus.candidate.focus_score;
    
    // === INTERPRET FOCUS ===
    
    // Subject: the focused object
    thought.subject = focus.id;
    
    // Predicate: determine action/state based on scores
    if (focus.candidate.C > 0.7) {
        thought.predicate = "moves_unexpectedly";
        thought.keywords.push_back("motion");
        thought.keywords.push_back("curiosity");
    } else if (focus.candidate.T > 0.6) {
        thought.predicate = "persists";
        thought.keywords.push_back("stable");
        thought.keywords.push_back("tracked");
    } else if (focus.candidate.A > 0.7) {
        thought.predicate = "is_salient";
        thought.keywords.push_back("bright");
        thought.keywords.push_back("detailed");
    } else {
        thought.predicate = "appears";
        thought.keywords.push_back("present");
    }
    
    // Object: query graph for related concepts
    auto node_id = graph_->get_or_create_node(focus.id, 0);  // 0 = SENSORY
    auto neighbors = graph_->neighbors(node_id);
    
    if (!neighbors.empty()) {
        // Find strongest connected concept
        float max_weight = 0;
        uint64_t best_neighbor = 0;
        
        for (auto neighbor_id : neighbors) {
            float weight = graph_->get_edge_weight(node_id, neighbor_id, CO_OCCURS_WITH);
            if (weight > max_weight) {
                max_weight = weight;
                best_neighbor = neighbor_id;
            }
        }
        
        if (best_neighbor > 0) {
            const Node* neighbor_node = graph_->get_node(best_neighbor);
            if (neighbor_node) {
                thought.object = std::string(neighbor_node->label, 
                                            strnlen(neighbor_node->label, 32));
                thought.keywords.push_back(thought.object);
            }
        }
    }
    
    // If no neighbors, use focus type
    if (thought.object.empty()) {
        thought.object = focus.type + "_region";
    }
    
    return thought;
}

OutputExpression UnifiedMind::output_stage(const Thought& thought) {
    OutputExpression output;
    
    if (thought.subject.empty()) {
        return output;  // No output
    }
    
    auto now = std::chrono::steady_clock::now();
    output.timestamp = std::chrono::duration<double>(now - start_time_).count();
    
    // Generate text
    output.text = "I " + thought.predicate + " " + thought.subject;
    
    // Add object if meaningful
    if (!thought.object.empty() && thought.object != thought.subject) {
        output.text += " (" + thought.object + ")";
    }
    
    // Set emotional tone based on curiosity and salience
    if (thought.keywords.size() > 0) {
        bool has_curiosity = std::find(thought.keywords.begin(), thought.keywords.end(), 
                                      "curiosity") != thought.keywords.end();
        bool has_motion = std::find(thought.keywords.begin(), thought.keywords.end(), 
                                   "motion") != thought.keywords.end();
        
        if (has_curiosity || has_motion) {
            output.emotional_tone = 0.6f;  // Interested/excited
        }
    }
    
    output.speech_text = output.text;
    
    return output;
}

void UnifiedMind::feedback_stage(const Thought& thought, const FocusTarget& focus) {
    // === UPDATE FEEDBACK BIASES ===
    feedback_->process_thought(thought);
    
    // === REINFORCE MEMORY ===
    // Create/reinforce node for focused object
    auto subject_id = graph_->get_or_create_node(thought.subject, 0);  // SENSORY
    
    // If thought has an object, link them
    if (!thought.object.empty() && thought.object != thought.subject) {
        auto object_id = graph_->get_or_create_node(thought.object, 1);  // CONCEPT
        
        // Create EXACT edge (co-occurrence)
        graph_->add_edge(subject_id, object_id, CO_OCCURS_WITH, thought.confidence);
    }
    
    // Create temporal edges with recent focus history
    auto history = attention_->get_focus_history(5);
    if (history.size() > 1) {
        // Link current focus with previous focus (temporal sequence)
        const auto& prev = history[history.size() - 2];
        auto prev_id = graph_->get_or_create_node(prev.label, 0);
        graph_->add_edge(prev_id, subject_id, TEMPORAL_NEXT, 0.5f);
    }
}

// ========================================================================
// STATE MANAGEMENT
// ========================================================================

void UnifiedMind::set_state(MelvinState state) {
    attention_->set_state(state);
}

void UnifiedMind::print_status() const {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 UNIFIED MIND STATUS                               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "Cycles: " << cycle_count_ << "\n";
    std::cout << "Knowledge: " << graph_->node_count() << " nodes, "
              << graph_->edge_count() << " edges\n\n";
    
    if (current_focus_.is_valid()) {
        std::cout << "Current Focus:\n";
        std::cout << "  ID: " << current_focus_.id << "\n";
        std::cout << "  Type: " << current_focus_.type << "\n";
        std::cout << "  Score: " << current_focus_.candidate.focus_score << "\n\n";
    }
    
    if (!last_thought_.subject.empty()) {
        std::cout << "Last Thought:\n";
        std::cout << "  " << last_thought_.to_sentence() << "\n";
        std::cout << "  Confidence: " << last_thought_.confidence << "\n";
        std::cout << "  Keywords: ";
        for (const auto& kw : last_thought_.keywords) {
            std::cout << kw << " ";
        }
        std::cout << "\n\n";
    }
    
    auto biases = feedback_->get_attention_biases();
    if (!biases.empty()) {
        std::cout << "Active Biases:\n";
        for (const auto& [keyword, bias] : biases) {
            if (bias > 0.01f) {
                std::cout << "  " << keyword << ": +" << bias << "\n";
            }
        }
        std::cout << "\n";
    }
}

} // namespace melvin

