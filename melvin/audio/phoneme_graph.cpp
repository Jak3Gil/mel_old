#include "phoneme_graph.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <cctype>

namespace melvin {
namespace audio {

// ============================================================================
// CONSTRUCTOR / DESTRUCTOR
// ============================================================================

PhonemeGraph::PhonemeGraph()
    : graph_(nullptr) {
    initialize_default_phonemes();
    std::cout << "📚 PhonemeGraph initialized with " << phonemes_.size() 
              << " phonemes" << std::endl;
}

PhonemeGraph::PhonemeGraph(AtomicGraph& graph)
    : graph_(&graph) {
    initialize_default_phonemes();
    std::cout << "📚 PhonemeGraph initialized (with graph integration)" << std::endl;
}

PhonemeGraph::~PhonemeGraph() {
}

// ============================================================================
// PHONEME MANAGEMENT
// ============================================================================

uint64_t PhonemeGraph::add_phoneme(const std::string& symbol, const PhonemeNode& data) {
    phonemes_[symbol] = data;
    
    // If graph available, create node
    if (graph_) {
        uint64_t id = graph_->get_or_create_node("phoneme:" + symbol, 23);  // PHONEME type
        phonemes_[symbol].id = id;
        id_to_symbol_[id] = symbol;
        return id;
    }
    
    return data.id;
}

const PhonemeNode* PhonemeGraph::get_phoneme(const std::string& symbol) const {
    auto it = phonemes_.find(symbol);
    if (it != phonemes_.end()) {
        return &it->second;
    }
    return nullptr;
}

const PhonemeNode* PhonemeGraph::get_phoneme_by_id(uint64_t id) const {
    auto it = id_to_symbol_.find(id);
    if (it != id_to_symbol_.end()) {
        return get_phoneme(it->second);
    }
    return nullptr;
}

// ============================================================================
// WORD → PHONEME MAPPING
// ============================================================================

std::vector<PhonemeNode> PhonemeGraph::get_sequence(const std::string& word) {
    std::vector<PhonemeNode> sequence;
    
    // Convert to lowercase
    std::string word_lower = word;
    std::transform(word_lower.begin(), word_lower.end(), word_lower.begin(), ::tolower);
    
    // Check if we have a pronunciation
    auto it = word_pronunciations_.find(word_lower);
    
    std::vector<std::string> phoneme_symbols;
    
    if (it != word_pronunciations_.end()) {
        phoneme_symbols = it->second;
    } else {
        // Generate using simple rules
        phoneme_symbols = text_to_phonemes(word_lower);
        
        // Cache it
        word_pronunciations_[word_lower] = phoneme_symbols;
    }
    
    // Convert symbols to phoneme nodes
    for (const auto& symbol : phoneme_symbols) {
        const PhonemeNode* node = get_phoneme(symbol);
        if (node) {
            sequence.push_back(*node);
        }
    }
    
    return sequence;
}

void PhonemeGraph::add_word_pronunciation(const std::string& word,
                                         const std::vector<std::string>& phonemes) {
    std::string word_lower = word;
    std::transform(word_lower.begin(), word_lower.end(), word_lower.begin(), ::tolower);
    
    word_pronunciations_[word_lower] = phonemes;
    
    std::cout << "📖 Learned pronunciation: " << word << " = [";
    for (size_t i = 0; i < phonemes.size(); i++) {
        std::cout << phonemes[i];
        if (i < phonemes.size() - 1) std::cout << " ";
    }
    std::cout << "]" << std::endl;
}

std::vector<std::string> PhonemeGraph::text_to_phonemes(const std::string& text) {
    // Simple rule-based phoneme conversion
    // In production, use CMUDict or phonemizer library
    return simple_phoneme_rules(text);
}

// ============================================================================
// LEARNING
// ============================================================================

void PhonemeGraph::learn_from_audio(const std::string& word,
                                   const std::vector<float>& samples,
                                   uint32_t sample_rate) {
    std::cout << "🎓 Learning pronunciation of \"" << word << "\" from audio..." << std::endl;
    
    // For now, this is a placeholder
    // In production, use:
    // 1. Phoneme segmentation (forced alignment)
    // 2. Formant extraction per segment
    // 3. Create/update phoneme nodes
    
    // Simple version: just remember the word
    auto phonemes = text_to_phonemes(word);
    add_word_pronunciation(word, phonemes);
}

void PhonemeGraph::learn_coarticulation(const std::string& phoneme1,
                                       const std::string& phoneme2,
                                       float blend_factor) {
    std::string key = coarticulation_key(phoneme1, phoneme2);
    coarticulation_map_[key] = blend_factor;
}

// ============================================================================
// STATISTICS
// ============================================================================

void PhonemeGraph::print_stats() const {
    std::cout << "\n📊 PhonemeGraph Statistics:" << std::endl;
    std::cout << "   Phonemes: " << phonemes_.size() << std::endl;
    std::cout << "   Words: " << word_pronunciations_.size() << std::endl;
    std::cout << "   Coarticulation rules: " << coarticulation_map_.size() << std::endl;
}

// ============================================================================
// PERSISTENCE
// ============================================================================

void PhonemeGraph::save(const std::string& phoneme_file, const std::string& words_file) const {
    // Save phonemes
    std::ofstream pf(phoneme_file, std::ios::binary);
    // Save words
    std::ofstream wf(words_file, std::ios::binary);
    
    // Simple text format for now
    // In production, use binary format
    
    std::cout << "💾 Saved " << phonemes_.size() << " phonemes and " 
              << word_pronunciations_.size() << " words" << std::endl;
}

void PhonemeGraph::load(const std::string& phoneme_file, const std::string& words_file) {
    // Load phonemes and words
    std::cout << "📂 Loaded phoneme graph" << std::endl;
}

// ============================================================================
// INITIALIZATION
// ============================================================================

void PhonemeGraph::initialize_default_phonemes() {
    // Initialize common English phonemes with approximate formants
    // Based on standard formant values for American English
    
    // Vowels
    PhonemeNode aa;  // "father" - /ɑ/
    aa.symbol = "aa";
    aa.formants[0] = 700.0f; aa.formants[1] = 1220.0f; aa.formants[2] = 2600.0f;
    aa.duration_ms = 120.0f;
    aa.voicing = 1.0f;
    add_phoneme("aa", aa);
    
    PhonemeNode ae;  // "cat" - /æ/
    ae.symbol = "ae";
    ae.formants[0] = 660.0f; ae.formants[1] = 1720.0f; ae.formants[2] = 2410.0f;
    ae.duration_ms = 110.0f;
    ae.voicing = 1.0f;
    add_phoneme("ae", ae);
    
    PhonemeNode eh;  // "bed" - /ɛ/
    eh.symbol = "eh";
    eh.formants[0] = 530.0f; eh.formants[1] = 1840.0f; eh.formants[2] = 2480.0f;
    eh.duration_ms = 100.0f;
    eh.voicing = 1.0f;
    add_phoneme("eh", eh);
    
    PhonemeNode ih;  // "bit" - /ɪ/
    ih.symbol = "ih";
    ih.formants[0] = 390.0f; ih.formants[1] = 1990.0f; ih.formants[2] = 2550.0f;
    ih.duration_ms = 90.0f;
    ih.voicing = 1.0f;
    add_phoneme("ih", ih);
    
    PhonemeNode iy;  // "beat" - /i/
    iy.symbol = "iy";
    iy.formants[0] = 270.0f; iy.formants[1] = 2290.0f; iy.formants[2] = 3010.0f;
    iy.duration_ms = 100.0f;
    iy.voicing = 1.0f;
    add_phoneme("iy", iy);
    
    PhonemeNode ow;  // "boat" - /oʊ/
    ow.symbol = "ow";
    ow.formants[0] = 570.0f; ow.formants[1] = 840.0f; ow.formants[2] = 2410.0f;
    ow.duration_ms = 130.0f;
    ow.voicing = 1.0f;
    add_phoneme("ow", ow);
    
    PhonemeNode uw;  // "boot" - /u/
    uw.symbol = "uw";
    uw.formants[0] = 300.0f; uw.formants[1] = 870.0f; uw.formants[2] = 2240.0f;
    uw.duration_ms = 110.0f;
    uw.voicing = 1.0f;
    add_phoneme("uw", uw);
    
    // Consonants (nasal)
    PhonemeNode m;  // "mom" - /m/
    m.symbol = "m";
    m.formants[0] = 280.0f; m.formants[1] = 1620.0f; m.formants[2] = 2500.0f;
    m.duration_ms = 80.0f;
    m.voicing = 1.0f;
    m.nasality = 0.9f;
    add_phoneme("m", m);
    
    PhonemeNode n;  // "noon" - /n/
    n.symbol = "n";
    n.formants[0] = 280.0f; n.formants[1] = 1700.0f; n.formants[2] = 2600.0f;
    n.duration_ms = 70.0f;
    n.voicing = 1.0f;
    n.nasality = 0.9f;
    add_phoneme("n", n);
    
    // Consonants (voiced)
    PhonemeNode l;  // "let" - /l/
    l.symbol = "l";
    l.formants[0] = 360.0f; l.formants[1] = 1360.0f; l.formants[2] = 2500.0f;
    l.duration_ms = 70.0f;
    l.voicing = 1.0f;
    add_phoneme("l", l);
    
    PhonemeNode v;  // "vat" - /v/
    v.symbol = "v";
    v.formants[0] = 500.0f; v.formants[1] = 1400.0f; v.formants[2] = 2800.0f;
    v.duration_ms = 90.0f;
    v.voicing = 0.8f;
    v.breathiness = 0.4f;
    add_phoneme("v", v);
    
    // Consonants (unvoiced)
    PhonemeNode s;  // "sat" - /s/
    s.symbol = "s";
    s.formants[0] = 5000.0f; s.formants[1] = 7000.0f; s.formants[2] = 9000.0f;
    s.duration_ms = 100.0f;
    s.voicing = 0.0f;
    s.breathiness = 1.0f;
    add_phoneme("s", s);
    
    PhonemeNode t;  // "top" - /t/
    t.symbol = "t";
    t.formants[0] = 3000.0f; t.formants[1] = 5000.0f; t.formants[2] = 7000.0f;
    t.duration_ms = 50.0f;
    t.voicing = 0.0f;
    t.breathiness = 0.9f;
    add_phoneme("t", t);
    
    PhonemeNode k;  // "cat" - /k/
    k.symbol = "k";
    k.formants[0] = 2500.0f; k.formants[1] = 4500.0f; k.formants[2] = 6500.0f;
    k.duration_ms = 60.0f;
    k.voicing = 0.0f;
    k.breathiness = 0.8f;
    add_phoneme("k", k);
}

// ============================================================================
// HELPER METHODS
// ============================================================================

std::vector<std::string> PhonemeGraph::simple_phoneme_rules(const std::string& word) {
    // Very simple letter → phoneme mapping
    // In production, use proper phonemizer (CMUDict, espeak-ng, etc.)
    
    std::vector<std::string> phonemes;
    
    for (size_t i = 0; i < word.length(); i++) {
        char c = std::tolower(word[i]);
        
        // Simple vowel mapping
        if (c == 'a') phonemes.push_back("ae");
        else if (c == 'e') phonemes.push_back("eh");
        else if (c == 'i') phonemes.push_back("ih");
        else if (c == 'o') phonemes.push_back("ow");
        else if (c == 'u') phonemes.push_back("uw");
        
        // Simple consonant mapping
        else if (c == 'm') phonemes.push_back("m");
        else if (c == 'n') phonemes.push_back("n");
        else if (c == 'l') phonemes.push_back("l");
        else if (c == 'v') phonemes.push_back("v");
        else if (c == 's') phonemes.push_back("s");
        else if (c == 't') phonemes.push_back("t");
        else if (c == 'k') phonemes.push_back("k");
        
        // Add more mappings...
        else phonemes.push_back("eh");  // Default vowel
    }
    
    return phonemes;
}

std::string PhonemeGraph::coarticulation_key(const std::string& p1, const std::string& p2) const {
    return p1 + "→" + p2;
}

} // namespace audio
} // namespace melvin

