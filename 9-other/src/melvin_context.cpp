/**
 * @file melvin_context.cpp
 * @brief Unified Multimodal Context System Implementation
 * 
 * This implements Melvin's "consciousness" - a continuously evolving
 * understanding of what's happening in the world right now.
 */

#include "melvin_context.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>

namespace melvin {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

ContextField::ContextField(AtomicGraph& graph)
    : graph_(graph)
    , alpha_vision_(0.50f)
    , alpha_audio_(0.30f)
    , alpha_text_(0.20f)
    , cumulative_prediction_error_(0.0f)
    , cycle_count_(0)
{
    last_update_ = std::chrono::steady_clock::now();
    std::cout << "🧠 Multimodal ContextField initialized" << std::endl;
    std::cout << "   Vision weight: " << alpha_vision_ << std::endl;
    std::cout << "   Audio weight:  " << alpha_audio_ << std::endl;
    std::cout << "   Text weight:   " << alpha_text_ << std::endl;
}

ContextField::~ContextField() {}

// ============================================================================
// MULTIMODAL INPUT INTEGRATION
// ============================================================================

void ContextField::update_from_vision(const VisionFrame& vf) {
    /**
     * Vision input → Context activation
     * 
     * For each detected object:
     * - Inject activation proportional to saliency
     * - Link to visual features
     * - Create co-occurrence edges
     */
    
    for (size_t i = 0; i < vf.object_nodes.size(); ++i) {
        uint64_t node_id = vf.object_nodes[i];
        float saliency = (i < vf.saliencies.size()) ? vf.saliencies[i] : 0.5f;
        
        // Inject activation (weighted by modality importance)
        activation_[node_id] += alpha_vision_ * saliency;
        activation_[node_id] = std::min(1.0f, activation_[node_id]);
        
        // Track persistence
        persistence_frames_[node_id]++;
    }
    
    // Inject feature nodes
    for (uint64_t feature_node : vf.feature_nodes) {
        activation_[feature_node] += alpha_vision_ * 0.3f;
    }
    
    // Create co-occurrence edges between simultaneously visible objects
    for (size_t i = 0; i < vf.object_nodes.size(); ++i) {
        for (size_t j = i + 1; j < vf.object_nodes.size(); ++j) {
            graph_.add_or_bump_edge(
                vf.object_nodes[i],
                vf.object_nodes[j],
                Rel::CO_OCCURS_WITH,
                0.1f
            );
        }
    }
}

void ContextField::update_from_audio(const AudioFrame& af) {
    /**
     * Audio input → Context activation
     */
    
    for (size_t i = 0; i < af.sound_nodes.size(); ++i) {
        uint64_t node_id = af.sound_nodes[i];
        float amplitude = (i < af.amplitudes.size()) ? af.amplitudes[i] : 0.5f;
        
        activation_[node_id] += alpha_audio_ * amplitude;
        activation_[node_id] = std::min(1.0f, activation_[node_id]);
        
        persistence_frames_[node_id]++;
    }
    
    // Link phonemes temporally (for speech)
    for (size_t i = 0; i + 1 < af.phoneme_nodes.size(); ++i) {
        graph_.add_temp_next(af.phoneme_nodes[i], af.phoneme_nodes[i+1]);
    }
}

void ContextField::update_from_text(const TextInput& text) {
    /**
     * Text/linguistic input → Context activation
     * Strongest signal (explicit semantic content)
     */
    
    for (uint64_t concept_node : text.concept_nodes) {
        activation_[concept_node] += alpha_text_ * 1.0f;  // Strong activation
        activation_[concept_node] = std::min(1.0f, activation_[concept_node]);
        
        persistence_frames_[concept_node]++;
    }
    
    // Relations from text are very strong
    for (size_t i = 0; i + 1 < text.relation_nodes.size(); i += 2) {
        graph_.add_or_bump_edge(
            text.relation_nodes[i],
            text.relation_nodes[i+1],
            Rel::NAMED_AS,
            0.2f
        );
    }
}

void ContextField::update_from_reasoning(uint64_t subject, uint64_t predicate, uint64_t object) {
    /**
     * Thought reinforces context
     * "fire causes heat" → Both fire and heat stay active
     */
    
    activation_[subject] += 0.4f;
    activation_[predicate] += 0.2f;
    activation_[object] += 0.4f;
    
    // Normalize
    for (auto& [_, act] : activation_) {
        act = std::min(1.0f, act);
    }
}

// ============================================================================
// CONTEXT DYNAMICS
// ============================================================================

void ContextField::spread_activation(float diffusion_rate) {
    /**
     * ASSOCIATIVE ACTIVATION SPREADING
     * 
     * Like Hopfield network dynamics:
     * Connected concepts co-activate, forming coherent attractor states
     */
    
    std::unordered_map<uint64_t, float> diffusion_buffer;
    
    // Spread from each active node
    for (const auto& [node_id, act] : activation_) {
        if (act < 0.1f) continue;  // Too weak to spread
        
        // Get semantic neighbors
        auto neighbors = graph_.neighbors(node_id, {
            Rel::CO_OCCURS_WITH,
            Rel::CAUSES,
            Rel::EXPECTS,
            Rel::USED_FOR,
            Rel::SIMILAR_FEATURES
        }, 30);
        
        // Spread to each neighbor
        for (uint64_t neighbor_id : neighbors) {
            float edge_weight = graph_.get_edge_weight(node_id, neighbor_id, Rel::CO_OCCURS_WITH);
            if (edge_weight == 0) edge_weight = 0.5f;  // Default strength
            
            float spread = act * edge_weight * diffusion_rate;
            diffusion_buffer[neighbor_id] += spread;
        }
    }
    
    // Apply diffused activations
    for (const auto& [node_id, diff] : diffusion_buffer) {
        activation_[node_id] += diff;
        activation_[node_id] = std::min(1.0f, activation_[node_id]);
    }
}

void ContextField::apply_decay(float dt) {
    /**
     * Exponential decay - forgetting
     * Unreinforced concepts fade from working memory
     */
    
    float decay_factor = std::exp(-0.005f * dt);  // Decay rate
    
    std::vector<uint64_t> to_remove;
    
    for (auto& [node_id, act] : activation_) {
        act *= decay_factor;
        
        // Remove if too weak
        if (act < 0.01f) {
            to_remove.push_back(node_id);
        }
    }
    
    for (uint64_t id : to_remove) {
        activation_.erase(id);
        persistence_frames_.erase(id);
    }
}

void ContextField::normalize() {
    float max_act = 0;
    for (const auto& [_, act] : activation_) {
        max_act = std::max(max_act, act);
    }
    
    if (max_act > 1.5f) {
        float scale = 1.0f / max_act;
        for (auto& [_, act] : activation_) {
            act *= scale;
        }
    }
}

void ContextField::settle(int max_iterations) {
    /**
     * Attractor dynamics - let context stabilize
     * Useful after major input (new scene, surprising event)
     */
    
    for (int i = 0; i < max_iterations; ++i) {
        spread_activation(0.03f);
        normalize();
        apply_decay(0.1f);
    }
}

// ============================================================================
// ATTENTION COMPUTATION (A,R,N,T,C)
// ============================================================================

std::vector<std::pair<uint64_t, FocusScores>> ContextField::compute_focus(float need_level) {
    /**
     * Compute A,R,N,T,C scores for all active nodes
     * 
     * This determines what Melvin "pays attention to"
     * Top-scoring nodes become conscious content
     */
    
    std::vector<std::pair<uint64_t, FocusScores>> scored_nodes;
    
    for (const auto& [node_id, _] : activation_) {
        FocusScores scores;
        
        // A: Appearance (sensory salience)
        scores.appearance = compute_appearance(node_id);
        
        // R: Relevance (goal matching)
        scores.relevance = compute_relevance(node_id);
        
        // N: Need (task urgency)
        scores.need = need_level * activation_[node_id];
        
        // T: Temporal (persistence)
        scores.temporal = compute_temporal(node_id);
        
        // C: Curiosity (novelty/prediction error)
        scores.curiosity = compute_curiosity(node_id);
        
        scored_nodes.push_back({node_id, scores});
    }
    
    // Sort by combined score
    std::sort(scored_nodes.begin(), scored_nodes.end(),
        [](const auto& a, const auto& b) {
            return a.second.combined() > b.second.combined();
        });
    
    return scored_nodes;
}

std::vector<uint64_t> ContextField::get_active_context(float threshold) const {
    std::vector<uint64_t> active;
    
    for (const auto& [node_id, act] : activation_) {
        if (act >= threshold) {
            active.push_back(node_id);
        }
    }
    
    return active;
}

FocusScores ContextField::get_scores(uint64_t node_id) const {
    FocusScores scores;
    
    auto it = activation_.find(node_id);
    if (it != activation_.end()) {
        scores.appearance = compute_appearance(node_id);
        scores.relevance = compute_relevance(node_id);
        scores.need = 0.5f;
        scores.temporal = compute_temporal(node_id);
        scores.curiosity = compute_curiosity(node_id);
    }
    
    return scores;
}

// ========================================================================
// PREDICTIVE CODING
// ============================================================================

std::vector<ContextField::Prediction> ContextField::predict_next_state() {
    /**
     * Generate predictions from active context
     * Uses EXPECTS edges to predict likely next observations
     */
    
    std::vector<Prediction> predictions;
    
    // Get top active nodes
    auto top_active = get_active_context(0.4f);
    
    for (uint64_t node_id : top_active) {
        // Find what this concept EXPECTS
        auto expected = graph_.neighbors(node_id, {Rel::EXPECTS}, 10);
        
        for (uint64_t exp_node : expected) {
            Prediction pred;
            pred.predicted_node = exp_node;
            pred.confidence = activation_[node_id] * 0.7f;  // Moderate confidence
            pred.expected_time = 0.5;  // Within 0.5 seconds
            pred.basis_nodes = {node_id};
            
            predictions.push_back(pred);
        }
    }
    
    active_predictions_ = predictions;
    return predictions;
}

float ContextField::compute_prediction_error(const std::vector<uint64_t>& actual_nodes) {
    /**
     * Compare predictions to reality
     * High error = surprise = learning opportunity
     */
    
    if (active_predictions_.empty()) {
        return 0.2f;  // No predictions = moderate novelty
    }
    
    float total_error = 0;
    int comparisons = 0;
    
    // Check each prediction
    for (const auto& pred : active_predictions_) {
        bool matched = false;
        
        for (uint64_t actual : actual_nodes) {
            if (actual == pred.predicted_node) {
                // Correct prediction!
                total_error += 0.0f;
                matched = true;
                break;
            }
        }
        
        if (!matched) {
            // Prediction failed!
            total_error += pred.confidence;  // Error proportional to confidence
        }
        
        comparisons++;
    }
    
    // Also check for unpredicted inputs (surprise!)
    for (uint64_t actual : actual_nodes) {
        if (!matches_prediction(actual)) {
            total_error += 0.3f;  // Unexpected input
            comparisons++;
        }
    }
    
    float avg_error = comparisons > 0 ? total_error / comparisons : 0;
    cumulative_prediction_error_ = cumulative_prediction_error_ * 0.9f + avg_error * 0.1f;
    
    return avg_error;
}

void ContextField::learn_from_error(float error, const std::vector<uint64_t>& actual_nodes) {
    /**
     * Prediction error drives learning
     * 
     * High error → Form LEAP edges between unexpected co-occurrences
     * Low error → Strengthen existing predictions
     */
    
    if (error > 0.15f) {
        // Significant error - form new connections
        auto active = get_active_context(0.5f);
        
        for (uint64_t active_node : active) {
            for (uint64_t actual_node : actual_nodes) {
                // Maybe form LEAP between active concept and surprising input
                graph_.maybe_form_leap(active_node, actual_node, error);
            }
        }
    } else if (error < 0.05f) {
        // Low error - reinforce correct predictions
        for (const auto& pred : active_predictions_) {
            for (uint64_t actual : actual_nodes) {
                if (actual == pred.predicted_node) {
                    // Correct! Strengthen
                    graph_.reinforce(pred.basis_nodes[0], actual, Rel::EXPECTS, 0.1f);
                }
            }
        }
    }
}

// ============================================================================
// EPISODIC MEMORY
// ============================================================================

ContextField::ContextSnapshot ContextField::create_snapshot() {
    /**
     * Capture current context state
     * Like taking a mental photograph of "now"
     */
    
    ContextSnapshot snap;
    snap.snapshot_id = cycle_count_;
    snap.timestamp = std::chrono::duration<double>(
        std::chrono::steady_clock::now() - last_update_
    ).count();
    
    // Categorize active nodes by type
    for (const auto& [node_id, act] : activation_) {
        if (act < 0.2f) continue;
        
        const Node* node = graph_.get_node(node_id);
        if (!node) continue;
        
        snap.activations.push_back({node_id, act});
        
        // Categorize by type
        switch (node->type) {
            case NodeType::VISUAL_PATCH:
            case NodeType::OBJECT:
                snap.visual_nodes.push_back(node_id);
                break;
            case NodeType::EVENT:
                snap.audio_nodes.push_back(node_id);  // Approximate
                break;
            case NodeType::CONCEPT:
                snap.concept_nodes.push_back(node_id);
                break;
            default:
                break;
        }
    }
    
    // Generate description
    std::ostringstream desc;
    desc << snap.visual_nodes.size() << " visual, "
         << snap.audio_nodes.size() << " audio, "
         << snap.concept_nodes.size() << " concepts active";
    snap.scene_description = desc.str();
    
    return snap;
}

void ContextField::store_episode(const ContextSnapshot& snapshot) {
    /**
     * Store as episodic memory
     * Creates SceneNode linked to all active nodes
     */
    
    // Create scene node
    uint64_t scene_node = graph_.create_object(0, 0);  // Use as scene marker
    
    // Link to all active nodes
    for (const auto& [node_id, activation] : snapshot.activations) {
        graph_.add_or_bump_edge(scene_node, node_id, Rel::PART_OF, activation);
    }
    
    // Add to recent snapshots
    recent_snapshots_.push_back(snapshot);
    if (recent_snapshots_.size() > 100) {
        recent_snapshots_.pop_front();
    }
}

std::vector<ContextField::ContextSnapshot> ContextField::recall_similar_contexts(
    const ContextSnapshot& query,
    int top_k)
{
    /**
     * Find similar past contexts
     * Like episodic memory recall
     */
    
    std::vector<std::pair<ContextSnapshot, float>> scored;
    
    for (const auto& past : recent_snapshots_) {
        // Compute overlap
        float overlap = 0;
        for (const auto& [node_id, _] : query.activations) {
            for (const auto& [past_id, past_act] : past.activations) {
                if (node_id == past_id) {
                    overlap += past_act;
                }
            }
        }
        
        scored.push_back({past, overlap});
    }
    
    // Sort by overlap
    std::sort(scored.begin(), scored.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return top-k
    std::vector<ContextSnapshot> result;
    for (int i = 0; i < std::min(top_k, (int)scored.size()); ++i) {
        result.push_back(scored[i].first);
    }
    
    return result;
}

// ============================================================================
// MODALITY WEIGHTS
// ============================================================================

void ContextField::set_modality_weights(float vision, float audio, float text) {
    alpha_vision_ = vision;
    alpha_audio_ = audio;
    alpha_text_ = text;
    
    // Normalize to sum to 1.0
    float total = alpha_vision_ + alpha_audio_ + alpha_text_;
    if (total > 0) {
        alpha_vision_ /= total;
        alpha_audio_ /= total;
        alpha_text_ /= total;
    }
}

void ContextField::compute_adaptive_weights() {
    /**
     * Adapt modality weights based on environment
     * 
     * Examples:
     * - Dark scene → boost audio
     * - Noisy environment → boost vision
     * - Reading text → boost text
     */
    
    // Count active nodes per modality
    int visual_count = 0, audio_count = 0, concept_count = 0;
    
    for (const auto& [node_id, act] : activation_) {
        const Node* node = graph_.get_node(node_id);
        if (!node) continue;
        
        if (node->type == NodeType::VISUAL_PATCH || node->type == NodeType::OBJECT) {
            visual_count++;
        } else if (node->type == NodeType::EVENT) {
            audio_count++;
        } else if (node->type == NodeType::CONCEPT) {
            concept_count++;
        }
    }
    
    // Adaptive weighting (boost active modalities)
    int total = visual_count + audio_count + concept_count;
    if (total > 0) {
        alpha_vision_ = 0.5f + 0.3f * (float)visual_count / total;
        alpha_audio_ = 0.3f + 0.2f * (float)audio_count / total;
        alpha_text_ = 0.2f + 0.2f * (float)concept_count / total;
        
        // Normalize
        float sum = alpha_vision_ + alpha_audio_ + alpha_text_;
        alpha_vision_ /= sum;
        alpha_audio_ /= sum;
        alpha_text_ /= sum;
    }
}

// ============================================================================
// QUERIES & VISUALIZATION
// ============================================================================

std::string ContextField::describe_current_context() const {
    std::ostringstream oss;
    
    auto active = get_active_context(0.3f);
    
    oss << "Context (" << active.size() << " active): ";
    
    int shown = 0;
    for (uint64_t node_id : active) {
        if (shown >= 5) break;
        
        const Node* node = graph_.get_node(node_id);
        if (node) {
            oss << "node_" << node_id << " ";
        }
        shown++;
    }
    
    return oss.str();
}

void ContextField::visualize_context(int top_n) const {
    auto active = get_active_context(0.2f);
    
    std::cout << "\n╔════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 Multimodal Context Field (Working Memory)              ║\n";
    std::cout << "╚════════════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "  Active nodes: " << active.size() << "\n";
    std::cout << "  Predictions: " << active_predictions_.size() << "\n";
    std::cout << "  Avg error: " << std::fixed << std::setprecision(3) 
              << cumulative_prediction_error_ << "\n\n";
    
    std::cout << "  Top " << top_n << " active nodes:\n";
    std::cout << "  Node ID          | Activation | A     R     N     T     C\n";
    std::cout << "  ─────────────────────────────────────────────────────────\n";
    
    // Sort by activation
    std::vector<std::pair<uint64_t, float>> sorted_active;
    for (uint64_t id : active) {
        sorted_active.push_back({id, activation_.at(id)});
    }
    std::sort(sorted_active.begin(), sorted_active.end(),
        [](const auto& a, const auto& b) { return a.second > b.second; });
    
    for (int i = 0; i < std::min(top_n, (int)sorted_active.size()); ++i) {
        auto [node_id, act] = sorted_active[i];
        auto scores = get_scores(node_id);
        
        std::cout << "  " << std::setw(16) << node_id << " | "
                  << std::fixed << std::setprecision(2) << act << "       | "
                  << scores.appearance << " " << scores.relevance << " "
                  << scores.need << " " << scores.temporal << " "
                  << scores.curiosity << "\n";
    }
    
    std::cout << std::endl;
}

ContextField::ContextStats ContextField::get_stats() const {
    ContextStats s;
    
    // Count by type
    for (const auto& [node_id, act] : activation_) {
        s.total_active_nodes++;
        
        const Node* node = graph_.get_node(node_id);
        if (node) {
            if (node->type == NodeType::VISUAL_PATCH || node->type == NodeType::OBJECT) {
                s.visual_nodes++;
            } else if (node->type == NodeType::EVENT) {
                s.audio_nodes++;
            } else if (node->type == NodeType::CONCEPT) {
                s.concept_nodes++;
            }
        }
        
        s.avg_activation += act;
        s.max_activation = std::max(s.max_activation, act);
    }
    
    if (s.total_active_nodes > 0) {
        s.avg_activation /= s.total_active_nodes;
    }
    
    s.num_predictions = active_predictions_.size();
    s.avg_prediction_error = cumulative_prediction_error_;
    
    return s;
}

void ContextField::print_stats() const {
    auto s = get_stats();
    
    std::cout << "\n╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  Multimodal Context Statistics                           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    std::cout << "  Total active:    " << s.total_active_nodes << "\n";
    std::cout << "  Visual nodes:    " << s.visual_nodes << "\n";
    std::cout << "  Audio nodes:     " << s.audio_nodes << "\n";
    std::cout << "  Concept nodes:   " << s.concept_nodes << "\n";
    std::cout << "  Avg activation:  " << std::fixed << std::setprecision(3) << s.avg_activation << "\n";
    std::cout << "  Max activation:  " << s.max_activation << "\n";
    std::cout << "  Predictions:     " << s.num_predictions << "\n";
    std::cout << "  Avg pred error:  " << s.avg_prediction_error << "\n";
    std::cout << "  Modality weights: V=" << alpha_vision_ 
              << " A=" << alpha_audio_ 
              << " T=" << alpha_text_ << "\n";
    std::cout << std::endl;
}

// ============================================================================
// SCORE COMPUTATION HELPERS
// ============================================================================

float ContextField::compute_appearance(uint64_t node_id) const {
    // Appearance = current activation (sensory signal strength)
    auto it = activation_.find(node_id);
    return it != activation_.end() ? it->second : 0.0f;
}

float ContextField::compute_relevance(uint64_t node_id) const {
    // Relevance = connection to focus anchors
    if (focus_anchors_.empty()) {
        return 0.3f;  // No anchors, moderate relevance
    }
    
    float max_relevance = 0;
    for (uint64_t anchor : focus_anchors_) {
        float edge_weight = graph_.get_edge_weight(anchor, node_id, Rel::CO_OCCURS_WITH);
        max_relevance = std::max(max_relevance, edge_weight);
    }
    
    return max_relevance;
}

float ContextField::compute_temporal(uint64_t node_id) const {
    // Temporal = how long has this been active
    auto it = persistence_frames_.find(node_id);
    if (it == persistence_frames_.end()) {
        return 0.0f;
    }
    
    // Normalize to [0, 1]
    return std::min(1.0f, it->second / 30.0f);
}

float ContextField::compute_curiosity(uint64_t node_id) const {
    // Curiosity = unpredicted or high prediction error
    if (matches_prediction(node_id)) {
        return 0.1f;  // Predicted, low curiosity
    }
    
    // Not predicted = novel!
    return 0.7f;
}

bool ContextField::matches_prediction(uint64_t node_id) const {
    for (const auto& pred : active_predictions_) {
        if (pred.predicted_node == node_id) {
            return true;
        }
    }
    return false;
}

} // namespace melvin

