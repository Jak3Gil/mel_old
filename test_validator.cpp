/**
 * @file test_validator.cpp
 * @brief Test the Cognitive OS validator
 * 
 * Runs full validation suite and generates readiness report
 */

#include <iostream>
#include <iomanip>
#include "validator/validator.h"
#include "cognitive_os/cognitive_os.h"
#include "core/unified_intelligence.h"

using namespace melvin::validator;
using namespace melvin::cognitive_os;
using namespace melvin::intelligence;

void print_header() {
    std::cout << "╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║     MELVIN COGNITIVE OS VALIDATOR                    ║\n";
    std::cout << "║     Go-Live Readiness Suite                          ║\n";
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
        "vision", "audio", "attention", "memory", "reasoning",
        "knowledge", "concept", "query", "answer", "confidence"
    };
    
    int node_id = 0;
    for (const auto& word : vocabulary) {
        word_to_id[word] = node_id;
        id_to_word[node_id] = word;
        
        std::vector<float> emb(128);
        size_t hash = std::hash<std::string>{}(word);
        for (size_t i = 0; i < 128; i++) {
            emb[i] = std::sin(static_cast<float>(hash + i) * 0.01f);
        }
        embeddings[node_id] = emb;
        
        node_id++;
    }
    
    auto add_edge = [&](const std::string& from, const std::string& to, float weight) {
        int from_id = word_to_id[from];
        int to_id = word_to_id[to];
        graph[from_id].push_back({to_id, weight});
        graph[to_id].push_back({from_id, weight * 0.8f});
    };
    
    add_edge("hello", "world", 0.95f);
    add_edge("melvin", "intelligence", 0.9f);
    add_edge("intelligence", "learning", 0.85f);
    add_edge("learning", "adaptation", 0.9f);
    add_edge("vision", "attention", 0.8f);
    add_edge("audio", "attention", 0.8f);
    add_edge("attention", "memory", 0.85f);
    add_edge("memory", "reasoning", 0.9f);
    add_edge("reasoning", "intelligence", 0.95f);
    add_edge("query", "answer", 0.9f);
    add_edge("answer", "confidence", 0.85f);
}

int main(int argc, char** argv) {
    print_header();
    
    // Parse args
    int duration = 10;  // Default 10 seconds for quick test
    std::string report_path = "readiness_report.md";
    
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--duration" && i + 1 < argc) {
            duration = std::stoi(argv[++i]);
        } else if (arg == "--report" && i + 1 < argc) {
            report_path = argv[++i];
        }
    }
    
    std::cout << "⚙️  Configuration:\n";
    std::cout << "   Duration: " << duration << "s\n";
    std::cout << "   Report: " << report_path << "\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // BUILD SYSTEM
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🔧 Building test system...\n";
    
    std::unordered_map<std::string, int> word_to_id;
    std::unordered_map<int, std::string> id_to_word;
    std::unordered_map<int, std::vector<std::pair<int, float>>> graph;
    std::unordered_map<int, std::vector<float>> embeddings;
    
    build_demo_graph(word_to_id, id_to_word, graph, embeddings);
    
    UnifiedIntelligence melvin;
    melvin.initialize(graph, embeddings, word_to_id, id_to_word);
    
    FieldFacade field(graph, embeddings);
    
    CognitiveOS os;
    os.attach(&melvin, &field);
    
    std::cout << "   ✅ System ready\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // START COGNITIVE OS
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🚀 Starting Cognitive OS...\n";
    os.start();
    std::cout << "   ✅ OS running\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // START VALIDATOR
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🔍 Starting Validator...\n";
    Validator validator(&os);
    validator.start(duration);
    
    // Wait for validation to complete
    std::cout << "\n⏱  Running validation for " << duration << " seconds...\n\n";
    
    for (int i = 0; i < duration; i++) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "  [" << (i + 1) << "/" << duration << "s] Collecting metrics...\n";
    }
    
    validator.stop();
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // GENERATE REPORT
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "\n📊 Generating readiness report...\n";
    validator.generate_report(report_path);
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // PRINT SUMMARY
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    auto results = validator.run_validation_suite();
    
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║     VALIDATION SUMMARY                               ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << (results.timing_ok ? "✅" : "❌") << " Timing ......... ";
    std::cout << (results.timing_ok ? "PASS" : "FAIL");
    std::cout << " (jitter: " << std::fixed << std::setprecision(2) << results.avg_jitter << "ms)\n";
    
    std::cout << (results.scheduler_fair ? "✅" : "❌") << " Scheduler ...... ";
    std::cout << (results.scheduler_fair ? "PASS" : "FAIL") << "\n";
    
    std::cout << (results.field_healthy ? "✅" : "❌") << " Field Health ... ";
    std::cout << (results.field_healthy ? "PASS" : "FAIL");
    std::cout << " (coherence: +" << (results.coherence_gain * 100) << "%)\n";
    
    std::cout << (results.reasoning_ok ? "✅" : "❌") << " Reasoning ...... ";
    std::cout << (results.reasoning_ok ? "PASS" : "FAIL");
    std::cout << " (accuracy: " << (results.accuracy * 100) << "%)\n";
    
    std::cout << (results.stress_passed ? "✅" : "❌") << " Stress Tests ... ";
    std::cout << (results.stress_passed ? "PASS" : "FAIL") << "\n";
    
    std::cout << (results.safety_ok ? "✅" : "❌") << " Safety ......... ";
    std::cout << (results.safety_ok ? "PASS" : "FAIL") << "\n";
    
    std::cout << (results.memory_ok ? "✅" : "❌") << " Memory ......... ";
    std::cout << (results.memory_ok ? "PASS" : "FAIL");
    std::cout << " (growth: " << results.memory_growth_pct << "%)\n";
    
    std::cout << "\n";
    std::cout << "═══════════════════════════════════════════════════════\n";
    std::cout << " OVERALL: " << (results.ready_for_deployment ? "✅ READY" : "❌ NOT READY") << "\n";
    std::cout << "═══════════════════════════════════════════════════════\n\n";
    
    if (!results.warnings.empty()) {
        std::cout << "⚠️  Warnings:\n";
        for (const auto& w : results.warnings) {
            std::cout << "   - " << w << "\n";
        }
        std::cout << "\n";
    }
    
    if (!results.recommendations.empty()) {
        std::cout << "💡 Recommendations:\n";
        for (const auto& r : results.recommendations) {
            std::cout << "   - " << r << "\n";
        }
        std::cout << "\n";
    }
    
    std::cout << "📄 Full report: " << report_path << "\n\n";
    
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    // CLEANUP
    // ━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
    
    std::cout << "🛑 Stopping system...\n";
    os.stop();
    
    std::cout << "\n╔══════════════════════════════════════════════════════╗\n";
    std::cout << "║     VALIDATION COMPLETE                              ║\n";
    std::cout << "╚══════════════════════════════════════════════════════╝\n";
    
    return results.ready_for_deployment ? 0 : 1;
}

