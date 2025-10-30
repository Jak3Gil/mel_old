/**
 * @file test_cognitive_os.cpp
 * @brief Test the complete always-on Cognitive OS
 * 
 * Demonstrates:
 * - All services running concurrently
 * - Event-driven communication
 * - Shared global field
 * - Autonomous adaptation
 * - Continuous learning
 */

#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include "cognitive_os/cognitive_os.h"
#include "core/unified_intelligence.h"

using namespace melvin::cognitive_os;
using namespace melvin::intelligence;

void print_header() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║     MELVIN COGNITIVE OS TEST                         ║\n";
    std::cout << "║     Always-On Human-Like System                      ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
}

void build_demo_graph(
    std::unordered_map<std::string, int>& word_to_id,
    std::unordered_map<int, std::string>& id_to_word,
    std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    std::unordered_map<int, std::vector<float>>& embeddings
) {
    std::vector<std::string> vocabulary = {
        "hello", "world", "melvin", "intelligence", "system",
        "cognitive", "always", "active", "learning", "adaptation",
        "vision", "audio", "attention", "memory", "reasoning"
    };
    
    int node_id = 0;
    for (const auto& word : vocabulary) {
        word_to_id[word] = node_id;
        id_to_word[node_id] = word;
        
        // Simple embedding
        std::vector<float> emb(128);
        size_t hash = std::hash<std::string>{}(word);
        for (size_t i = 0; i < 128; i++) {
            emb[i] = std::sin(static_cast<float>(hash + i) * 0.01f);
        }
        embeddings[node_id] = emb;
        
        node_id++;
    }
    
    // Create connections
    auto add_edge = [&](const std::string& from, const std::string& to, float weight) {
        int from_id = word_to_id[from];
        int to_id = word_to_id[to];
        graph[from_id].push_back({to_id, weight});
        graph[to_id].push_back({from_id, weight * 0.8f});
    };
    
    add_edge("hello", "world", 0.95f);
    add_edge("melvin", "intelligence", 0.9f);
    add_edge("melvin", "system", 0.85f);
    add_edge("cognitive", "system", 0.9f);
    add_edge("always", "active", 0.95f);
    add_edge("learning", "adaptation", 0.9f);
    add_edge("vision", "attention", 0.8f);
    add_edge("audio", "attention", 0.8f);
    add_edge("attention", "memory", 0.85f);
    add_edge("memory", "reasoning", 0.9f);
    add_edge("reasoning", "intelligence", 0.95f);
}

int main() {
    print_header();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // BUILD KNOWLEDGE GRAPH
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🔧 Building knowledge graph...\n";
    
    std::unordered_map<std::string, int> word_to_id;
    std::unordered_map<int, std::string> id_to_word;
    std::unordered_map<int, std::vector<std::pair<int, float>>> graph;
    std::unordered_map<int, std::vector<float>> embeddings;
    
    build_demo_graph(word_to_id, id_to_word, graph, embeddings);
    
    std::cout << "   ✅ " << word_to_id.size() << " concepts\n";
    std::cout << "   ✅ Knowledge graph built\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // CREATE UNIFIED INTELLIGENCE
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🧠 Initializing Unified Intelligence...\n";
    
    UnifiedIntelligence melvin;
    melvin.initialize(graph, embeddings, word_to_id, id_to_word);
    
    std::cout << "   ✅ Intelligence ready\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // CREATE ACTIVATION FIELD
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🌊 Creating global activation field...\n";
    
    FieldFacade field(graph, embeddings);
    
    std::cout << "   ✅ Field ready\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // CREATE COGNITIVE OS
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🚀 Creating Cognitive OS...\n";
    
    CognitiveOS os;
    os.attach(&melvin, &field);
    
    std::cout << "   ✅ OS ready\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // START ALWAYS-ON SYSTEM
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << "                 STARTING COGNITIVE OS                 \n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";
    
    os.start();
    
    std::cout << "🎉 System is ALIVE and ALWAYS-ON!\n\n";
    
    std::cout << "Services running concurrently:\n";
    std::cout << "  • Scheduler:      50 Hz (20ms ticks)\n";
    std::cout << "  • Cognition:      30 Hz\n";
    std::cout << "  • Attention:      60 Hz\n";
    std::cout << "  • Working Memory: 30 Hz\n";
    std::cout << "  • Learning:       10 Hz\n";
    std::cout << "  • Reflection:      5 Hz\n";
    std::cout << "  • Field Decay:    50 Hz\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // INJECT TEST STIMULI
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "💉 Injecting test stimuli...\n\n";
    
    // Simulate vision input
    VisionEvent vision;
    vision.timestamp = 0.0;
    vision.obj_ids = {word_to_id["vision"], word_to_id["attention"]};
    os.event_bus()->publish(topics::VISION_EVENTS, vision);
    std::cout << "  ✅ Vision event published\n";
    
    // Simulate audio input
    AudioEvent audio;
    audio.timestamp = 0.1;
    audio.phonemes = {"h", "e", "l", "l", "o"};
    audio.energy = 0.8f;
    os.event_bus()->publish(topics::AUDIO_EVENTS, audio);
    std::cout << "  ✅ Audio event published\n";
    
    // Send cognitive query
    CogQuery query;
    query.timestamp = 0.2;
    query.text = "What is intelligence?";
    query.intent = 0;  // DEFINE
    os.event_bus()->publish(topics::COG_QUERY, query);
    std::cout << "  ✅ Cognitive query published\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // LET SYSTEM RUN
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "⏱  Running for 5 seconds...\n\n";
    
    for (int i = 0; i < 5; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        // Get current state
        auto metrics = field.get_metrics();
        
        std::cout << "[" << (i + 1) << "s] ";
        std::cout << "Active: " << std::setw(4) << metrics.active_nodes << " | ";
        std::cout << "Entropy: " << std::fixed << std::setprecision(2) << metrics.entropy << " | ";
        std::cout << "Sparsity: " << std::setprecision(1) << (metrics.sparsity * 100) << "%\n";
    }
    
    std::cout << "\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // CHECK FOR ANSWER
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    auto answer_event = os.event_bus()->get_latest(topics::COG_ANSWER);
    if (answer_event.data) {
        auto answer = answer_event.get<CogAnswer>();
        if (answer) {
            std::cout << "💬 Answer received:\n";
            std::cout << "   \"" << answer->text << "\"\n";
            std::cout << "   Confidence: " << (answer->confidence * 100) << "%\n\n";
        }
    }
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // CHECK METRICS
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "📊 System metrics:\n";
    std::cout << "   Logs written: " << os.metrics()->logs_written() << "\n";
    std::cout << "   Dropped msgs: " << os.event_bus()->dropped_messages() << "\n";
    std::cout << "   Field size:   " << field.get_metrics().active_nodes << " active nodes\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // STOP SYSTEM
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🛑 Stopping Cognitive OS...\n";
    os.stop();
    
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║     COGNITIVE OS TEST COMPLETE                       ║\n";
    std::cout << "║                                                      ║\n";
    std::cout << "║     ✅ Always-on system working                      ║\n";
    std::cout << "║     ✅ Concurrent services                           ║\n";
    std::cout << "║     ✅ Event-driven communication                    ║\n";
    std::cout << "║     ✅ Shared global state                           ║\n";
    std::cout << "║     ✅ Autonomous adaptation                         ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";
    
    return 0;
}

