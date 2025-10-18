#include "audio_tokenizer.h"
#include <iostream>
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace melvin {
namespace audio {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

AudioTokenizer::AudioTokenizer()
    : AudioTokenizer(Config()) {
}

AudioTokenizer::AudioTokenizer(const Config& config)
    : config_(config) {
    
    std::cout << "🔤 AudioTokenizer initialized" << std::endl;
    std::cout << "   Frame size: " << config_.frame_size_ms << " ms" << std::endl;
    std::cout << "   Hop size: " << config_.hop_size_ms << " ms" << std::endl;
    std::cout << "   MFCC coefficients: " << config_.num_mfcc << std::endl;
    std::cout << "   Pure audio tokens (no text, no APIs)" << std::endl;
}

AudioTokenizer::~AudioTokenizer() {
}

// ============================================================================
// TOKENIZATION
// ============================================================================

std::vector<uint64_t> AudioTokenizer::tokenize(const std::vector<float>& samples,
                                               AtomicGraph& graph) {
    std::vector<uint64_t> token_ids;
    
    if (samples.empty()) {
        return token_ids;
    }
    
    // Calculate frame parameters
    size_t frame_samples = (config_.frame_size_ms * config_.sample_rate) / 1000;
    size_t hop_samples = (config_.hop_size_ms * config_.sample_rate) / 1000;
    
    std::cout << "\n🔤 Tokenizing audio (" << samples.size() << " samples)..." << std::endl;
    
    // Extract overlapping frames
    for (size_t i = 0; i + frame_samples < samples.size(); i += hop_samples) {
        // Extract token from this frame
        AudioToken token = extract_token(samples, i, i + frame_samples);
        token.timestamp = static_cast<float>(i) / config_.sample_rate;
        
        // Check for matching token (deduplication)
        uint64_t node_id = 0;
        
        if (config_.enable_deduplication) {
            node_id = find_match(token, graph);
        }
        
        // Create new node if no match
        if (node_id == 0) {
            node_id = create_node(token, graph);
            unique_tokens_++;
        } else {
            // Reinforce existing node
            // (could increment edge weights here)
        }
        
        token_ids.push_back(node_id);
        token_count_++;
    }
    
    // Link temporal sequence
    if (token_ids.size() > 1) {
        link_sequence(token_ids, graph);
    }
    
    // Learn patterns
    learn_patterns(token_ids, graph);
    
    std::cout << "   ✅ Created " << token_ids.size() << " audio tokens" << std::endl;
    std::cout << "   📊 Unique: " << unique_tokens_ << " | Total: " << token_count_ << std::endl;
    
    return token_ids;
}

AudioToken AudioTokenizer::extract_token(const std::vector<float>& samples,
                                        size_t start_idx,
                                        size_t end_idx) {
    AudioToken token;
    
    // Extract chunk
    std::vector<float> chunk(samples.begin() + start_idx, 
                            samples.begin() + end_idx);
    
    // Extract features
    token.features = extract_features(chunk);
    token.duration_ms = ((end_idx - start_idx) / static_cast<float>(config_.sample_rate)) * 1000.0f;
    
    // Extract basic acoustic properties
    token.energy = compute_energy(chunk);
    token.pitch = estimate_pitch(chunk);
    
    // Spectral centroid (brightness)
    auto spectral = extract_spectral_features(chunk);
    if (!spectral.empty()) {
        token.spectral_centroid = spectral[0];
    }
    
    return token;
}

uint64_t AudioTokenizer::create_node(const AudioToken& token, AtomicGraph& graph) {
    // Generate unique ID based on audio features
    std::string token_id = generate_token_id(token);
    
    // Create node
    uint64_t node_id = graph.get_or_create_node("audio_token:" + token_id, 2);  // Feature type
    
    // Store token for matching
    recent_tokens_.push_back(token);
    
    // Keep recent tokens limited
    if (recent_tokens_.size() > 1000) {
        recent_tokens_.erase(recent_tokens_.begin());
    }
    
    return node_id;
}

// ============================================================================
// FEATURE EXTRACTION
// ============================================================================

std::vector<float> AudioTokenizer::extract_features(const std::vector<float>& samples) {
    std::vector<float> features;
    
    // Extract MFCC (primary features)
    auto mfcc = extract_mfcc(samples);
    features.insert(features.end(), mfcc.begin(), mfcc.end());
    
    // Extract spectral features
    auto spectral = extract_spectral_features(samples);
    features.insert(features.end(), spectral.begin(), spectral.end());
    
    // Add energy and pitch
    features.push_back(compute_energy(samples));
    features.push_back(estimate_pitch(samples));
    
    return features;
}

std::vector<float> AudioTokenizer::extract_mfcc(const std::vector<float>& samples) {
    // Simplified MFCC extraction
    // In production, use librosa or custom FFT→mel→DCT pipeline
    
    std::vector<float> mfcc(config_.num_mfcc, 0.0f);
    
    // Compute FFT
    auto fft = compute_fft(samples);
    
    // Apply mel filterbank
    auto mel = mel_filterbank(fft);
    
    // Apply DCT to get cepstral coefficients
    auto coeffs = dct(mel);
    
    // Take first N coefficients
    for (size_t i = 0; i < std::min(coeffs.size(), config_.num_mfcc); i++) {
        mfcc[i] = coeffs[i];
    }
    
    return mfcc;
}

std::vector<float> AudioTokenizer::extract_spectral_features(const std::vector<float>& samples) {
    std::vector<float> features;
    
    // Compute FFT
    auto fft = compute_fft(samples);
    
    // Spectral centroid (center of mass of spectrum)
    float centroid = 0.0f;
    float sum_mag = 0.0f;
    float sum_weighted = 0.0f;
    
    for (size_t i = 0; i < fft.size(); i++) {
        float freq = static_cast<float>(i) * config_.sample_rate / (2.0f * fft.size());
        sum_weighted += freq * fft[i];
        sum_mag += fft[i];
    }
    
    if (sum_mag > 0) {
        centroid = sum_weighted / sum_mag;
    }
    
    features.push_back(centroid);
    
    // Spectral rolloff (frequency below which 85% of energy)
    float rolloff = 0.0f;
    float cumsum = 0.0f;
    float threshold = sum_mag * 0.85f;
    
    for (size_t i = 0; i < fft.size(); i++) {
        cumsum += fft[i];
        if (cumsum >= threshold) {
            rolloff = static_cast<float>(i) * config_.sample_rate / (2.0f * fft.size());
            break;
        }
    }
    
    features.push_back(rolloff);
    
    return features;
}

float AudioTokenizer::compute_energy(const std::vector<float>& samples) {
    if (samples.empty()) return 0.0f;
    
    float sum = 0.0f;
    for (float s : samples) {
        sum += s * s;
    }
    
    return std::sqrt(sum / samples.size());
}

float AudioTokenizer::estimate_pitch(const std::vector<float>& samples) {
    // Simplified pitch detection (autocorrelation method)
    // In production, use YIN, PYIN, or similar algorithm
    
    if (samples.size() < 100) return 0.0f;
    
    // Find first peak in autocorrelation
    float max_corr = 0.0f;
    size_t best_lag = 0;
    
    size_t min_lag = config_.sample_rate / 500;  // Max 500 Hz
    size_t max_lag = config_.sample_rate / 80;   // Min 80 Hz
    
    for (size_t lag = min_lag; lag < std::min(max_lag, samples.size() / 2); lag++) {
        float corr = 0.0f;
        for (size_t i = 0; i + lag < samples.size(); i++) {
            corr += samples[i] * samples[i + lag];
        }
        
        if (corr > max_corr) {
            max_corr = corr;
            best_lag = lag;
        }
    }
    
    if (best_lag > 0) {
        return static_cast<float>(config_.sample_rate) / best_lag;
    }
    
    return 0.0f;
}

// ============================================================================
// SIMILARITY & MATCHING
// ============================================================================

float AudioTokenizer::compute_similarity(const AudioToken& token1, const AudioToken& token2) {
    // Cosine similarity on feature vectors
    if (token1.features.empty() || token2.features.empty()) {
        return 0.0f;
    }
    
    size_t dim = std::min(token1.features.size(), token2.features.size());
    
    float dot = 0.0f;
    float norm1 = 0.0f;
    float norm2 = 0.0f;
    
    for (size_t i = 0; i < dim; i++) {
        dot += token1.features[i] * token2.features[i];
        norm1 += token1.features[i] * token1.features[i];
        norm2 += token2.features[i] * token2.features[i];
    }
    
    if (norm1 > 0 && norm2 > 0) {
        return dot / (std::sqrt(norm1) * std::sqrt(norm2));
    }
    
    return 0.0f;
}

uint64_t AudioTokenizer::find_match(const AudioToken& token, AtomicGraph& graph) {
    float best_similarity = 0.0f;
    uint64_t best_id = 0;
    
    // Check against recent tokens
    for (const auto& cached : recent_tokens_) {
        float sim = compute_similarity(token, cached);
        
        if (sim >= config_.similarity_threshold && sim > best_similarity) {
            best_similarity = sim;
            best_id = cached.id;
        }
    }
    
    return best_id;
}

// ============================================================================
// PATTERN LEARNING
// ============================================================================

void AudioTokenizer::link_sequence(const std::vector<uint64_t>& token_ids,
                                  AtomicGraph& graph) {
    // Create temporal links between consecutive tokens
    for (size_t i = 1; i < token_ids.size(); i++) {
        graph.add_edge(token_ids[i-1], token_ids[i], Relation::TEMPORAL_NEXT, 1.0f);
    }
}

void AudioTokenizer::learn_patterns(const std::vector<uint64_t>& token_ids,
                                   AtomicGraph& graph) {
    // Find repeated subsequences
    if (token_ids.size() < 3) return;
    
    // Look for 2-token and 3-token patterns
    for (size_t pattern_len = 2; pattern_len <= 3 && pattern_len < token_ids.size(); pattern_len++) {
        for (size_t i = 0; i + pattern_len <= token_ids.size(); i++) {
            // Check if this pattern appears elsewhere
            for (size_t j = i + pattern_len; j + pattern_len <= token_ids.size(); j++) {
                // Compare patterns
                bool match = true;
                for (size_t k = 0; k < pattern_len; k++) {
                    if (token_ids[i + k] != token_ids[j + k]) {
                        match = false;
                        break;
                    }
                }
                
                // If pattern repeats, strengthen connections
                if (match) {
                    for (size_t k = 1; k < pattern_len; k++) {
                        graph.add_edge(token_ids[i + k - 1], token_ids[i + k], 
                                     Relation::CO_OCCURS_WITH, 0.5f);
                    }
                }
            }
        }
    }
}

// ============================================================================
// STATISTICS
// ============================================================================

void AudioTokenizer::print_stats() const {
    std::cout << "\n📊 AudioTokenizer Statistics:" << std::endl;
    std::cout << "   Total tokens: " << token_count_ << std::endl;
    std::cout << "   Unique tokens: " << unique_tokens_ << std::endl;
    std::cout << "   Deduplication: " << (config_.enable_deduplication ? "enabled" : "disabled") << std::endl;
    std::cout << "   Cached tokens: " << recent_tokens_.size() << std::endl;
}

// ============================================================================
// HELPER METHODS
// ============================================================================

std::string AudioTokenizer::generate_token_id(const AudioToken& token) {
    // Generate unique ID from features
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2);
    oss << "e" << token.energy << "_p" << token.pitch << "_c" << token.spectral_centroid;
    return oss.str();
}

std::vector<float> AudioTokenizer::compute_fft(const std::vector<float>& samples) {
    // Simplified FFT (magnitude spectrum)
    // In production, use FFTW or similar library
    
    size_t n = samples.size();
    std::vector<float> magnitude(n / 2);
    
    // Very simplified DFT for demo purposes
    for (size_t k = 0; k < n / 2; k++) {
        float real = 0.0f;
        float imag = 0.0f;
        
        for (size_t i = 0; i < n; i++) {
            float angle = 2.0f * M_PI * k * i / n;
            real += samples[i] * std::cos(angle);
            imag += samples[i] * std::sin(angle);
        }
        
        magnitude[k] = std::sqrt(real * real + imag * imag);
    }
    
    return magnitude;
}

std::vector<float> AudioTokenizer::mel_filterbank(const std::vector<float>& fft) {
    // Simplified mel filterbank
    // In production, use proper mel-scale triangular filters
    
    const size_t num_filters = 26;
    std::vector<float> mel(num_filters, 0.0f);
    
    // Group FFT bins into mel bands (logarithmic spacing)
    size_t bins_per_filter = fft.size() / num_filters;
    
    for (size_t i = 0; i < num_filters && i * bins_per_filter < fft.size(); i++) {
        for (size_t j = 0; j < bins_per_filter && i * bins_per_filter + j < fft.size(); j++) {
            mel[i] += fft[i * bins_per_filter + j];
        }
        mel[i] /= bins_per_filter;
    }
    
    return mel;
}

std::vector<float> AudioTokenizer::dct(const std::vector<float>& mel) {
    // Discrete Cosine Transform
    // Converts mel spectrum to cepstral coefficients
    
    size_t n = mel.size();
    std::vector<float> coeffs(n);
    
    for (size_t k = 0; k < n; k++) {
        float sum = 0.0f;
        for (size_t i = 0; i < n; i++) {
            sum += mel[i] * std::cos(M_PI * k * (i + 0.5f) / n);
        }
        coeffs[k] = sum;
    }
    
    return coeffs;
}

} // namespace audio
} // namespace melvin

