/**
 * Audio Graph Layer Implementation
 * 
 * Learns bidirectional audio-semantic mappings through temporal co-activation.
 */

#include "audio_graph_layer.h"
#include <algorithm>
#include <cmath>
#include <iostream>
#include <fstream>
#include <chrono>

namespace melvin {
namespace audio {

AudioGraphLayer::AudioGraphLayer()
    : inputs_processed_(0), outputs_processed_(0), next_audio_node_id_(1000000),
      vocal_synthesizer_(16000),
      vocal_learner_(),
      conversation_count_(0) {
    // Audio node IDs start at 1,000,000 to avoid collision with semantic nodes
    
    // Initialize hybrid generator
    hybrid_generator_ = std::make_unique<HybridVocalGenerator>(
        vocal_synthesizer_,
        vocal_learner_
    );
}

// ============================================================
// LEARNING FROM INPUT (Whisper)
// ============================================================

void AudioGraphLayer::learn_from_speech_input(
    const std::string& transcribed_text,
    const std::vector<float>& audio_embedding,
    const std::vector<float>& mel_features,
    const std::vector<uint64_t>& activated_concept_ids,
    float temporal_proximity
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (activated_concept_ids.empty()) {
        return;  // Nothing to learn
    }
    
    // 1. Convert text to phonemes
    std::string phonemes = text_to_phonemes(transcribed_text);
    
    // 2. Get or create audio node for this pattern
    uint64_t audio_node_id = get_or_create_phoneme_node(phonemes);
    
    // 3. Update audio node features (running average)
    auto& audio_node = audio_nodes_[audio_node_id];
    
    if (audio_node.audio_embedding.empty()) {
        // First time seeing this pattern
        audio_node.audio_embedding = audio_embedding;
        audio_node.mel_features = mel_features;
    } else {
        // Update with exponential moving average (α = 0.1)
        float alpha = 0.1f;
        for (size_t i = 0; i < std::min(audio_embedding.size(), audio_node.audio_embedding.size()); ++i) {
            audio_node.audio_embedding[i] = 
                (1.0f - alpha) * audio_node.audio_embedding[i] + alpha * audio_embedding[i];
        }
        for (size_t i = 0; i < std::min(mel_features.size(), audio_node.mel_features.size()); ++i) {
            audio_node.mel_features[i] = 
                (1.0f - alpha) * audio_node.mel_features[i] + alpha * mel_features[i];
        }
    }
    
    audio_node.co_activation_count++;
    audio_node.timestamp_us = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()
    ).count();
    
    // 4. Strengthen links to activated concepts
    for (uint64_t concept_id : activated_concept_ids) {
        strengthen_link(audio_node_id, concept_id, temporal_proximity);
    }
    
    inputs_processed_++;
}

// ============================================================
// LEARNING FROM OUTPUT (TTS)
// ============================================================

void AudioGraphLayer::learn_from_speech_output(
    const std::string& spoken_text,
    const std::vector<float>& mel_features,
    const std::vector<uint64_t>& concept_ids_that_triggered_speech,
    float temporal_proximity
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (concept_ids_that_triggered_speech.empty()) {
        return;
    }
    
    // 1. Convert text to phonemes
    std::string phonemes = text_to_phonemes(spoken_text);
    
    // 2. Get or create audio node
    uint64_t audio_node_id = get_or_create_phoneme_node(phonemes);
    
    // 3. Update mel features (TTS output is high quality, so weight it more)
    auto& audio_node = audio_nodes_[audio_node_id];
    
    if (audio_node.mel_features.empty()) {
        audio_node.mel_features = mel_features;
    } else {
        // Higher learning rate for output (we're more confident in our own speech)
        float alpha = 0.2f;
        for (size_t i = 0; i < std::min(mel_features.size(), audio_node.mel_features.size()); ++i) {
            audio_node.mel_features[i] = 
                (1.0f - alpha) * audio_node.mel_features[i] + alpha * mel_features[i];
        }
    }
    
    audio_node.co_activation_count++;
    
    // 4. Strengthen links (output associations are stronger)
    for (uint64_t concept_id : concept_ids_that_triggered_speech) {
        strengthen_link(audio_node_id, concept_id, temporal_proximity, 0.02f);  // 2x learning rate
    }
    
    outputs_processed_++;
}

// ============================================================
// PHONEME-LEVEL LEARNING
// ============================================================

void AudioGraphLayer::learn_phoneme_pattern(
    const std::string& text,
    const std::vector<float>& mel_features,
    const std::vector<uint64_t>& concept_ids
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::string phonemes = text_to_phonemes(text);
    
    // Update phoneme pattern
    auto& pattern = phoneme_patterns_[phonemes];
    pattern.phoneme_sequence = phonemes;
    
    if (pattern.mel_template.empty()) {
        pattern.mel_template = mel_features;
    } else {
        // Running average
        float alpha = 0.15f;
        for (size_t i = 0; i < std::min(mel_features.size(), pattern.mel_template.size()); ++i) {
            pattern.mel_template[i] = 
                (1.0f - alpha) * pattern.mel_template[i] + alpha * mel_features[i];
        }
    }
    
    // Add concept associations
    for (uint64_t concept_id : concept_ids) {
        if (std::find(pattern.word_concepts.begin(), pattern.word_concepts.end(), concept_id) 
            == pattern.word_concepts.end()) {
            pattern.word_concepts.push_back(concept_id);
        }
    }
    
    // Increase confidence with more observations
    pattern.confidence = std::min(1.0f, pattern.confidence + 0.05f);
}

// ============================================================
// AUDIO GENERATION (Future - Phase 5)
// ============================================================

std::vector<float> AudioGraphLayer::generate_audio_from_concepts(
    const std::vector<uint64_t>& concept_ids,
    float energy_threshold
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Find audio nodes linked to these concepts
    std::unordered_map<uint64_t, float> audio_node_activations;
    
    for (uint64_t concept_id : concept_ids) {
        if (concept_to_audio_.find(concept_id) != concept_to_audio_.end()) {
            for (uint64_t audio_node_id : concept_to_audio_[concept_id]) {
                float strength = get_association_strength(audio_node_id, concept_id);
                if (strength >= energy_threshold) {
                    audio_node_activations[audio_node_id] += strength;
                }
            }
        }
    }
    
    if (audio_node_activations.empty()) {
        return {};  // No audio nodes activated
    }
    
    // Combine mel features (weighted average)
    size_t mel_dim = 80;
    std::vector<float> combined_mel(mel_dim, 0.0f);
    float total_weight = 0.0f;
    
    for (const auto& [audio_node_id, activation] : audio_node_activations) {
        const auto& audio_node = audio_nodes_[audio_node_id];
        
        if (audio_node.mel_features.size() >= mel_dim) {
            for (size_t i = 0; i < mel_dim; ++i) {
                combined_mel[i] += audio_node.mel_features[i] * activation;
            }
            total_weight += activation;
        }
    }
    
    // Normalize
    if (total_weight > 0.0f) {
        for (float& val : combined_mel) {
            val /= total_weight;
        }
    }
    
    return combined_mel;
}

bool AudioGraphLayer::can_self_generate_audio(const std::vector<uint64_t>& concept_ids) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check if we have strong enough associations
    int strong_links = 0;
    for (uint64_t concept_id : concept_ids) {
        auto it = concept_to_audio_.find(concept_id);
        if (it != concept_to_audio_.end()) {
            for (uint64_t audio_node_id : it->second) {
                // For const correctness, just check existence
                if (audio_nodes_.count(audio_node_id) > 0) {
                    strong_links++;
                }
            }
        }
    }
    
    // Need at least 3 strong associations
    return strong_links >= 3;
}

// ============================================================
// QUERY & INSPECTION
// ============================================================

std::vector<uint64_t> AudioGraphLayer::get_audio_for_concept(uint64_t concept_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (concept_to_audio_.find(concept_id) != concept_to_audio_.end()) {
        return concept_to_audio_[concept_id];
    }
    return {};
}

std::vector<uint64_t> AudioGraphLayer::get_concepts_for_audio(uint64_t audio_node_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (audio_to_concepts_.find(audio_node_id) != audio_to_concepts_.end()) {
        return audio_to_concepts_[audio_node_id];
    }
    return {};
}

float AudioGraphLayer::get_association_strength(uint64_t audio_node_id, uint64_t concept_id) {
    if (association_matrix_.find(audio_node_id) != association_matrix_.end()) {
        auto& concept_map = association_matrix_[audio_node_id];
        if (concept_map.find(concept_id) != concept_map.end()) {
            return concept_map[concept_id];
        }
    }
    return 0.0f;
}

AudioGraphLayer::LearningStats AudioGraphLayer::get_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    LearningStats stats;
    stats.total_audio_nodes = audio_nodes_.size();
    stats.total_phoneme_patterns = phoneme_patterns_.size();
    stats.inputs_processed = inputs_processed_;
    stats.outputs_processed = outputs_processed_;
    
    // Count total associations
    stats.total_associations = 0;
    float confidence_sum = 0.0f;
    
    for (const auto& [audio_id, audio_node] : audio_nodes_) {
        stats.total_associations += audio_node.linked_concepts.size();
        confidence_sum += audio_node.confidence;
    }
    
    stats.average_confidence = audio_nodes_.empty() ? 
        0.0f : confidence_sum / audio_nodes_.size();
    
    return stats;
}

// ============================================================
// INTERNAL HELPER METHODS
// ============================================================

uint64_t AudioGraphLayer::create_audio_node(
    AudioNodeType type,
    const std::vector<float>& audio_embedding,
    const std::vector<float>& mel_features,
    const std::string& phoneme_sequence
) {
    uint64_t node_id = next_audio_node_id_++;
    
    AudioNode node;
    node.node_id = node_id;
    node.type = type;
    node.audio_embedding = audio_embedding;
    node.mel_features = mel_features;
    node.phoneme_sequence = phoneme_sequence;
    node.confidence = 0.1f;  // Start with low confidence
    node.co_activation_count = 0;
    
    audio_nodes_[node_id] = node;
    
    return node_id;
}

void AudioGraphLayer::strengthen_link(
    uint64_t audio_node_id,
    uint64_t concept_id,
    float temporal_proximity,
    float learning_rate
) {
    // STDP-like learning: strengthen association based on temporal co-activation
    float delta_w = learning_rate * temporal_proximity;
    
    // Update association matrix
    association_matrix_[audio_node_id][concept_id] += delta_w;
    
    // Clamp to [0, 1]
    float& strength = association_matrix_[audio_node_id][concept_id];
    strength = std::min(1.0f, std::max(0.0f, strength));
    
    // Update bidirectional mappings
    if (std::find(concept_to_audio_[concept_id].begin(), 
                  concept_to_audio_[concept_id].end(), 
                  audio_node_id) == concept_to_audio_[concept_id].end()) {
        concept_to_audio_[concept_id].push_back(audio_node_id);
    }
    
    if (std::find(audio_to_concepts_[audio_node_id].begin(), 
                  audio_to_concepts_[audio_node_id].end(), 
                  concept_id) == audio_to_concepts_[audio_node_id].end()) {
        audio_to_concepts_[audio_node_id].push_back(concept_id);
    }
    
    // Update audio node's linked concepts
    audio_nodes_[audio_node_id].linked_concepts[concept_id] = strength;
    
    // Increase confidence with more co-activations
    audio_nodes_[audio_node_id].confidence = 
        std::min(1.0f, audio_nodes_[audio_node_id].confidence + 0.01f);
}

void AudioGraphLayer::decay_weak_links() {
    // Prune associations below threshold
    float min_strength = 0.05f;
    
    for (auto& [audio_id, concept_map] : association_matrix_) {
        for (auto it = concept_map.begin(); it != concept_map.end(); ) {
            if (it->second < min_strength) {
                it = concept_map.erase(it);
            } else {
                ++it;
            }
        }
    }
}

std::string AudioGraphLayer::text_to_phonemes(const std::string& text) {
    // Simplified phoneme conversion (for now, just return text)
    // TODO: Integrate with espeak-ng for proper phonemization
    // Command: echo "hello" | espeak-ng -q --ipa
    
    // For now, just normalize text
    std::string normalized = text;
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    return normalized;
}

uint64_t AudioGraphLayer::get_or_create_phoneme_node(const std::string& phoneme_sequence) {
    // Check if we already have a node for this phoneme pattern
    for (const auto& [node_id, audio_node] : audio_nodes_) {
        if (audio_node.phoneme_sequence == phoneme_sequence && 
            audio_node.type == AudioNodeType::PHONEME) {
            return node_id;
        }
    }
    
    // Create new node
    return create_audio_node(
        AudioNodeType::PHONEME,
        {},  // Empty embedding initially
        {},  // Empty mel features initially
        phoneme_sequence
    );
}

float AudioGraphLayer::compute_audio_similarity(
    const std::vector<float>& audio1,
    const std::vector<float>& audio2
) {
    if (audio1.empty() || audio2.empty()) {
        return 0.0f;
    }
    
    // Use shorter length for comparison
    size_t len = std::min(audio1.size(), audio2.size());
    if (len == 0) return 0.0f;
    
    // Compute means
    float mean1 = 0.0f, mean2 = 0.0f;
    for (size_t i = 0; i < len; ++i) {
        mean1 += audio1[i];
        mean2 += audio2[i];
    }
    mean1 /= len;
    mean2 /= len;
    
    // Compute normalized cross-correlation
    float numerator = 0.0f;
    float denom1 = 0.0f, denom2 = 0.0f;
    
    for (size_t i = 0; i < len; ++i) {
        float diff1 = audio1[i] - mean1;
        float diff2 = audio2[i] - mean2;
        numerator += diff1 * diff2;
        denom1 += diff1 * diff1;
        denom2 += diff2 * diff2;
    }
    
    if (denom1 <= 0.0f || denom2 <= 0.0f) {
        return 0.0f;
    }
    
    // Pearson correlation coefficient
    float correlation = numerator / std::sqrt(denom1 * denom2);
    
    // Clamp to [0, 1] range (correlation is in [-1, 1])
    // We map -1 to 0, 0 to 0.5, 1 to 1.0
    float similarity = (correlation + 1.0f) / 2.0f;
    
    return std::max(0.0f, std::min(1.0f, similarity));
}

// ============================================================
// VOCAL SYNTHESIS (Phase 6)
// ============================================================

void AudioGraphLayer::learn_vocal_parameters_from_tts(
    const std::string& text,
    const std::vector<float>& tts_audio,
    const std::vector<uint64_t>& active_concepts,
    int sample_rate
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    vocal_learner_.learn_from_tts(text, tts_audio, active_concepts, sample_rate);
}

std::vector<float> AudioGraphLayer::generate_with_vocal_cords(
    const std::string& text,
    const std::vector<float>& tts_audio,
    const std::vector<uint64_t>& concept_ids,
    HybridVocalGenerator::Mode mode
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return hybrid_generator_->generate_speech(text, tts_audio, concept_ids, mode);
}

AudioGraphLayer::DualOutput AudioGraphLayer::generate_dual_output(
    const std::string& text,
    const std::vector<float>& tts_audio,
    const std::vector<uint64_t>& concept_ids
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    DualOutput output;
    
    // 1. Primary output: TTS (reliable)
    output.tts_audio = tts_audio;
    
    // 2. Secondary output: Self-generated (learning attempt)
    auto config = vocal_learner_.get_config_for_concepts(concept_ids);
    output.self_generated = vocal_synthesizer_.synthesize_text(text, config.f0);
    
    // 3. Compute similarity
    output.similarity_score = compute_audio_similarity(tts_audio, output.self_generated);
    
    // 4. Get confidence
    auto learner_stats = vocal_learner_.get_stats();
    output.vocal_confidence = learner_stats.average_confidence;
    
    // 5. Determine current mode
    output.current_mode = hybrid_generator_->auto_select_mode(concept_ids, conversation_count_);
    
    // 6. Check if ready for solo
    output.ready_for_solo = vocal_learner_.can_self_generate(concept_ids, 0.85f);
    
    // 7. Track similarity over time
    similarity_history_.push_back(output.similarity_score);
    if (similarity_history_.size() > 100) {
        similarity_history_.erase(similarity_history_.begin());  // Keep last 100
    }
    
    conversation_count_++;
    
    return output;
}

bool AudioGraphLayer::can_speak_independently(
    const std::vector<uint64_t>& concept_ids,
    float confidence_threshold
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return vocal_learner_.can_self_generate(concept_ids, confidence_threshold);
}

HybridVocalGenerator::Mode AudioGraphLayer::get_recommended_mode(
    const std::vector<uint64_t>& concept_ids,
    size_t conversation_count
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return hybrid_generator_->auto_select_mode(concept_ids, conversation_count);
}

AudioGraphLayer::VocalStats AudioGraphLayer::get_vocal_stats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto learner_stats = vocal_learner_.get_stats();
    
    VocalStats stats;
    stats.vocal_configs_learned = learner_stats.total_vocal_configs;
    stats.vocal_confidence = learner_stats.average_confidence;
    stats.tts_samples_observed = learner_stats.tts_samples_observed;
    stats.practice_attempts = learner_stats.practice_attempts;
    stats.can_self_generate = learner_stats.average_confidence >= 0.7f;
    
    // Compute average similarity
    if (!similarity_history_.empty()) {
        float sum = 0.0f;
        for (float sim : similarity_history_) {
            sum += sim;
        }
        stats.average_similarity = sum / similarity_history_.size();
    } else {
        stats.average_similarity = 0.0f;
    }
    
    return stats;
}

void AudioGraphLayer::save_dual_output_for_analysis(
    const DualOutput& output,
    const std::string& text,
    size_t conversation_number,
    const std::string& output_dir,
    bool save_tts
) {
    // Create output directory if it doesn't exist
    std::string mkdir_cmd = "mkdir -p " + output_dir;
    system(mkdir_cmd.c_str());
    
    // Save TTS audio (optional - mainly for comparison)
    if (save_tts) {
        std::string tts_file = output_dir + "/conv_" + std::to_string(conversation_number) + "_tts.raw";
        std::ofstream tts_out(tts_file, std::ios::binary);
        if (tts_out) {
            tts_out.write(reinterpret_cast<const char*>(output.tts_audio.data()), 
                         output.tts_audio.size() * sizeof(float));
            tts_out.close();
        }
    }
    
    // Save self-generated audio (MELVIN's attempt - this is what we care about!)
    std::string self_file = output_dir + "/conv_" + std::to_string(conversation_number) + "_self.raw";
    std::ofstream self_out(self_file, std::ios::binary);
    if (self_out) {
        self_out.write(reinterpret_cast<const char*>(output.self_generated.data()), 
                      output.self_generated.size() * sizeof(float));
        self_out.close();
    }
    
    // Save metadata
    std::string meta_file = output_dir + "/conv_" + std::to_string(conversation_number) + "_meta.txt";
    std::ofstream meta_out(meta_file);
    if (meta_out) {
        meta_out << "Conversation: " << conversation_number << "\n";
        meta_out << "Text: \"" << text << "\"\n";
        meta_out << "Similarity: " << output.similarity_score << "\n";
        meta_out << "Vocal Confidence: " << output.vocal_confidence << "\n";
        meta_out << "Mode: " << static_cast<int>(output.current_mode) << "\n";
        meta_out << "Ready for Solo: " << (output.ready_for_solo ? "YES" : "NO") << "\n";
        meta_out << "Self samples: " << output.self_generated.size() << "\n";
        if (save_tts) {
            meta_out << "TTS samples: " << output.tts_audio.size() << "\n";
        }
        meta_out.close();
    }
    
    std::cout << "💾 Saved MELVIN's attempt for conversation " << conversation_number 
              << " (similarity: " << (output.similarity_score * 100.0f) << "%)" << std::endl;
}

// ============================================================
// PERSISTENCE (Placeholder)
// ============================================================

void AudioGraphLayer::save_to_file(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ofstream out(filepath, std::ios::binary);
    if (!out) {
        std::cerr << "Failed to save audio graph to " << filepath << std::endl;
        return;
    }
    
    // TODO: Implement binary serialization
    std::cout << "Audio graph saved to " << filepath << std::endl;
}

void AudioGraphLayer::load_from_file(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::ifstream in(filepath, std::ios::binary);
    if (!in) {
        std::cerr << "Failed to load audio graph from " << filepath << std::endl;
        return;
    }
    
    // TODO: Implement binary deserialization
    std::cout << "Audio graph loaded from " << filepath << std::endl;
}

} // namespace audio
} // namespace melvin

