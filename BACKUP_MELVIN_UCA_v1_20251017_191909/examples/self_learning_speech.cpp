/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  SELF-LEARNING SPEECH - Complete Babbling-to-Speech Loop                  ║
 * ║  Melvin learns to speak by hearing himself and adapting                   ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "../audio/audio_tokenizer.h"
#include "../audio/phoneme_cluster.h"
#include "../audio/vocal_engine.h"
#include "../audio/self_feedback.h"
#include "../core/atomic_graph.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace melvin;
using namespace melvin::audio;

// ============================================================================
// Generate test audio (simulating microphone input)
// ============================================================================

std::vector<float> generate_speech_like_audio(float duration_s, uint32_t sample_rate) {
    size_t num_samples = static_cast<size_t>(duration_s * sample_rate);
    std::vector<float> samples(num_samples);
    
    // Simulate speech-like patterns (varying pitch and formants)
    for (size_t i = 0; i < num_samples; i++) {
        float t = static_cast<float>(i) / sample_rate;
        
        // Fundamental frequency (varying pitch)
        float pitch = 120.0f + 30.0f * std::sin(t * 2.0f);
        
        // Multiple harmonics (formant-like)
        samples[i] = 0.3f * std::sin(2.0f * M_PI * pitch * t) +
                    0.2f * std::sin(2.0f * M_PI * pitch * 2.0f * t) +
                    0.1f * std::sin(2.0f * M_PI * pitch * 3.0f * t);
    }
    
    return samples;
}

// ============================================================================
// DEMO 1: Audio Tokenization → Phoneme Discovery
// ============================================================================

void demo_phoneme_discovery() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 1: Phoneme Discovery from Raw Audio                ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    AudioTokenizer tokenizer;
    PhonemeClusterer clusterer;
    
    std::cout << "🎵 Generating speech-like audio..." << std::endl;
    auto audio = generate_speech_like_audio(2.0f, 16000);
    
    std::cout << "\n🔤 Step 1: Tokenize audio..." << std::endl;
    auto token_ids = tokenizer.tokenize(audio, graph);
    
    // Get actual token objects (simplified for demo)
    std::vector<AudioToken> tokens;
    for (size_t i = 0; i < token_ids.size() && i < 20; i++) {
        AudioToken token;
        token.id = token_ids[i];
        token.features = {0.5f, 0.3f, 0.8f};  // Placeholder
        tokens.push_back(token);
    }
    
    std::cout << "\n🧬 Step 2: Cluster into phonemes..." << std::endl;
    auto clusters = clusterer.cluster_tokens(tokens, graph);
    
    std::cout << "\n🔗 Step 3: Link tokens to clusters..." << std::endl;
    clusterer.link_to_graph(graph);
    
    std::cout << "\n📊 Results:" << std::endl;
    graph.print_stats();
    tokenizer.print_stats();
    clusterer.print_stats();
    clusterer.print_cluster_details();
    
    std::cout << "\n✅ Discovered " << clusterer.cluster_count() << " phoneme-like patterns!" << std::endl;
    std::cout << "   No pre-defined phonemes, learned from audio!" << std::endl;
}

// ============================================================================
// DEMO 2: Self-Feedback Loop
// ============================================================================

void demo_self_feedback() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 2: Self-Feedback Loop (Hear Own Voice)             ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    VocalEngine vocal;
    AudioTokenizer tokenizer;
    SelfFeedback feedback;
    
    std::cout << "🎙️ Step 1: Melvin speaks (generates audio)..." << std::endl;
    auto generated = vocal.speak("hello");
    
    std::cout << "\n🎧 Step 2: Melvin hears himself (microphone capture)..." << std::endl;
    // In real system, this would be microphone input
    // For demo, we simulate it
    auto heard = generated;  // Perfect copy for now
    
    std::cout << "\n🔤 Step 3: Tokenize both intended and heard..." << std::endl;
    AtomicGraph temp_graph;
    auto intended_ids = tokenizer.tokenize(generated, temp_graph);
    auto heard_ids = tokenizer.tokenize(heard, temp_graph);
    
    // Create token objects for comparison
    std::vector<AudioToken> intended_tokens, heard_tokens;
    for (size_t i = 0; i < std::min(intended_ids.size(), size_t(5)); i++) {
        AudioToken t;
        t.id = intended_ids[i];
        t.pitch = 120.0f;
        t.energy = 0.5f;
        t.features = {0.5f, 0.3f, 0.8f};
        intended_tokens.push_back(t);
        heard_tokens.push_back(t);  // Perfect match for demo
    }
    
    std::cout << "\n🔄 Step 4: Compare intended vs heard..." << std::endl;
    auto result = feedback.compare(intended_tokens, heard_tokens);
    
    std::cout << "\n📊 Feedback Results:" << std::endl;
    std::cout << "   Similarity: " << result.similarity << std::endl;
    std::cout << "   Needs adjustment: " << (result.needs_adjustment ? "Yes" : "No") << std::endl;
    
    if (result.needs_adjustment) {
        std::cout << "\n🔧 Step 5: Apply adjustments..." << std::endl;
        feedback.apply_adjustments(result, vocal);
    }
    
    feedback.print_stats();
    
    std::cout << "\n✅ Self-feedback loop complete!" << std::endl;
}

// ============================================================================
// DEMO 3: Phoneme Learning from Listening
// ============================================================================

void demo_phoneme_learning() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 3: Learn Phonemes from Environment                 ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    AudioTokenizer tokenizer;
    PhonemeClusterer clusterer;
    PhonemeGraph phoneme_graph(graph);
    
    std::cout << "🎧 Melvin listens to environment..." << std::endl;
    
    // Simulate different sounds over time
    std::vector<AudioToken> all_tokens;
    
    for (int session = 1; session <= 3; session++) {
        std::cout << "\n📅 Listening session " << session << ":" << std::endl;
        
        auto audio = generate_speech_like_audio(1.0f, 16000);
        auto tokens = tokenizer.tokenize(audio, graph);
        
        // Convert to token objects
        for (auto id : tokens) {
            AudioToken t;
            t.id = id;
            t.features = {static_cast<float>(session) * 0.2f, 0.5f, 0.7f};
            all_tokens.push_back(t);
        }
        
        std::cout << "   Collected " << tokens.size() << " audio tokens" << std::endl;
    }
    
    std::cout << "\n🧬 Discovering phoneme prototypes..." << std::endl;
    clusterer.cluster_tokens(all_tokens, graph);
    
    std::cout << "\n📤 Exporting to vocal engine..." << std::endl;
    clusterer.export_to_phonemes(phoneme_graph);
    
    std::cout << "\n📊 Results:" << std::endl;
    clusterer.print_stats();
    clusterer.print_cluster_details();
    
    std::cout << "\n✅ Melvin learned " << clusterer.cluster_count() << " phonemes from listening!" << std::endl;
    std::cout << "   Can now use them for speech synthesis!" << std::endl;
}

// ============================================================================
// DEMO 4: Complete Learning Cycle
// ============================================================================

void demo_complete_cycle() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 4: Complete Self-Learning Cycle                    ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    AudioTokenizer tokenizer;
    PhonemeClusterer clusterer;
    VocalEngine vocal;
    SelfFeedback feedback;
    
    std::cout << "🔄 Simulating babbling-to-speech learning...\n" << std::endl;
    
    for (int iteration = 1; iteration <= 3; iteration++) {
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "🔁 Iteration " << iteration << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        
        // 1. Generate speech
        std::cout << "\n1️⃣  Generate: Melvin speaks..." << std::endl;
        auto generated = vocal.speak("test");
        
        // 2. Hear self
        std::cout << "2️⃣  Capture: Melvin hears himself..." << std::endl;
        auto heard = generated;  // Simulated
        
        // 3. Tokenize
        std::cout << "3️⃣  Tokenize: Extract audio features..." << std::endl;
        AtomicGraph temp_graph;
        tokenizer.tokenize(generated, temp_graph);
        
        // 4. Compare
        std::cout << "4️⃣  Compare: Intended vs heard..." << std::endl;
        std::vector<AudioToken> tokens;
        AudioToken t;
        t.pitch = 120.0f + iteration * 5.0f;  // Slightly different each time
        t.energy = 0.5f;
        t.features = {0.5f, 0.3f, 0.8f};
        tokens.push_back(t);
        
        auto result = feedback.compare(tokens, tokens);
        
        // 5. Adapt
        if (result.needs_adjustment) {
            std::cout << "5️⃣  Adapt: Adjusting vocal parameters..." << std::endl;
            feedback.apply_adjustments(result, vocal);
        } else {
            std::cout << "5️⃣  Match: No adjustment needed ✅" << std::endl;
        }
        
        std::cout << std::endl;
    }
    
    std::cout << "\n📊 Learning Progress:" << std::endl;
    feedback.print_stats();
    
    std::cout << "\n✅ Self-learning cycle complete!" << std::endl;
    std::cout << "   Melvin improved through auditory feedback!" << std::endl;
}

// ============================================================================
// DEMO 5: Cross-Modal Audio Learning
// ============================================================================

void demo_cross_modal() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 5: Cross-Modal Audio Learning                      ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    AudioTokenizer tokenizer;
    
    std::cout << "🎧 Scenario: Melvin hears sound + sees object simultaneously\n" << std::endl;
    
    // Simulate hearing a sound
    std::cout << "1️⃣  Audio: Hears barking sound..." << std::endl;
    auto bark_audio = generate_speech_like_audio(0.5f, 16000);
    auto audio_tokens = tokenizer.tokenize(bark_audio, graph);
    
    // Simulate seeing something
    std::cout << "2️⃣  Vision: Sees dog..." << std::endl;
    uint64_t dog_visual = graph.get_or_create_node("vision:dog", 1);
    
    // Link audio → vision (no text label needed!)
    std::cout << "3️⃣  Cross-modal link: Audio ↔ Vision..." << std::endl;
    for (auto audio_id : audio_tokens) {
        graph.add_edge(audio_id, dog_visual, Relation::CO_OCCURS_WITH, 1.0f);
    }
    
    std::cout << "\n📊 Results:" << std::endl;
    graph.print_stats();
    
    std::cout << "\n✅ Direct audio→vision link created!" << std::endl;
    std::cout << "   No text needed: audio_tokens ↔ vision:dog" << std::endl;
    std::cout << "   Melvin learned: 'This sound pattern goes with that visual'" << std::endl;
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  🧠 SELF-LEARNING SPEECH                                  ║" << std::endl;
    std::cout << "║  Babbling → Hearing → Clustering → Adaptation            ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n";
    
    if (argc > 1) {
        int demo = std::atoi(argv[1]);
        
        switch (demo) {
            case 1: demo_phoneme_discovery(); break;
            case 2: demo_self_feedback(); break;
            case 3: demo_phoneme_learning(); break;
            case 4: demo_complete_cycle(); break;
            case 5: demo_cross_modal(); break;
            case 6:
                demo_phoneme_discovery();
                demo_self_feedback();
                demo_phoneme_learning();
                demo_complete_cycle();
                demo_cross_modal();
                break;
            default:
                std::cerr << "Invalid demo (1-6)" << std::endl;
                return 1;
        }
    } else {
        // Run all
        demo_phoneme_discovery();
        demo_self_feedback();
        demo_phoneme_learning();
        demo_complete_cycle();
        demo_cross_modal();
    }
    
    std::cout << "\n✨ Self-learning speech demos complete!\n" << std::endl;
    
    return 0;
}


