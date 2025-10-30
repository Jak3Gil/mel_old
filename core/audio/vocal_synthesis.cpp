/**
 * Vocal Synthesis Implementation
 * 
 * Formant-based vocal synthesis that simulates human speech production.
 */

#include "vocal_synthesis.h"
#include <cmath>
#include <algorithm>
#include <random>
#include <iostream>

namespace melvin {
namespace audio {

static constexpr float PI = 3.14159265358979323846f;

// ============================================================
// VOCAL CONFIGURATION
// ============================================================

VocalConfiguration::VocalConfiguration()
    : f0(150.0f),
      voicing_strength(1.0f),
      aspiration_level(0.0f),
      open_quotient(0.6f),
      nasal_coupling(0.0f),
      lip_radiation(1.0f),
      duration_ms(100.0f),
      amplitude(1.0f) {
    // Default: neutral vowel
    formants = {
        Formant(500, 60, 1.0),
        Formant(1500, 90, 1.0),
        Formant(2500, 120, 0.8),
        Formant(3500, 150, 0.5),
        Formant(4500, 200, 0.3)
    };
}

VocalConfiguration VocalConfiguration::for_phoneme(const std::string& phoneme) {
    VocalConfiguration config;
    config.duration_ms = 80.0f;  // Default duration
    config.voicing_strength = 1.0f;
    config.aspiration_level = 0.0f;
    
    if (phoneme == "AA" || phoneme == "a") {  // /ɑ/ as in "father"
        config.formants = {
            Formant(730, 80, 1.0),
            Formant(1090, 100, 1.0),
            Formant(2440, 120, 0.8),
            Formant(3400, 200, 0.5),
            Formant(4950, 250, 0.3)
        };
        config.open_quotient = 0.6f;
        
    } else if (phoneme == "IY" || phoneme == "i") {  // /i/ as in "see"
        config.formants = {
            Formant(270, 50, 1.0),
            Formant(2290, 100, 1.0),
            Formant(3010, 150, 0.7),
            Formant(3500, 200, 0.4),
            Formant(4950, 250, 0.2)
        };
        config.open_quotient = 0.5f;
        
    } else if (phoneme == "UW" || phoneme == "u") {  // /u/ as in "boot"
        config.formants = {
            Formant(300, 60, 1.0),
            Formant(870, 90, 1.0),
            Formant(2240, 120, 0.7),
            Formant(3000, 150, 0.4),
            Formant(4300, 200, 0.2)
        };
        config.open_quotient = 0.55f;
        
    } else if (phoneme == "EH" || phoneme == "e") {  // /ɛ/ as in "bed"
        config.formants = {
            Formant(530, 70, 1.0),
            Formant(1840, 100, 1.0),
            Formant(2480, 120, 0.8),
            Formant(3500, 200, 0.4),
            Formant(4500, 250, 0.2)
        };
        config.open_quotient = 0.58f;
        
    } else if (phoneme == "SH" || phoneme == "sh") {  // /ʃ/ as in "show"
        config.formants = {
            Formant(2500, 500, 0.6),
            Formant(3500, 600, 0.5),
            Formant(4500, 700, 0.4),
            Formant(5500, 800, 0.3),
            Formant(6500, 900, 0.2)
        };
        config.voicing_strength = 0.0f;  // Unvoiced
        config.aspiration_level = 1.0f;
        config.open_quotient = 0.7f;
        
    } else if (phoneme == "L" || phoneme == "l") {  // /l/ as in "love"
        config.formants = {
            Formant(400, 80, 1.0),
            Formant(1200, 100, 0.9),
            Formant(2500, 150, 0.6),
            Formant(3500, 200, 0.3),
            Formant(4500, 250, 0.2)
        };
        config.voicing_strength = 0.9f;
        config.open_quotient = 0.55f;
        
    } else if (phoneme == "H" || phoneme == "h") {  // /h/ as in "hello"
        config.formants = {
            Formant(500, 200, 0.5),
            Formant(1500, 300, 0.4),
            Formant(2500, 400, 0.3),
            Formant(3500, 500, 0.2),
            Formant(4500, 600, 0.1)
        };
        config.voicing_strength = 0.2f;
        config.aspiration_level = 0.8f;
        config.duration_ms = 60.0f;
    }
    
    return config;
}

VocalConfiguration VocalConfiguration::interpolate(
    const VocalConfiguration& a,
    const VocalConfiguration& b,
    float t
) {
    VocalConfiguration result;
    
    result.f0 = a.f0 * (1.0f - t) + b.f0 * t;
    result.voicing_strength = a.voicing_strength * (1.0f - t) + b.voicing_strength * t;
    result.aspiration_level = a.aspiration_level * (1.0f - t) + b.aspiration_level * t;
    result.open_quotient = a.open_quotient * (1.0f - t) + b.open_quotient * t;
    result.nasal_coupling = a.nasal_coupling * (1.0f - t) + b.nasal_coupling * t;
    result.lip_radiation = a.lip_radiation * (1.0f - t) + b.lip_radiation * t;
    result.amplitude = a.amplitude * (1.0f - t) + b.amplitude * t;
    
    // Interpolate formants
    size_t num_formants = std::min(a.formants.size(), b.formants.size());
    for (size_t i = 0; i < num_formants; ++i) {
        Formant f;
        f.frequency = a.formants[i].frequency * (1.0f - t) + b.formants[i].frequency * t;
        f.bandwidth = a.formants[i].bandwidth * (1.0f - t) + b.formants[i].bandwidth * t;
        f.amplitude = a.formants[i].amplitude * (1.0f - t) + b.formants[i].amplitude * t;
        result.formants.push_back(f);
    }
    
    return result;
}

// ============================================================
// VOCAL SYNTHESIZER
// ============================================================

VocalSynthesizer::VocalSynthesizer(int sample_rate)
    : sample_rate_(sample_rate) {}

std::vector<float> VocalSynthesizer::synthesize(const VocalConfiguration& config) {
    int num_samples = static_cast<int>(config.duration_ms * sample_rate_ / 1000.0f);
    
    // 1. Generate glottal source
    auto glottal = generate_glottal_pulse(
        config.f0,
        config.open_quotient,
        config.voicing_strength,
        config.aspiration_level,
        num_samples
    );
    
    // 2. Apply formant filtering
    auto filtered = apply_formants(glottal, config.formants);
    
    // 3. Apply lip radiation
    auto radiated = apply_lip_radiation(filtered);
    
    // 4. Apply amplitude
    for (float& sample : radiated) {
        sample *= config.amplitude;
    }
    
    return radiated;
}

std::vector<float> VocalSynthesizer::synthesize_phonemes(
    const std::vector<std::string>& phonemes,
    const std::vector<float>& pitch_contour,
    const std::vector<float>& durations_ms
) {
    std::vector<float> result;
    
    for (size_t i = 0; i < phonemes.size(); ++i) {
        VocalConfiguration config = VocalConfiguration::for_phoneme(phonemes[i]);
        
        if (i < pitch_contour.size()) {
            config.f0 = pitch_contour[i];
        }
        if (i < durations_ms.size()) {
            config.duration_ms = durations_ms[i];
        }
        
        auto audio = synthesize(config);
        result.insert(result.end(), audio.begin(), audio.end());
    }
    
    return result;
}

std::vector<float> VocalSynthesizer::synthesize_with_transitions(
    const std::vector<VocalConfiguration>& configs,
    float transition_time_ms
) {
    std::vector<float> result;
    
    for (size_t i = 0; i < configs.size(); ++i) {
        const auto& config = configs[i];
        
        // Synthesize main part
        auto audio = synthesize(config);
        result.insert(result.end(), audio.begin(), audio.end());
        
        // Add transition to next if not last
        if (i + 1 < configs.size()) {
            int transition_samples = static_cast<int>(transition_time_ms * sample_rate_ / 1000.0f);
            
            for (int j = 0; j < transition_samples; ++j) {
                float t = static_cast<float>(j) / transition_samples;
                VocalConfiguration interp = VocalConfiguration::interpolate(
                    config, configs[i + 1], t
                );
                interp.duration_ms = 1.0f;  // 1ms per sample
                
                auto sample_audio = synthesize(interp);
                if (!sample_audio.empty()) {
                    result.push_back(sample_audio[0]);
                }
            }
        }
    }
    
    return result;
}

std::vector<float> VocalSynthesizer::synthesize_text(
    const std::string& text,
    float base_pitch
) {
    auto phonemes = text_to_phonemes(text);
    
    std::vector<float> pitch_contour(phonemes.size(), base_pitch);
    std::vector<float> durations(phonemes.size(), 80.0f);
    
    return synthesize_phonemes(phonemes, pitch_contour, durations);
}

// ============================================================
// PRIVATE METHODS
// ============================================================

std::vector<float> VocalSynthesizer::generate_glottal_pulse(
    float f0,
    float open_quotient,
    float voicing_strength,
    float aspiration_level,
    int num_samples
) {
    std::vector<float> signal(num_samples, 0.0f);
    
    if (f0 <= 0.0f) {
        // No pitch - just noise
        return generate_noise(num_samples);
    }
    
    float period_samples = sample_rate_ / f0;
    
    for (int i = 0; i < num_samples; ++i) {
        float phase = std::fmod(i, period_samples) / period_samples;
        
        // Simplified glottal pulse (Rosenberg model)
        float glottal_value = 0.0f;
        if (phase < open_quotient) {
            // Opening phase
            glottal_value = 0.5f * (1.0f - std::cos(PI * phase / open_quotient));
        } else {
            // Closing phase (sharp closure)
            float t = (phase - open_quotient) / (1.0f - open_quotient);
            glottal_value = 0.5f * (1.0f + std::cos(PI * t));
        }
        
        // Mix voiced and noise components
        float noise = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        signal[i] = voicing_strength * glottal_value + aspiration_level * noise * 0.3f;
    }
    
    return signal;
}

std::vector<float> VocalSynthesizer::apply_formants(
    const std::vector<float>& source,
    const std::vector<Formant>& formants
) {
    std::vector<float> result = source;
    
    // Apply each formant as a resonant filter (2nd order IIR)
    for (const auto& formant : formants) {
        float r = std::exp(-PI * formant.bandwidth / sample_rate_);
        float omega = 2.0f * PI * formant.frequency / sample_rate_;
        
        // Filter coefficients
        float a1 = -2.0f * r * std::cos(omega);
        float a2 = r * r;
        float b0 = formant.amplitude * (1.0f - r * r);
        
        // Apply filter
        std::vector<float> filtered(result.size(), 0.0f);
        for (size_t i = 2; i < result.size(); ++i) {
            filtered[i] = b0 * result[i] - a1 * filtered[i-1] - a2 * filtered[i-2];
        }
        
        result = filtered;
    }
    
    return result;
}

std::vector<float> VocalSynthesizer::apply_lip_radiation(
    const std::vector<float>& signal
) {
    // Simple first-order high-pass filter (6dB/octave)
    std::vector<float> result(signal.size(), 0.0f);
    
    float alpha = 0.95f;  // High-pass coefficient
    
    for (size_t i = 1; i < signal.size(); ++i) {
        result[i] = alpha * (result[i-1] + signal[i] - signal[i-1]);
    }
    
    return result;
}

std::vector<float> VocalSynthesizer::generate_noise(int num_samples) {
    std::vector<float> noise(num_samples);
    for (int i = 0; i < num_samples; ++i) {
        noise[i] = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
    }
    return noise;
}

std::vector<std::string> VocalSynthesizer::text_to_phonemes(const std::string& text) {
    // Simplified: just split into letters and map to phonemes
    std::vector<std::string> phonemes;
    
    std::string lower_text = text;
    std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
    
    for (char c : lower_text) {
        if (c == 'a') phonemes.push_back("AA");
        else if (c == 'e') phonemes.push_back("EH");
        else if (c == 'i') phonemes.push_back("IY");
        else if (c == 'o') phonemes.push_back("AA");
        else if (c == 'u') phonemes.push_back("UW");
        else if (c == 'h') phonemes.push_back("H");
        else if (c == 'l') phonemes.push_back("L");
        else if (c == 's') phonemes.push_back("SH");
        else if (c != ' ') phonemes.push_back("AA");  // Default vowel
    }
    
    return phonemes;
}

// ============================================================
// VOCAL PARAMETER LEARNER
// ============================================================

VocalParameterLearner::VocalParameterLearner()
    : tts_samples_observed_(0), practice_attempts_(0) {}

void VocalParameterLearner::learn_from_tts(
    const std::string& text,
    const std::vector<float>& tts_audio,
    const std::vector<uint64_t>& active_concepts,
    int sample_rate
) {
    // Extract vocal parameters from TTS audio
    VocalConfiguration config = extract_parameters_from_audio(tts_audio, sample_rate);
    
    // Associate with active concepts
    for (uint64_t concept_id : active_concepts) {
        strengthen_association(concept_id, config, 0.02f);
    }
    
    tts_samples_observed_++;
    
    std::cout << "🎙️  Learned vocal params from TTS: \"" << text << "\" → " 
              << active_concepts.size() << " concepts (F0=" << config.f0 << " Hz)" << std::endl;
}

void VocalParameterLearner::learn_from_practice(
    const VocalConfiguration& attempted,
    const VocalConfiguration& target,
    float similarity_score,
    const std::vector<uint64_t>& active_concepts
) {
    // Reinforcement learning: move towards target if similarity is good
    if (similarity_score > 0.5f) {
        for (uint64_t concept_id : active_concepts) {
            // Interpolate towards target
            VocalConfiguration improved = VocalConfiguration::interpolate(
                attempted, target, 0.2f * similarity_score
            );
            strengthen_association(concept_id, improved, 0.01f * similarity_score);
        }
    }
    
    practice_attempts_++;
}

VocalConfiguration VocalParameterLearner::get_config_for_concepts(
    const std::vector<uint64_t>& concept_ids
) {
    if (concept_ids.empty()) {
        return VocalConfiguration();  // Default
    }
    
    // Find most confident concept
    uint64_t best_concept = concept_ids[0];
    float best_confidence = 0.0f;
    
    for (uint64_t concept_id : concept_ids) {
        if (vocal_confidence_.find(concept_id) != vocal_confidence_.end()) {
            if (vocal_confidence_[concept_id] > best_confidence) {
                best_confidence = vocal_confidence_[concept_id];
                best_concept = concept_id;
            }
        }
    }
    
    if (concept_to_vocal_.find(best_concept) != concept_to_vocal_.end()) {
        return concept_to_vocal_[best_concept];
    }
    
    return VocalConfiguration();  // Default if not found
}

bool VocalParameterLearner::can_self_generate(
    const std::vector<uint64_t>& concept_ids,
    float confidence_threshold
) {
    int confident_concepts = 0;
    
    for (uint64_t concept_id : concept_ids) {
        if (vocal_confidence_.find(concept_id) != vocal_confidence_.end()) {
            if (vocal_confidence_[concept_id] >= confidence_threshold) {
                confident_concepts++;
            }
        }
    }
    
    return confident_concepts >= 2;  // Need at least 2 confident concepts
}

VocalParameterLearner::LearningStats VocalParameterLearner::get_stats() const {
    LearningStats stats;
    stats.total_vocal_configs = concept_to_vocal_.size();
    stats.total_concept_mappings = vocal_confidence_.size();
    stats.tts_samples_observed = tts_samples_observed_;
    stats.practice_attempts = practice_attempts_;
    
    float confidence_sum = 0.0f;
    for (const auto& [concept_id, confidence] : vocal_confidence_) {
        confidence_sum += confidence;
    }
    stats.average_confidence = vocal_confidence_.empty() ? 
        0.0f : confidence_sum / vocal_confidence_.size();
    
    return stats;
}

VocalConfiguration VocalParameterLearner::extract_parameters_from_audio(
    const std::vector<float>& audio,
    int sample_rate
) {
    VocalConfiguration config;
    
    // Extract pitch
    config.f0 = estimate_pitch(audio, sample_rate);
    
    // Extract formants
    config.formants = estimate_formants(audio, sample_rate);
    
    // Estimate voicing
    float energy = 0.0f;
    for (float sample : audio) {
        energy += sample * sample;
    }
    config.voicing_strength = std::min(1.0f, energy / audio.size() * 100.0f);
    
    return config;
}

void VocalParameterLearner::strengthen_association(
    uint64_t concept_id,
    const VocalConfiguration& config,
    float learning_rate
) {
    // Update or create vocal config for this concept
    if (concept_to_vocal_.find(concept_id) == concept_to_vocal_.end()) {
        concept_to_vocal_[concept_id] = config;
        vocal_confidence_[concept_id] = 0.1f;
    } else {
        // Blend with existing config
        auto& existing = concept_to_vocal_[concept_id];
        existing = VocalConfiguration::interpolate(existing, config, learning_rate);
    }
    
    // Increase confidence
    vocal_confidence_[concept_id] = std::min(1.0f, vocal_confidence_[concept_id] + learning_rate);
}

float VocalParameterLearner::estimate_pitch(
    const std::vector<float>& audio,
    int sample_rate
) {
    // Simplified autocorrelation pitch detection
    float min_pitch = 80.0f;  // Hz
    float max_pitch = 400.0f;  // Hz
    
    int min_lag = static_cast<int>(sample_rate / max_pitch);
    int max_lag = static_cast<int>(sample_rate / min_pitch);
    
    float best_correlation = 0.0f;
    int best_lag = min_lag;
    
    for (int lag = min_lag; lag < max_lag && lag < static_cast<int>(audio.size() / 2); ++lag) {
        float correlation = 0.0f;
        for (size_t i = 0; i < audio.size() - lag; ++i) {
            correlation += audio[i] * audio[i + lag];
        }
        
        if (correlation > best_correlation) {
            best_correlation = correlation;
            best_lag = lag;
        }
    }
    
    return static_cast<float>(sample_rate) / best_lag;
}

std::vector<Formant> VocalParameterLearner::estimate_formants(
    const std::vector<float>& audio,
    int sample_rate
) {
    // Simplified: return typical formants
    // In production, would use LPC (Linear Predictive Coding)
    std::vector<Formant> formants = {
        Formant(500, 60, 1.0),
        Formant(1500, 90, 1.0),
        Formant(2500, 120, 0.8),
        Formant(3500, 150, 0.5),
        Formant(4500, 200, 0.3)
    };
    
    return formants;
}

// ============================================================
// HYBRID VOCAL GENERATOR
// ============================================================

HybridVocalGenerator::HybridVocalGenerator(
    VocalSynthesizer& synthesizer,
    VocalParameterLearner& learner
) : synthesizer_(synthesizer), learner_(learner) {}

std::vector<float> HybridVocalGenerator::generate_speech(
    const std::string& text,
    const std::vector<float>& tts_audio,
    const std::vector<uint64_t>& active_concepts,
    Mode mode
) {
    float tts_weight = get_tts_weight(mode);
    
    if (tts_weight >= 1.0f) {
        // Pure TTS
        return tts_audio;
    }
    
    if (tts_weight <= 0.0f) {
        // Pure self-generation
        auto config = learner_.get_config_for_concepts(active_concepts);
        return synthesizer_.synthesize_text(text, config.f0);
    }
    
    // Blend TTS and self-generated
    auto config = learner_.get_config_for_concepts(active_concepts);
    auto self_audio = synthesizer_.synthesize_text(text, config.f0);
    
    return blend_audio(tts_audio, self_audio, tts_weight);
}

HybridVocalGenerator::Mode HybridVocalGenerator::auto_select_mode(
    const std::vector<uint64_t>& concept_ids,
    size_t conversation_count
) {
    if (conversation_count < 50) return PURE_TTS;
    if (conversation_count < 150) return MOSTLY_TTS;
    if (conversation_count < 300) return BALANCED;
    if (conversation_count < 500) return MOSTLY_SELF;
    return PURE_SELF;
}

float HybridVocalGenerator::get_proficiency(const std::vector<uint64_t>& concept_ids) {
    auto stats = learner_.get_stats();
    return stats.average_confidence;
}

std::vector<float> HybridVocalGenerator::blend_audio(
    const std::vector<float>& tts_audio,
    const std::vector<float>& self_audio,
    float tts_weight
) {
    size_t min_size = std::min(tts_audio.size(), self_audio.size());
    std::vector<float> blended(min_size);
    
    for (size_t i = 0; i < min_size; ++i) {
        blended[i] = tts_weight * tts_audio[i] + (1.0f - tts_weight) * self_audio[i];
    }
    
    return blended;
}

float HybridVocalGenerator::get_tts_weight(Mode mode) {
    switch (mode) {
        case PURE_TTS:    return 1.0f;
        case MOSTLY_TTS:  return 0.8f;
        case BALANCED:    return 0.5f;
        case MOSTLY_SELF: return 0.2f;
        case PURE_SELF:   return 0.0f;
    }
    return 1.0f;
}

} // namespace audio
} // namespace melvin

