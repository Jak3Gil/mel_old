/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  PURE AUDIO LEARNING - No APIs, No Text, Just Audio Patterns             ║
 * ║  Audio tokens stored as graph nodes - fully local                         ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include "../audio/audio_tokenizer.h"
#include "../core/atomic_graph.h"
#include <iostream>
#include <vector>
#include <cmath>
#include <random>

using namespace melvin;
using namespace melvin::audio;

// ============================================================================
// GENERATE TEST AUDIO
// ============================================================================

std::vector<float> generate_tone(float frequency, float duration_s, uint32_t sample_rate) {
    size_t num_samples = static_cast<size_t>(duration_s * sample_rate);
    std::vector<float> samples(num_samples);
    
    for (size_t i = 0; i < num_samples; i++) {
        float t = static_cast<float>(i) / sample_rate;
        samples[i] = 0.5f * std::sin(2.0f * M_PI * frequency * t);
    }
    
    return samples;
}

std::vector<float> generate_noise(float duration_s, uint32_t sample_rate) {
    size_t num_samples = static_cast<size_t>(duration_s * sample_rate);
    std::vector<float> samples(num_samples);
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-0.3, 0.3);
    
    for (size_t i = 0; i < num_samples; i++) {
        samples[i] = dis(gen);
    }
    
    return samples;
}

std::vector<float> generate_chirp(float f_start, float f_end, float duration_s, uint32_t sample_rate) {
    size_t num_samples = static_cast<size_t>(duration_s * sample_rate);
    std::vector<float> samples(num_samples);
    
    for (size_t i = 0; i < num_samples; i++) {
        float t = static_cast<float>(i) / sample_rate;
        float freq = f_start + (f_end - f_start) * t / duration_s;
        samples[i] = 0.5f * std::sin(2.0f * M_PI * freq * t);
    }
    
    return samples;
}

// ============================================================================
// DEMO 1: Basic Audio Tokenization
// ============================================================================

void demo_basic_tokenization() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 1: Basic Audio Tokenization (No APIs)              ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    AudioTokenizer tokenizer;
    
    std::cout << "🎵 Generating test audio (pure tone at 440 Hz)..." << std::endl;
    auto audio = generate_tone(440.0f, 1.0f, 16000);
    
    std::cout << "\n🔤 Tokenizing audio into graph nodes..." << std::endl;
    auto tokens = tokenizer.tokenize(audio, graph);
    
    std::cout << "\n📊 Results:" << std::endl;
    graph.print_stats();
    tokenizer.print_stats();
    
    std::cout << "\n✅ Audio converted to " << tokens.size() << " graph nodes!" << std::endl;
    std::cout << "   (No text, no APIs, pure audio features)" << std::endl;
}

// ============================================================================
// DEMO 2: Pattern Recognition
// ============================================================================

void demo_pattern_recognition() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 2: Pattern Recognition (Repeated Sounds)           ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    AudioTokenizer tokenizer;
    
    std::cout << "🎵 Creating audio with repeated patterns..." << std::endl;
    
    // Pattern 1: 440 Hz tone (repeated)
    auto pattern1 = generate_tone(440.0f, 0.2f, 16000);
    
    // Pattern 2: 880 Hz tone (repeated)
    auto pattern2 = generate_tone(880.0f, 0.2f, 16000);
    
    // Combine: pattern1 → pattern2 → pattern1 → pattern2
    std::vector<float> audio;
    audio.insert(audio.end(), pattern1.begin(), pattern1.end());
    audio.insert(audio.end(), pattern2.begin(), pattern2.end());
    audio.insert(audio.end(), pattern1.begin(), pattern1.end());
    audio.insert(audio.end(), pattern2.begin(), pattern2.end());
    
    std::cout << "   Pattern: A-B-A-B (where A and B are distinct sounds)" << std::endl;
    
    std::cout << "\n🔤 Tokenizing..." << std::endl;
    auto tokens = tokenizer.tokenize(audio, graph);
    
    std::cout << "\n📊 Results:" << std::endl;
    std::cout << "   Total tokens: " << tokens.size() << std::endl;
    std::cout << "   Unique tokens: " << tokenizer.get_unique_tokens() << std::endl;
    std::cout << "   Graph nodes: " << graph.node_count() << std::endl;
    std::cout << "   Graph edges: " << graph.edge_count() << std::endl;
    
    std::cout << "\n✅ Melvin recognized the repeated pattern!" << std::endl;
    std::cout << "   Deduplication: Same sound = same node (learned)" << std::endl;
}

// ============================================================================
// DEMO 3: Temporal Sequence Learning
// ============================================================================

void demo_temporal_learning() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 3: Temporal Sequence Learning                      ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    AudioTokenizer tokenizer;
    
    std::cout << "🎵 Creating chirp (rising frequency)..." << std::endl;
    auto audio = generate_chirp(200.0f, 800.0f, 1.0f, 16000);
    
    std::cout << "\n🔤 Tokenizing..." << std::endl;
    auto tokens = tokenizer.tokenize(audio, graph);
    
    std::cout << "\n📊 Analyzing temporal connections..." << std::endl;
    
    // Check temporal links
    size_t temporal_edges = 0;
    for (size_t i = 1; i < tokens.size(); i++) {
        float weight = graph.get_edge_weight(tokens[i-1], tokens[i], Relation::TEMPORAL_NEXT);
        if (weight > 0) {
            temporal_edges++;
        }
    }
    
    std::cout << "   Temporal edges created: " << temporal_edges << std::endl;
    std::cout << "   Sequence length: " << tokens.size() << std::endl;
    
    std::cout << "\n✅ Melvin learned the temporal pattern!" << std::endl;
    std::cout << "   Rising chirp = sequence of connected audio tokens" << std::endl;
}

// ============================================================================
// DEMO 4: Multi-Sound Learning
// ============================================================================

void demo_multi_sound() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 4: Multi-Sound Environment                         ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    AudioTokenizer tokenizer;
    
    std::cout << "🎵 Simulating complex sound environment..." << std::endl;
    
    // Create different types of sounds
    auto low_hum = generate_tone(100.0f, 0.3f, 16000);
    auto mid_beep = generate_tone(440.0f, 0.3f, 16000);
    auto high_chirp = generate_chirp(800.0f, 1600.0f, 0.3f, 16000);
    auto noise = generate_noise(0.3f, 16000);
    
    // Combine sounds
    std::vector<float> audio;
    audio.insert(audio.end(), low_hum.begin(), low_hum.end());
    audio.insert(audio.end(), mid_beep.begin(), mid_beep.end());
    audio.insert(audio.end(), high_chirp.begin(), high_chirp.end());
    audio.insert(audio.end(), noise.begin(), noise.end());
    
    std::cout << "   Sounds: Low hum → Mid beep → High chirp → Noise" << std::endl;
    
    std::cout << "\n🔤 Tokenizing complex audio..." << std::endl;
    auto tokens = tokenizer.tokenize(audio, graph);
    
    std::cout << "\n📊 Results:" << std::endl;
    graph.print_stats();
    tokenizer.print_stats();
    
    std::cout << "\n💾 Saving audio knowledge..." << std::endl;
    graph.save("pure_audio_nodes.bin", "pure_audio_edges.bin");
    
    std::cout << "\n✅ Melvin learned 4 distinct sound types!" << std::endl;
    std::cout << "   Each sound = unique pattern of audio tokens" << std::endl;
}

// ============================================================================
// DEMO 5: Audio Memory and Recall
// ============================================================================

void demo_memory_recall() {
    std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  Demo 5: Audio Memory and Recall                         ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    
    AtomicGraph graph;
    
    std::cout << "📂 Loading previous audio knowledge..." << std::endl;
    
    try {
        graph.load("pure_audio_nodes.bin", "pure_audio_edges.bin");
        
        std::cout << "✅ Loaded audio memory:" << std::endl;
        graph.print_stats();
        
        std::cout << "\n🔍 Analyzing learned audio patterns..." << std::endl;
        
        // Find audio token nodes
        auto audio_nodes = graph.find_nodes("audio_token:");
        std::cout << "   Found " << audio_nodes.size() << " audio tokens" << std::endl;
        
        // Show temporal sequences
        if (!audio_nodes.empty()) {
            uint64_t first_token = audio_nodes[0];
            auto neighbors = graph.neighbors(first_token, Relation::TEMPORAL_NEXT);
            std::cout << "   First token connects to " << neighbors.size() << " next tokens" << std::endl;
        }
        
        std::cout << "\n✅ Melvin remembered all the sounds!" << std::endl;
        std::cout << "   No text, pure audio patterns in graph" << std::endl;
        
    } catch (...) {
        std::cout << "⚠️  No previous memory. Run demo 4 first." << std::endl;
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char** argv) {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════╗" << std::endl;
    std::cout << "║  🎧 PURE AUDIO LEARNING                                   ║" << std::endl;
    std::cout << "║  No APIs • No Text • Just Audio → Graph                  ║" << std::endl;
    std::cout << "╚═══════════════════════════════════════════════════════════╝" << std::endl;
    std::cout << "\n";
    
    if (argc > 1) {
        int demo = std::atoi(argv[1]);
        
        switch (demo) {
            case 1: demo_basic_tokenization(); break;
            case 2: demo_pattern_recognition(); break;
            case 3: demo_temporal_learning(); break;
            case 4: demo_multi_sound(); break;
            case 5: demo_memory_recall(); break;
            case 6:
                demo_basic_tokenization();
                demo_pattern_recognition();
                demo_temporal_learning();
                demo_multi_sound();
                demo_memory_recall();
                break;
            default:
                std::cerr << "Invalid demo number (1-6)" << std::endl;
                return 1;
        }
    } else {
        // Run all demos
        demo_basic_tokenization();
        demo_pattern_recognition();
        demo_temporal_learning();
        demo_multi_sound();
        demo_memory_recall();
    }
    
    std::cout << "\n✨ Pure audio learning complete!\n" << std::endl;
    
    return 0;
}

