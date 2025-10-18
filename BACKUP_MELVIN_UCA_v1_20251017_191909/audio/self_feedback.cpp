#include "self_feedback.h"
#include <iostream>
#include <cmath>
#include <algorithm>

namespace melvin {
namespace audio {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

SelfFeedback::SelfFeedback()
    : SelfFeedback(Config()) {
}

SelfFeedback::SelfFeedback(const Config& config)
    : config_(config) {
    
    std::cout << "🔄 SelfFeedback initialized (auditory self-monitoring)" << std::endl;
    std::cout << "   Similarity threshold: " << config_.similarity_threshold << std::endl;
    std::cout << "   Auto-adaptation: " << (config_.enable_auto_adaptation ? "enabled" : "disabled") << std::endl;
}

SelfFeedback::~SelfFeedback() {
}

// ============================================================================
// FEEDBACK LOOP
// ============================================================================

SelfFeedback::FeedbackResult SelfFeedback::compare(
    const std::vector<AudioToken>& intended_tokens,
    const std::vector<AudioToken>& heard_tokens) {
    
    FeedbackResult result;
    
    if (intended_tokens.empty() || heard_tokens.empty()) {
        return result;
    }
    
    // Compute average similarity across all tokens
    result.similarity = compute_average_similarity(intended_tokens, heard_tokens);
    
    // Compute pitch and energy errors
    float intended_pitch = 0.0f, heard_pitch = 0.0f;
    float intended_energy = 0.0f, heard_energy = 0.0f;
    
    for (const auto& token : intended_tokens) {
        intended_pitch += token.pitch;
        intended_energy += token.energy;
    }
    intended_pitch /= intended_tokens.size();
    intended_energy /= intended_tokens.size();
    
    for (const auto& token : heard_tokens) {
        heard_pitch += token.pitch;
        heard_energy += token.energy;
    }
    heard_pitch /= heard_tokens.size();
    heard_energy /= heard_tokens.size();
    
    result.pitch_error = std::abs(intended_pitch - heard_pitch);
    result.energy_error = std::abs(intended_energy - heard_energy);
    
    // Determine if adjustment needed
    result.needs_adjustment = 
        result.similarity < config_.similarity_threshold ||
        result.pitch_error > config_.pitch_tolerance ||
        result.energy_error > config_.energy_tolerance;
    
    // Compute suggested adjustments
    if (result.needs_adjustment) {
        result.pitch_adjustment = (heard_pitch - intended_pitch) * config_.adaptation_rate;
        
        std::cout << "   ⚠️  Mismatch detected:" << std::endl;
        std::cout << "      Similarity: " << result.similarity << std::endl;
        std::cout << "      Pitch error: " << result.pitch_error << " Hz" << std::endl;
        std::cout << "      Suggested adjustment: " << result.pitch_adjustment << " Hz" << std::endl;
    } else {
        std::cout << "   ✅ Good match (similarity: " << result.similarity << ")" << std::endl;
    }
    
    record_feedback(result);
    
    return result;
}

SelfFeedback::FeedbackResult SelfFeedback::compare_token(
    const AudioToken& intended,
    const AudioToken& heard) {
    
    FeedbackResult result;
    
    result.similarity = compute_token_similarity(intended, heard);
    result.pitch_error = std::abs(intended.pitch - heard.pitch);
    result.energy_error = std::abs(intended.energy - heard.energy);
    
    result.needs_adjustment = result.similarity < config_.similarity_threshold;
    
    if (result.needs_adjustment) {
        result.pitch_adjustment = (heard.pitch - intended.pitch) * config_.adaptation_rate;
    }
    
    return result;
}

bool SelfFeedback::apply_adjustments(const FeedbackResult& result,
                                    VocalEngine& vocal_engine) {
    if (!result.needs_adjustment || !config_.enable_auto_adaptation) {
        return false;
    }
    
    std::cout << "\n🔧 Applying vocal adjustments..." << std::endl;
    
    // Get current config
    auto config = vocal_engine.get_config();
    
    // Adjust pitch
    float new_pitch = config.base_pitch - result.pitch_adjustment;  // Negative feedback
    vocal_engine.set_pitch(new_pitch);
    
    std::cout << "   Pitch: " << config.base_pitch << " → " << new_pitch << " Hz" << std::endl;
    
    adjustment_count_++;
    
    return true;
}

// ============================================================================
// SELF-MONITORING
// ============================================================================

void SelfFeedback::create_feedback_link(uint64_t intended_node,
                                        uint64_t heard_node,
                                        float similarity,
                                        AtomicGraph& graph) {
    // Create bidirectional feedback link
    graph.add_edge(intended_node, heard_node, Relation::OBSERVED_AS, similarity);
    graph.add_edge(heard_node, intended_node, Relation::INSTANCE_OF, similarity);
    
    std::cout << "   🔗 Feedback link: intended(" << intended_node 
              << ") ↔ heard(" << heard_node << ") [" << similarity << "]" << std::endl;
}

void SelfFeedback::record_feedback(const FeedbackResult& result) {
    history_.push_back(result);
    
    // Keep history limited
    if (history_.size() > 1000) {
        history_.erase(history_.begin());
    }
}

// ============================================================================
// ANALYSIS
// ============================================================================

float SelfFeedback::get_average_quality() const {
    if (history_.empty()) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    for (const auto& result : history_) {
        sum += result.similarity;
    }
    
    return sum / history_.size();
}

bool SelfFeedback::is_improving() const {
    if (history_.size() < 10) {
        return false;
    }
    
    // Compare first half to second half
    size_t mid = history_.size() / 2;
    
    float first_half = 0.0f;
    for (size_t i = 0; i < mid; i++) {
        first_half += history_[i].similarity;
    }
    first_half /= mid;
    
    float second_half = 0.0f;
    for (size_t i = mid; i < history_.size(); i++) {
        second_half += history_[i].similarity;
    }
    second_half /= (history_.size() - mid);
    
    return second_half > first_half;
}

// ============================================================================
// STATISTICS
// ============================================================================

void SelfFeedback::print_stats() const {
    std::cout << "\n📊 SelfFeedback Statistics:" << std::endl;
    std::cout << "   Total feedback loops: " << history_.size() << std::endl;
    std::cout << "   Adjustments applied: " << adjustment_count_ << std::endl;
    std::cout << "   Average quality: " << get_average_quality() << std::endl;
    std::cout << "   Improving: " << (is_improving() ? "Yes ✅" : "Not yet") << std::endl;
}

// ============================================================================
// HELPER METHODS
// ============================================================================

float SelfFeedback::compute_token_similarity(const AudioToken& t1, const AudioToken& t2) {
    if (t1.features.empty() || t2.features.empty()) {
        return 0.0f;
    }
    
    // Cosine similarity
    size_t dim = std::min(t1.features.size(), t2.features.size());
    
    float dot = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;
    
    for (size_t i = 0; i < dim; i++) {
        dot += t1.features[i] * t2.features[i];
        norm1 += t1.features[i] * t1.features[i];
        norm2 += t2.features[i] * t2.features[i];
    }
    
    if (norm1 > 0 && norm2 > 0) {
        return dot / (std::sqrt(norm1) * std::sqrt(norm2));
    }
    
    return 0.0f;
}

float SelfFeedback::compute_average_similarity(const std::vector<AudioToken>& intended,
                                              const std::vector<AudioToken>& heard) {
    if (intended.empty() || heard.empty()) {
        return 0.0f;
    }
    
    float sum = 0.0f;
    size_t count = std::min(intended.size(), heard.size());
    
    for (size_t i = 0; i < count; i++) {
        sum += compute_token_similarity(intended[i], heard[i]);
    }
    
    return sum / count;
}

} // namespace audio
} // namespace melvin


