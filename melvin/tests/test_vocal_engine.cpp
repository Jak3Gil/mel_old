/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  Test Suite - Vocal Engine (Biological-Style Speech Synthesis)            ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "../audio/vocal_engine.h"
#include "../audio/phoneme_graph.h"
#include <iostream>
#include <fstream>
#include <cassert>
#include <cmath>

using namespace melvin;
using namespace melvin::audio;

// Test utilities
void assert_true(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "❌ FAILED: " << message << std::endl;
        exit(1);
    }
    std::cout << "✅ PASSED: " << message << std::endl;
}

// ============================================================================
// TEST 1: Phoneme Graph Initialization
// ============================================================================

void test_phoneme_graph_init() {
    std::cout << "\n🧪 Test 1: Phoneme Graph Initialization" << std::endl;
    
    PhonemeGraph pg;
    
    assert_true(pg.phoneme_count() > 0, "Default phonemes loaded");
    assert_true(pg.phoneme_count() >= 10, "Sufficient phonemes for speech");
    
    std::cout << "   Loaded " << pg.phoneme_count() << " phonemes" << std::endl;
}

// ============================================================================
// TEST 2: Phoneme Retrieval
// ============================================================================

void test_phoneme_retrieval() {
    std::cout << "\n🧪 Test 2: Phoneme Retrieval" << std::endl;
    
    PhonemeGraph pg;
    
    const PhonemeNode* m_phoneme = pg.get_phoneme("m");
    assert_true(m_phoneme != nullptr, "Phoneme 'm' exists");
    
    if (m_phoneme) {
        assert_true(m_phoneme->symbol == "m", "Phoneme symbol correct");
        assert_true(m_phoneme->formants[0] > 0, "Formant F1 set");
        assert_true(m_phoneme->duration_ms > 0, "Duration set");
        
        std::cout << "   Phoneme 'm': F1=" << m_phoneme->formants[0]
                  << " F2=" << m_phoneme->formants[1]
                  << " F3=" << m_phoneme->formants[2] << " Hz" << std::endl;
    }
}

// ============================================================================
// TEST 3: Word → Phoneme Sequence
// ============================================================================

void test_word_to_phonemes() {
    std::cout << "\n🧪 Test 3: Word → Phoneme Sequence" << std::endl;
    
    PhonemeGraph pg;
    
    auto phonemes = pg.get_sequence("melvin");
    
    assert_true(!phonemes.empty(), "Phoneme sequence generated");
    assert_true(phonemes.size() > 0, "Multiple phonemes for word");
    
    std::cout << "   'melvin' = " << phonemes.size() << " phonemes" << std::endl;
}

// ============================================================================
// TEST 4: Single Phoneme Synthesis
// ============================================================================

void test_single_phoneme_synthesis() {
    std::cout << "\n🧪 Test 4: Single Phoneme Synthesis" << std::endl;
    
    VocalEngine engine;
    
    PhonemeNode test_phoneme;
    test_phoneme.symbol = "a";
    test_phoneme.formants[0] = 700.0f;
    test_phoneme.formants[1] = 1220.0f;
    test_phoneme.formants[2] = 2600.0f;
    test_phoneme.duration_ms = 100.0f;
    
    auto audio = engine.synthesize_phoneme(test_phoneme);
    
    assert_true(!audio.empty(), "Audio samples generated");
    assert_true(audio.size() > 1000, "Sufficient samples for phoneme");
    
    // Check audio is normalized
    bool in_range = true;
    for (float sample : audio) {
        if (sample < -1.0f || sample > 1.0f) {
            in_range = false;
            break;
        }
    }
    assert_true(in_range, "Audio samples normalized");
    
    std::cout << "   Generated " << audio.size() << " samples" << std::endl;
}

// ============================================================================
// TEST 5: Sequence Synthesis
// ============================================================================

void test_sequence_synthesis() {
    std::cout << "\n🧪 Test 5: Sequence Synthesis" << std::endl;
    
    PhonemeGraph pg;
    VocalEngine engine;
    
    auto phonemes = pg.get_sequence("hello");
    auto audio = engine.synthesize_sequence(phonemes);
    
    assert_true(!audio.empty(), "Sequence audio generated");
    assert_true(audio.size() > phonemes.size() * 1000, "Audio longer than single phoneme");
    
    std::cout << "   Generated " << audio.size() << " samples for " 
              << phonemes.size() << " phonemes" << std::endl;
}

// ============================================================================
// TEST 6: Full Word Synthesis
// ============================================================================

void test_full_word_synthesis() {
    std::cout << "\n🧪 Test 6: Full Word Synthesis" << std::endl;
    
    VocalEngine engine;
    
    auto audio = engine.speak("melvin");
    
    assert_true(!audio.empty(), "Word audio generated");
    assert_true(engine.get_synthesis_count() >= 1, "Synthesis count incremented");
    
    std::cout << "   Synthesized 'melvin': " << audio.size() << " samples" << std::endl;
}

// ============================================================================
// TEST 7: WAV File Output
// ============================================================================

void test_wav_output() {
    std::cout << "\n🧪 Test 7: WAV File Output" << std::endl;
    
    VocalEngine engine;
    
    bool success = engine.speak_to_file("test", "test_vocal.wav");
    
    assert_true(success, "WAV file created");
    
    // Check file exists
    std::ifstream file("test_vocal.wav");
    bool exists = file.good();
    file.close();
    
    assert_true(exists, "WAV file exists");
    
    std::cout << "   WAV file created successfully" << std::endl;
}

// ============================================================================
// TEST 8: Custom Pronunciation
// ============================================================================

void test_custom_pronunciation() {
    std::cout << "\n🧪 Test 8: Custom Pronunciation" << std::endl;
    
    PhonemeGraph pg;
    
    // Add custom word pronunciation
    pg.add_word_pronunciation("hello", {"hh", "eh", "l", "ow"});
    
    auto phonemes = pg.get_sequence("hello");
    
    assert_true(!phonemes.empty(), "Custom pronunciation retrieved");
    assert_true(pg.word_count() >= 1, "Word stored");
    
    std::cout << "   Custom pronunciation stored and retrieved" << std::endl;
}

// ============================================================================
// TEST 9: Phoneme Addition
// ============================================================================

void test_phoneme_addition() {
    std::cout << "\n🧪 Test 9: Phoneme Addition" << std::endl;
    
    PhonemeGraph pg;
    
    size_t before = pg.phoneme_count();
    
    // Add new phoneme
    PhonemeNode new_phoneme;
    new_phoneme.symbol = "test";
    new_phoneme.formants[0] = 600.0f;
    new_phoneme.formants[1] = 1400.0f;
    new_phoneme.formants[2] = 2200.0f;
    
    pg.add_phoneme("test", new_phoneme);
    
    size_t after = pg.phoneme_count();
    
    assert_true(after > before, "Phoneme added");
    
    const PhonemeNode* retrieved = pg.get_phoneme("test");
    assert_true(retrieved != nullptr, "New phoneme retrievable");
    
    std::cout << "   Phoneme count: " << before << " → " << after << std::endl;
}

// ============================================================================
// TEST 10: Multiple Word Synthesis
// ============================================================================

void test_multiple_words() {
    std::cout << "\n🧪 Test 10: Multiple Word Synthesis" << std::endl;
    
    VocalEngine engine;
    
    auto audio1 = engine.speak("hello");
    auto audio2 = engine.speak("world");
    
    assert_true(!audio1.empty() && !audio2.empty(), "Both words synthesized");
    assert_true(engine.get_synthesis_count() == 2, "Synthesis count correct");
    
    std::cout << "   Synthesized 2 words successfully" << std::endl;
}

// ============================================================================
// MAIN TEST RUNNER
// ============================================================================

int main() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  🧪 Vocal Engine Test Suite                              ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    
    try {
        test_phoneme_graph_init();
        test_phoneme_retrieval();
        test_word_to_phonemes();
        test_single_phoneme_synthesis();
        test_sequence_synthesis();
        test_full_word_synthesis();
        test_wav_output();
        test_custom_pronunciation();
        test_phoneme_addition();
        test_multiple_words();
        
        std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║  ✅ ALL VOCAL ENGINE TESTS PASSED (10/10)                ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ TEST SUITE FAILED: " << e.what() << std::endl;
        return 1;
    }
}

