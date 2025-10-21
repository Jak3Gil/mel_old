#pragma once

#include "audio_tokenizer.h"
#include "vocal_engine.h"
#include "../core/atomic_graph.h"
#include <vector>

namespace melvin {
namespace audio {

/**
 * SelfFeedback - Auditory self-monitoring and adaptation
 * 
 * When Melvin speaks:
 * 1. VocalEngine generates audio
 * 2. Microphone captures output
 * 3. AudioTokenizer processes captured audio
 * 4. Compare intended vs actual audio
 * 5. Adjust vocal parameters if mismatch
 * 
 * This creates a feedback loop like biological speech learning:
 * - Babbling → hearing → adjustment → improvement
 */
class SelfFeedback {
public:
    /**
     * Feedback comparison result
     */
    struct FeedbackResult {
        float similarity;               // How similar (0.0 to 1.0)
        float pitch_error;              // Pitch difference (Hz)
        float energy_error;             // Energy difference
        bool needs_adjustment;          // Should adapt vocal params
        
        // Suggested adjustments
        float pitch_adjustment;         // Delta pitch (Hz)
        float formant_adjustment[3];    // Delta formants
        
        FeedbackResult() 
            : similarity(0), pitch_error(0), energy_error(0),
              needs_adjustment(false), pitch_adjustment(0) {
            formant_adjustment[0] = formant_adjustment[1] = formant_adjustment[2] = 0;
        }
    };
    
    /**
     * Configuration
     */
    struct Config {
        float similarity_threshold = 0.8f;   // Min similarity to consider "good"
        float pitch_tolerance = 10.0f;       // Acceptable pitch error (Hz)
        float energy_tolerance = 0.1f;       // Acceptable energy error
        float adaptation_rate = 0.1f;        // How fast to adapt (0.0 to 1.0)
        bool enable_auto_adaptation = true;  // Auto-adjust vocal params
    };
    
    SelfFeedback();
    explicit SelfFeedback(const Config& config);
    ~SelfFeedback();
    
    // ========================================================================
    // FEEDBACK LOOP
    // ========================================================================
    
    /**
     * Compare intended speech with heard audio
     * 
     * @param intended_tokens Tokens from what Melvin meant to say
     * @param heard_tokens Tokens from microphone capture
     * @return Feedback result with similarity and adjustments
     */
    FeedbackResult compare(const std::vector<AudioToken>& intended_tokens,
                          const std::vector<AudioToken>& heard_tokens);
    
    /**
     * Simplified comparison (single token)
     */
    FeedbackResult compare_token(const AudioToken& intended,
                                const AudioToken& heard);
    
    /**
     * Apply feedback to vocal engine
     * 
     * @param result Feedback result
     * @param vocal_engine Engine to adjust
     * @return true if adjustments applied
     */
    bool apply_adjustments(const FeedbackResult& result,
                          VocalEngine& vocal_engine);
    
    // ========================================================================
    // SELF-MONITORING
    // ========================================================================
    
    /**
     * Monitor speech output for quality
     * 
     * Creates graph edges linking intended → actual audio
     */
    void create_feedback_link(uint64_t intended_node,
                             uint64_t heard_node,
                             float similarity,
                             AtomicGraph& graph);
    
    /**
     * Record feedback history
     */
    void record_feedback(const FeedbackResult& result);
    
    // ========================================================================
    // ANALYSIS
    // ========================================================================
    
    /**
     * Get average speech quality over time
     */
    float get_average_quality() const;
    
    /**
     * Check if speech is improving
     */
    bool is_improving() const;
    
    /**
     * Get feedback history
     */
    const std::vector<FeedbackResult>& get_history() const { return history_; }
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    size_t get_feedback_count() const { return history_.size(); }
    size_t get_adjustment_count() const { return adjustment_count_; }
    
    void print_stats() const;
    
private:
    Config config_;
    
    std::vector<FeedbackResult> history_;
    size_t adjustment_count_ = 0;
    
    // Helper methods
    float compute_token_similarity(const AudioToken& t1, const AudioToken& t2);
    float compute_average_similarity(const std::vector<AudioToken>& intended,
                                    const std::vector<AudioToken>& heard);
};

} // namespace audio
} // namespace melvin


