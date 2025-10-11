/*
 * MELVIN SENSORY UTILITIES
 * 
 * Common utilities for multi-modal processing:
 * - Embedding quantization
 * - Vector encoding/decoding
 * - Distance metrics
 * - Mock data generators
 */

#pragma once
#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <random>
#include <algorithm>
#include <sstream>

namespace melvin_sensory {

// ==================== EMBEDDING QUANTIZATION ====================

// Quantize float embedding to uint8 vector (for storage efficiency)
inline std::vector<uint8_t> quantize_embedding(const std::vector<float>& embedding) {
    std::vector<uint8_t> quantized;
    quantized.reserve(embedding.size());
    
    // Find min/max for normalization
    float min_val = *std::min_element(embedding.begin(), embedding.end());
    float max_val = *std::max_element(embedding.begin(), embedding.end());
    float range = max_val - min_val;
    
    if (range < 1e-6f) range = 1.0f; // Avoid division by zero
    
    // Quantize to 0-255
    for (float val : embedding) {
        float normalized = (val - min_val) / range;
        quantized.push_back(static_cast<uint8_t>(normalized * 255.0f));
    }
    
    return quantized;
}

// Dequantize back to float (approximate)
inline std::vector<float> dequantize_embedding(const std::vector<uint8_t>& quantized) {
    std::vector<float> embedding;
    embedding.reserve(quantized.size());
    
    for (uint8_t val : quantized) {
        embedding.push_back(val / 255.0f);
    }
    
    return embedding;
}

// ==================== DISTANCE METRICS ====================

// Cosine similarity between float vectors
inline float cosine_similarity(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) return 0.0f;
    
    float dot = 0.0f, norm_a = 0.0f, norm_b = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        dot += a[i] * b[i];
        norm_a += a[i] * a[i];
        norm_b += b[i] * b[i];
    }
    
    norm_a = std::sqrt(norm_a);
    norm_b = std::sqrt(norm_b);
    
    if (norm_a < 1e-6f || norm_b < 1e-6f) return 0.0f;
    
    return dot / (norm_a * norm_b);
}

// Euclidean distance
inline float euclidean_distance(const std::vector<float>& a, const std::vector<float>& b) {
    if (a.size() != b.size()) return std::numeric_limits<float>::max();
    
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    
    return std::sqrt(sum);
}

// ==================== MOCK DATA GENERATORS ====================

// Generate mock CLIP embedding (512-dim)
inline std::vector<float> generate_mock_clip_embedding(const std::string& label, int seed = 0) {
    std::mt19937 rng(std::hash<std::string>{}(label) + seed);
    std::normal_distribution<float> dist(0.0f, 1.0f);
    
    std::vector<float> embedding(512);
    for (auto& val : embedding) {
        val = dist(rng);
    }
    
    // Normalize to unit vector
    float norm = 0.0f;
    for (float val : embedding) {
        norm += val * val;
    }
    norm = std::sqrt(norm);
    
    for (auto& val : embedding) {
        val /= norm;
    }
    
    return embedding;
}

// Generate mock audio quantization codes
inline std::vector<uint8_t> generate_mock_audio_codes(const std::string& label, int num_frames = 50) {
    std::mt19937 rng(std::hash<std::string>{}(label));
    std::uniform_int_distribution<> dist(0, 255);
    
    std::vector<uint8_t> codes;
    codes.reserve(num_frames);
    
    for (int i = 0; i < num_frames; ++i) {
        codes.push_back(static_cast<uint8_t>(dist(rng)));
    }
    
    return codes;
}

// ==================== ENCODING HELPERS ====================

// Encode vector to hex string (for storage/display)
inline std::string encode_to_hex(const std::vector<uint8_t>& data) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for (uint8_t byte : data) {
        ss << std::setw(2) << static_cast<int>(byte);
    }
    return ss.str();
}

// Decode hex string to vector
inline std::vector<uint8_t> decode_from_hex(const std::string& hex) {
    std::vector<uint8_t> data;
    for (size_t i = 0; i + 1 < hex.length(); i += 2) {
        std::string byte_str = hex.substr(i, 2);
        uint8_t byte = static_cast<uint8_t>(std::stoi(byte_str, nullptr, 16));
        data.push_back(byte);
    }
    return data;
}

// ==================== FEATURE EXTRACTION ====================

// Extract simple visual features (mock for now)
inline std::vector<float> extract_visual_features(const std::string& image_path) {
    // In production, this would call OpenCV or similar
    // For now, generate deterministic features based on filename
    return generate_mock_clip_embedding(image_path);
}

// Extract simple audio features (mock for now)
inline std::vector<uint8_t> extract_audio_features(const std::string& audio_path) {
    // In production, this would do log-mel spectrogram + VQ
    // For now, generate deterministic codes based on filename
    return generate_mock_audio_codes(audio_path);
}

} // namespace melvin_sensory

