/*
 * AttentionManager - Cognitive gatekeeper for focus selection
 */

#include "attention_manager.h"
#include <iostream>
#include <iomanip>

namespace melvin {

AttentionManager::AttentionManager() 
    : state_(MelvinState::BALANCED)
    , decay_rate_(0.1f)
{
    update_weights_for_state();
}

AttentionManager::~AttentionManager() {
}

// ============================================================================
// CANDIDATE MANAGEMENT
// ============================================================================

void AttentionManager::add_candidate(const FocusCandidate& candidate) {
    candidates_.push_back(candidate);
}

void AttentionManager::clear_candidates() {
    candidates_.clear();
}

// ============================================================================
// FOCUS SELECTION
// ============================================================================

FocusCandidate AttentionManager::select_focus_target(double current_time) {
    if (candidates_.empty()) {
        // No candidates, return empty focus
        FocusCandidate empty;
        current_focus_ = empty;
        return empty;
    }
    
    // Compute focus scores for all candidates
    for (auto& candidate : candidates_) {
        // Apply temporal decay if candidate is old
        apply_temporal_decay(candidate, current_time);
        
        // Compute focus score
        candidate.focus_score = compute_focus_score(candidate);
    }
    
    // Normalize scores to 0-1 range
    normalize_scores(candidates_);
    
    // Select candidate with highest score
    auto best = std::max_element(candidates_.begin(), candidates_.end(),
        [](const FocusCandidate& a, const FocusCandidate& b) {
            return a.focus_score < b.focus_score;
        });
    
    if (best != candidates_.end()) {
        current_focus_ = *best;
        
        // Add to history
        focus_history_.push_back(current_focus_);
        if (focus_history_.size() > MAX_HISTORY) {
            focus_history_.erase(focus_history_.begin());
        }
        
        return current_focus_;
    }
    
    // Fallback: no valid focus
    FocusCandidate empty;
    current_focus_ = empty;
    return empty;
}

// ============================================================================
// STATE MANAGEMENT
// ============================================================================

void AttentionManager::set_state(MelvinState state) {
    state_ = state;
    update_weights_for_state();
}

void AttentionManager::set_weights(const AttentionWeights& weights) {
    weights_ = weights;
    weights_.normalize();
}

// ============================================================================
// METRICS
// ============================================================================

std::vector<FocusCandidate> AttentionManager::get_focus_history(int count) const {
    int start = std::max(0, (int)focus_history_.size() - count);
    return std::vector<FocusCandidate>(
        focus_history_.begin() + start,
        focus_history_.end()
    );
}

void AttentionManager::print_status() const {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  🎯 ATTENTION MANAGER STATUS                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "State: ";
    switch (state_) {
        case MelvinState::EXPLORING:       std::cout << "EXPLORING"; break;
        case MelvinState::TASK_FOCUSED:    std::cout << "TASK_FOCUSED"; break;
        case MelvinState::LOW_ENERGY:      std::cout << "LOW_ENERGY"; break;
        case MelvinState::OVERSTIMULATED:  std::cout << "OVERSTIMULATED"; break;
        case MelvinState::BALANCED:        std::cout << "BALANCED"; break;
    }
    std::cout << "\n\n";
    
    std::cout << "Weights:\n";
    std::cout << "  wA (Salience):   " << std::fixed << std::setprecision(2) << weights_.wA << "\n";
    std::cout << "  wR (Relevance):  " << weights_.wR << "\n";
    std::cout << "  wN (Need):       " << weights_.wN << "\n";
    std::cout << "  wT (Temporal):   " << weights_.wT << "\n";
    std::cout << "  wC (Curiosity):  " << weights_.wC << "\n\n";
    
    if (has_focus()) {
        std::cout << "Current Focus:\n";
        std::cout << "  Target: " << current_focus_.label << "\n";
        std::cout << "  Score: " << current_focus_.focus_score << "\n";
        std::cout << "  Source: " << current_focus_.source << "\n\n";
    } else {
        std::cout << "Current Focus: NONE\n\n";
    }
    
    std::cout << "Candidates: " << candidates_.size() << "\n";
    std::cout << "History size: " << focus_history_.size() << "\n\n";
}

// ============================================================================
// INTERNAL METHODS
// ============================================================================

float AttentionManager::compute_focus_score(const FocusCandidate& candidate) const {
    // F = (A * wA) + (R * wR) + (N * wN) + (T * wT) + (C * wC)
    float F = 
        (candidate.A * weights_.wA) +
        (candidate.R * weights_.wR) +
        (candidate.N * weights_.wN) +
        (candidate.T * weights_.wT) +
        (candidate.C * weights_.wC);
    
    // Clamp to [0, 1]
    return std::max(0.0f, std::min(1.0f, F));
}

void AttentionManager::apply_temporal_decay(FocusCandidate& candidate, double current_time) const {
    double dt = current_time - candidate.timestamp;
    if (dt > 0) {
        // F(t+1) = F(t) * exp(-k * Δt)
        float decay_factor = std::exp(-decay_rate_ * dt);
        candidate.focus_score *= decay_factor;
    }
}

void AttentionManager::normalize_scores(std::vector<FocusCandidate>& candidates) const {
    if (candidates.empty()) return;
    
    // Find min and max
    float min_score = candidates[0].focus_score;
    float max_score = candidates[0].focus_score;
    
    for (const auto& c : candidates) {
        if (c.focus_score < min_score) min_score = c.focus_score;
        if (c.focus_score > max_score) max_score = c.focus_score;
    }
    
    // Normalize to 0-1
    float range = max_score - min_score;
    if (range > 0.001f) {
        for (auto& c : candidates) {
            c.focus_score = (c.focus_score - min_score) / range;
        }
    }
}

void AttentionManager::update_weights_for_state() {
    switch (state_) {
        case MelvinState::EXPLORING:
            // Increase curiosity and salience
            weights_.wA = 0.35f;
            weights_.wR = 0.15f;
            weights_.wN = 0.15f;
            weights_.wT = 0.05f;
            weights_.wC = 0.30f;  // High curiosity!
            break;
            
        case MelvinState::TASK_FOCUSED:
            // Increase relevance, decrease distractions
            weights_.wA = 0.20f;  // Lower sensory
            weights_.wR = 0.50f;  // High relevance!
            weights_.wN = 0.15f;
            weights_.wT = 0.10f;
            weights_.wC = 0.05f;  // Low curiosity
            break;
            
        case MelvinState::LOW_ENERGY:
            // Prioritize needs
            weights_.wA = 0.20f;
            weights_.wR = 0.20f;
            weights_.wN = 0.50f;  // High need!
            weights_.wT = 0.05f;
            weights_.wC = 0.05f;
            break;
            
        case MelvinState::OVERSTIMULATED:
            // Filter out noise, focus on relevant
            weights_.wA = 0.15f;  // Very low salience
            weights_.wR = 0.50f;  // High relevance
            weights_.wN = 0.20f;
            weights_.wT = 0.10f;
            weights_.wC = 0.05f;
            break;
            
        case MelvinState::BALANCED:
        default:
            // Default balanced weights
            weights_.wA = 0.40f;
            weights_.wR = 0.30f;
            weights_.wN = 0.20f;
            weights_.wT = 0.05f;
            weights_.wC = 0.05f;
            break;
    }
    
    weights_.normalize();
}

} // namespace melvin

