/**
 * @file emotional_modulator.cpp
 * @brief Implementation of emotional modulator
 */

#include "emotional_modulator.h"
#include <random>
#include <cmath>

namespace melvin {
namespace cognitive {

EmotionalModulator::EmotionalModulator() {
    // Initialize hedging phrases
    low_confidence_hedges_ = {
        "I'm not entirely sure, but ",
        "I think perhaps ",
        "It's possible that ",
        "My understanding is limited, but "
    };
    
    medium_confidence_hedges_ = {
        "I believe ",
        "I think ",
        "It seems that ",
        "Likely "
    };
    
    // Initialize connector words
    confident_connectors_ = {
        "clearly", "definitely", "precisely", "exactly",
        "certainly", "obviously", "undoubtedly"
    };
    
    exploratory_connectors_ = {
        "perhaps", "might", "could be", "possibly",
        "potentially", "maybe", "interesting"
    };
    
    neutral_connectors_ = {
        "relates to", "connects with", "involves", "includes",
        "associated with", "linked to"
    };
}

ProsodyParams EmotionalModulator::compute_prosody(
    float confidence,
    float novelty,
    float coherence
) {
    ProsodyParams prosody;
    
    // Tempo based on novelty and confidence (genome-configurable)
    // High novelty = faster, excited speech
    // Low confidence = slower, deliberate speech
    float tempo_from_novelty = base_tempo_ + (novelty * novelty_tempo_scale_);
    float tempo_from_confidence = base_tempo_ + (confidence * confidence_tempo_scale_);
    prosody.tempo_scale = (tempo_from_novelty + tempo_from_confidence) / 2.0f;
    
    // Pitch variance based on emotional arousal (genome-configurable)
    // High novelty + low coherence = high arousal
    float arousal = novelty * (1.0f - coherence);
    prosody.pitch_variance = 0.5f + arousal * arousal_pitch_scale_;
    
    // Hedging based on confidence (genome-configurable threshold)
    if (confidence < hedge_confidence_threshold_) {
        prosody.hedge_probability = 1.0f - confidence;
    } else {
        prosody.hedge_probability = 0.0f;
    }
    
    // Pause duration based on coherence and confidence
    // Low coherence = longer pauses (searching)
    // Low confidence = longer pauses (uncertain)
    float pause_from_coherence = (1.0f - coherence) * 0.5f;
    float pause_from_confidence = (1.0f - confidence) * 0.3f;
    prosody.pause_duration = std::max(0.1f, pause_from_coherence + pause_from_confidence);
    
    return prosody;
}

std::string EmotionalModulator::add_hedging(
    const std::string& answer, 
    float hedge_prob
) {
    if (hedge_prob < 0.3f) {
        // High confidence - no hedging
        return answer;
    }
    else if (hedge_prob < 0.6f) {
        // Medium confidence - mild hedging
        return random_choice(medium_confidence_hedges_) + answer;
    }
    else {
        // Low confidence - strong hedging
        return random_choice(low_confidence_hedges_) + answer;
    }
}

std::vector<std::string> EmotionalModulator::select_connectors(
    float confidence,
    float novelty
) {
    if (novelty > 0.7f) {
        // Exploratory vocabulary
        return exploratory_connectors_;
    }
    else if (confidence > 0.8f) {
        // Confident vocabulary
        return confident_connectors_;
    }
    else {
        // Neutral vocabulary
        return neutral_connectors_;
    }
}

std::string EmotionalModulator::add_emotional_tone(
    const std::string& answer,
    float confidence,
    float novelty
) {
    std::string result = answer;
    
    // Add emphasis for high novelty
    if (novelty > 0.8f && confidence > 0.7f) {
        result += " This is quite interesting!";
    }
    
    // Add caution for low confidence
    if (confidence < 0.3f) {
        result += " I would need more information to be certain.";
    }
    
    // Add excitement for high novelty + high confidence
    if (novelty > 0.7f && confidence > 0.8f) {
        result += " This is a fascinating connection.";
    }
    
    return result;
}

std::string EmotionalModulator::get_emotional_state(
    float confidence,
    float novelty,
    float coherence
) {
    if (confidence > 0.8f && coherence > 0.7f) {
        return "confident and clear";
    }
    else if (novelty > 0.7f && confidence > 0.6f) {
        return "excited and curious";
    }
    else if (confidence < 0.4f && coherence < 0.5f) {
        return "uncertain and searching";
    }
    else if (coherence > 0.8f) {
        return "calm and thoughtful";
    }
    else if (novelty > 0.6f) {
        return "intrigued and exploratory";
    }
    else {
        return "neutral and attentive";
    }
}

template<typename T>
const T& EmotionalModulator::random_choice(const std::vector<T>& vec) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, vec.size() - 1);
    return vec[dis(gen)];
}

} // namespace cognitive
} // namespace melvin

