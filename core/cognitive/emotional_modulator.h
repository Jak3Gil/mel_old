/**
 * @file emotional_modulator.h
 * @brief Limbic-inspired emotional modulation of speech and behavior
 * 
 * Modulates prosody, word choice, and response style based on
 * neuromodulator-like parameters (confidence, novelty, coherence).
 */

#ifndef MELVIN_EMOTIONAL_MODULATOR_H
#define MELVIN_EMOTIONAL_MODULATOR_H

#include <string>
#include <vector>
#include <unordered_set>

namespace melvin {
namespace cognitive {

/**
 * @brief Prosody parameters for speech generation
 */
struct ProsodyParams {
    float tempo_scale;        // Speech rate multiplier (0.5 - 2.0)
    float pitch_variance;     // Emotional intensity (0.0 - 2.0)
    float hedge_probability;  // Uncertainty markers (0.0 - 1.0)
    float pause_duration;     // Thinking pauses (0.0 - 1.0 seconds)
    
    ProsodyParams()
        : tempo_scale(1.0f)
        , pitch_variance(1.0f)
        , hedge_probability(0.0f)
        , pause_duration(0.2f)
    {}
};

/**
 * @brief Emotional modulator
 * 
 * Maps cognitive state to linguistic/prosodic output:
 * - High novelty → faster tempo, exploratory language
 * - Low confidence → hedging, slower tempo
 * - High coherence → calm, precise language
 */
class EmotionalModulator {
public:
    EmotionalModulator();
    ~EmotionalModulator() = default;
    
    /**
     * @brief Compute prosody from cognitive state
     * 
     * @param confidence How certain MELVIN is (0-1)
     * @param novelty How surprising the situation is (0-1)
     * @param coherence How well-connected ideas are (0-1)
     * @return Prosody parameters for speech synthesis
     */
    ProsodyParams compute_prosody(
        float confidence,
        float novelty,
        float coherence
    );
    
    /**
     * @brief Add hedging language based on confidence
     * 
     * Low confidence → "I think...", "Maybe...", "I'm not sure but..."
     * High confidence → No modification
     * 
     * @param answer Original answer string
     * @param hedge_prob Hedging probability (0-1)
     * @return Modified answer with appropriate hedging
     */
    std::string add_hedging(
        const std::string& answer, 
        float hedge_prob
    );
    
    /**
     * @brief Select vocabulary based on mood/state
     * 
     * @param confidence Current confidence level
     * @param novelty Current novelty level
     * @return List of appropriate connector words
     */
    std::vector<std::string> select_connectors(
        float confidence,
        float novelty
    );
    
    /**
     * @brief Add emotional tone to answer
     * 
     * Adds emphasis, enthusiasm, or caution based on state
     */
    std::string add_emotional_tone(
        const std::string& answer,
        float confidence,
        float novelty
    );
    
    /**
     * @brief Get current emotional state description
     */
    std::string get_emotional_state(
        float confidence,
        float novelty,
        float coherence
    );
    
    // Genome-configurable parameters
    void set_base_tempo(float tempo) { base_tempo_ = tempo; }
    void set_novelty_tempo_scale(float scale) { novelty_tempo_scale_ = scale; }
    void set_confidence_tempo_scale(float scale) { confidence_tempo_scale_ = scale; }
    void set_arousal_pitch_scale(float scale) { arousal_pitch_scale_ = scale; }
    void set_hedge_confidence_threshold(float thresh) { hedge_confidence_threshold_ = thresh; }
    
private:
    // Genome parameters
    float base_tempo_ = 1.0f;
    float novelty_tempo_scale_ = 0.6f;
    float confidence_tempo_scale_ = 0.3f;
    float arousal_pitch_scale_ = 1.5f;
    float hedge_confidence_threshold_ = 0.4f;
    
    // Hedging phrase pools
    std::vector<std::string> low_confidence_hedges_;
    std::vector<std::string> medium_confidence_hedges_;
    
    // Connector word pools
    std::vector<std::string> confident_connectors_;
    std::vector<std::string> exploratory_connectors_;
    std::vector<std::string> neutral_connectors_;
    
    // Helper to select random element from vector
    template<typename T>
    const T& random_choice(const std::vector<T>& vec);
};

} // namespace cognitive
} // namespace melvin

#endif // MELVIN_EMOTIONAL_MODULATOR_H

