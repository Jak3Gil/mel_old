#pragma once

#include "../core/atomic_graph.h"
#include <string>
#include <vector>
#include <unordered_map>

namespace melvin {
namespace audio {

/**
 * PhonemeNode - Acoustic representation of a single phoneme
 * 
 * Stores biophysical parameters for vocal tract synthesis:
 * - Formants: Resonant frequencies of vocal tract
 * - Duration: Typical length of phoneme
 * - Amplitude: Loudness envelope
 * - Articulation: Vocal tract configuration
 */
struct PhonemeNode {
    uint64_t id;                    // Graph node ID
    std::string symbol;             // IPA symbol (e.g., /m/, /e/, /l/)
    
    // Formant frequencies (Hz) - defines vocal tract shape
    float formants[3];              // F1, F2, F3
    
    // Temporal properties
    float duration_ms;              // Average duration in milliseconds
    
    // Amplitude properties
    float amplitude;                // Base amplitude (0.0 to 1.0)
    std::vector<float> envelope;    // Amplitude over time (optional)
    
    // Articulatory features
    float nasality;                 // Nasal resonance (0.0 to 1.0)
    float voicing;                  // Vocal cord vibration (0.0 to 1.0)
    float breathiness;              // Glottal noise ratio (0.0 to 1.0)
    
    // Transition smoothing
    float coarticulation_strength;  // How much it blends with neighbors
    
    PhonemeNode() 
        : id(0), duration_ms(100.0f), amplitude(1.0f),
          nasality(0.0f), voicing(1.0f), breathiness(0.1f),
          coarticulation_strength(0.5f) {
        formants[0] = 500.0f;   // Default F1
        formants[1] = 1500.0f;  // Default F2
        formants[2] = 2500.0f;  // Default F3
    }
};

/**
 * PhonemeGraph - Knowledge graph of phonemes and their relationships
 * 
 * Stores:
 * - Individual phoneme nodes with acoustic parameters
 * - Coarticulation rules (how phonemes blend)
 * - Word → phoneme sequence mappings
 * - Learning history
 * 
 * Enables:
 * - Dynamic word pronunciation
 * - Phoneme learning from audio
 * - Natural coarticulation
 * - Voice style consistency
 */
class PhonemeGraph {
public:
    PhonemeGraph();
    explicit PhonemeGraph(AtomicGraph& graph);
    ~PhonemeGraph();
    
    // ========================================================================
    // PHONEME MANAGEMENT
    // ========================================================================
    
    /**
     * Add or update a phoneme
     * 
     * @param symbol IPA symbol (e.g., "m", "e", "l")
     * @param data Phoneme acoustic parameters
     * @return Phoneme node ID
     */
    uint64_t add_phoneme(const std::string& symbol, const PhonemeNode& data);
    
    /**
     * Get phoneme by symbol
     * 
     * @param symbol IPA symbol
     * @return Phoneme data (nullptr if not found)
     */
    const PhonemeNode* get_phoneme(const std::string& symbol) const;
    
    /**
     * Get phoneme by ID
     */
    const PhonemeNode* get_phoneme_by_id(uint64_t id) const;
    
    // ========================================================================
    // WORD → PHONEME MAPPING
    // ========================================================================
    
    /**
     * Get phoneme sequence for a word
     * 
     * @param word Word to pronounce
     * @return Sequence of phonemes
     */
    std::vector<PhonemeNode> get_sequence(const std::string& word);
    
    /**
     * Add word pronunciation
     * 
     * @param word Word text
     * @param phonemes Phoneme symbols (e.g., {"m", "eh", "l", "v", "ih", "n"})
     */
    void add_word_pronunciation(const std::string& word, 
                               const std::vector<std::string>& phonemes);
    
    /**
     * Generate phoneme sequence from text (using rules)
     * 
     * @param text Text to convert
     * @return Phoneme symbols
     */
    std::vector<std::string> text_to_phonemes(const std::string& text);
    
    // ========================================================================
    // LEARNING
    // ========================================================================
    
    /**
     * Learn phonemes from audio sample
     * 
     * @param word Word that was spoken
     * @param samples Audio samples
     * @param sample_rate Sample rate (Hz)
     */
    void learn_from_audio(const std::string& word, 
                         const std::vector<float>& samples,
                         uint32_t sample_rate = 16000);
    
    /**
     * Learn coarticulation pattern
     * 
     * @param phoneme1 First phoneme
     * @param phoneme2 Second phoneme
     * @param blend_factor How much they blend (0.0 to 1.0)
     */
    void learn_coarticulation(const std::string& phoneme1,
                             const std::string& phoneme2,
                             float blend_factor);
    
    // ========================================================================
    // STATISTICS
    // ========================================================================
    
    size_t phoneme_count() const { return phonemes_.size(); }
    size_t word_count() const { return word_pronunciations_.size(); }
    
    void print_stats() const;
    
    // ========================================================================
    // PERSISTENCE
    // ========================================================================
    
    /**
     * Save phoneme graph to files
     */
    void save(const std::string& phoneme_file, const std::string& words_file) const;
    
    /**
     * Load phoneme graph from files
     */
    void load(const std::string& phoneme_file, const std::string& words_file);
    
private:
    AtomicGraph* graph_;  // Optional graph integration
    
    // Phoneme storage
    std::unordered_map<std::string, PhonemeNode> phonemes_;
    std::unordered_map<uint64_t, std::string> id_to_symbol_;
    
    // Word pronunciations
    std::unordered_map<std::string, std::vector<std::string>> word_pronunciations_;
    
    // Coarticulation rules
    std::unordered_map<std::string, float> coarticulation_map_;
    
    // Helper methods
    void initialize_default_phonemes();
    std::vector<std::string> simple_phoneme_rules(const std::string& word);
    std::string coarticulation_key(const std::string& p1, const std::string& p2) const;
};

} // namespace audio
} // namespace melvin

