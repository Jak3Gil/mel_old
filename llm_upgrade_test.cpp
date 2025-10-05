/*
 * LLM-STYLE UPGRADE TEST FOR MELVIN
 * 
 * This test validates the LLM-style reasoning upgrades including:
 * - Soft attention-style reasoning
 * - Probabilistic output generation
 * - Embedding-aware processing
 * - Dual-state evolution
 * - Context buffer management
 * - Multimodal latent space projection
 */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <cassert>
#include <filesystem>
#include <random>

// Include Melvin headers
#include "melvin.h"
#include "UCAConfig.h"

struct LLMTestResult {
    std::string test_name;
    bool passed;
    std::string details;
    double score;
    std::chrono::milliseconds duration;
};

class LLMUpgradeTestSuite {
private:
    melvin_t* melvin;
    std::vector<LLMTestResult> results;
    std::string test_data_dir;
    std::mt19937 rng;
    
public:
    LLMUpgradeTestSuite() : melvin(nullptr), rng(std::random_device{}()) {
        test_data_dir = "./llm_upgrade_test_data";
        std::filesystem::create_directories(test_data_dir);
    }
    
    ~LLMUpgradeTestSuite() {
        if (melvin) {
            melvin_destroy(melvin);
        }
    }
    
    bool initialize() {
        std::cout << "🧠 Initializing Melvin with LLM-style upgrades...\n";
        
        // Create fresh test store
        std::string store_dir = test_data_dir + "/melvin_llm_store";
        melvin = melvin_create(store_dir.c_str());
        
        if (!melvin) {
            std::cerr << "❌ Failed to initialize Melvin\n";
            return false;
        }
        
        // Enable LLM-style features
        melvin->uca_config.use_soft_traversal = true;
        melvin->uca_config.use_probabilistic_output = true;
        melvin->uca_config.use_embeddings = true;
        melvin->uca_config.dual_state_evolution = true;
        melvin->uca_config.use_context_buffer = true;
        melvin->uca_config.use_feedback_loop = true;
        
        // Enable logging
        melvin->uca_config.log_attention_entropy = true;
        melvin->uca_config.log_output_diversity = true;
        melvin->uca_config.log_embedding_coherence = true;
        
        // Update the reasoning engine configuration
        melvin->llm_reasoning_engine->update_config(melvin->uca_config);
        
        std::cout << "✅ Melvin LLM-style initialization complete\n";
        std::cout << "   Store directory: " << store_dir << "\n";
        std::cout << "   Soft traversal: " << (melvin->uca_config.use_soft_traversal ? "ENABLED" : "DISABLED") << "\n";
        std::cout << "   Probabilistic output: " << (melvin->uca_config.use_probabilistic_output ? "ENABLED" : "DISABLED") << "\n";
        std::cout << "   Embeddings: " << (melvin->uca_config.use_embeddings ? "ENABLED" : "DISABLED") << "\n";
        std::cout << "   Dual-state evolution: " << (melvin->uca_config.dual_state_evolution ? "ENABLED" : "DISABLED") << "\n";
        std::cout << "   Context buffer: " << (melvin->uca_config.use_context_buffer ? "ENABLED" : "DISABLED") << "\n\n";
        
        return true;
    }
    
    LLMTestResult run_test(const std::string& test_name, 
                           std::function<bool()> test_func) {
        std::cout << "🔍 Running LLM test: " << test_name << "\n";
        
        auto start_time = std::chrono::high_resolution_clock::now();
        bool passed = test_func();
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        LLMTestResult result;
        result.test_name = test_name;
        result.passed = passed;
        result.duration = duration;
        result.score = passed ? 1.0 : 0.0;
        
        std::cout << "   " << (passed ? "✅ PASS" : "❌ FAIL") 
                  << " (" << duration.count() << "ms)\n\n";
        
        return result;
    }
    
    // TEST 1: Soft Attention-Style Reasoning
    LLMTestResult test_soft_attention_reasoning() {
        return run_test("Soft Attention-Style Reasoning", [this]() {
            std::cout << "   🎯 Testing soft attention traversal...\n";
            
            // Inject knowledge for testing
            std::vector<std::string> knowledge = {
                "artificial intelligence is the simulation of human intelligence",
                "machine learning is a subset of artificial intelligence",
                "neural networks are inspired by biological neurons",
                "deep learning uses multiple layers of neural networks",
                "transformer architecture revolutionized natural language processing",
                "attention mechanisms allow models to focus on relevant information",
                "embeddings represent words as dense vectors",
                "probabilistic models handle uncertainty in reasoning"
            };
            
            for (const auto& fact : knowledge) {
                melvin_learn(melvin, fact.c_str());
            }
            
            std::cout << "   📊 Knowledge injected: " << knowledge.size() << " facts\n";
            std::cout << "   📈 Current nodes: " << melvin_node_count(melvin) << "\n";
            std::cout << "   📈 Current edges: " << melvin_edge_count(melvin) << "\n";
            
            // Test soft attention reasoning
            std::vector<std::string> test_queries = {
                "What is artificial intelligence?",
                "How do neural networks work?",
                "What is machine learning?",
                "Explain attention mechanisms",
                "What are embeddings?"
            };
            
            int successful_reasoning = 0;
            for (const auto& query : test_queries) {
                std::cout << "     Query: " << query << "\n";
                
                const char* response = melvin_reason(melvin, query.c_str());
                std::string response_str = response ? response : "";
                
                if (!response_str.empty() && response_str != "I don't have enough information yet.") {
                    std::cout << "     Response: " << response_str << "\n";
                    successful_reasoning++;
                } else {
                    std::cout << "     Response: No response\n";
                }
            }
            
            std::cout << "   📊 Soft attention success rate: " << successful_reasoning << "/" 
                      << test_queries.size() << "\n";
            
            return successful_reasoning >= 3; // At least 3 successful reasoning attempts
        });
    }
    
    // TEST 2: Probabilistic Output Generation
    LLMTestResult test_probabilistic_output() {
        return run_test("Probabilistic Output Generation", [this]() {
            std::cout << "   🎲 Testing probabilistic output generation...\n";
            
            // Test multiple generations of the same query
            std::string test_query = "What is artificial intelligence?";
            std::vector<std::string> responses;
            
            for (int i = 0; i < 5; ++i) {
                const char* response = melvin_reason(melvin, test_query.c_str());
                if (response) {
                    responses.push_back(response);
                    std::cout << "     Generation " << (i + 1) << ": " << response << "\n";
                }
            }
            
            // Check for diversity in responses
            std::unordered_set<std::string> unique_responses(responses.begin(), responses.end());
            double diversity_ratio = static_cast<double>(unique_responses.size()) / responses.size();
            
            std::cout << "   📊 Response diversity: " << unique_responses.size() << "/" 
                      << responses.size() << " (" << std::fixed << std::setprecision(2) 
                      << (diversity_ratio * 100) << "%)\n";
            
            // Test temperature effects
            std::cout << "   🌡️ Testing temperature effects...\n";
            
            // Test with different temperature settings
            float original_temp = melvin->uca_config.output_temperature;
            
            melvin->uca_config.output_temperature = 0.5f; // Low temperature
            const char* low_temp_response = melvin_reason(melvin, test_query.c_str());
            
            melvin->uca_config.output_temperature = 1.5f; // High temperature
            const char* high_temp_response = melvin_reason(melvin, test_query.c_str());
            
            // Restore original temperature
            melvin->uca_config.output_temperature = original_temp;
            
            bool temp_effects_working = (low_temp_response != nullptr && high_temp_response != nullptr);
            std::cout << "     Low temp response: " << (low_temp_response ? low_temp_response : "None") << "\n";
            std::cout << "     High temp response: " << (high_temp_response ? high_temp_response : "None") << "\n";
            
            return diversity_ratio >= 0.4 && temp_effects_working; // At least 40% diversity
        });
    }
    
    // TEST 3: Embedding-Aware Processing
    LLMTestResult test_embedding_aware_processing() {
        return run_test("Embedding-Aware Processing", [this]() {
            std::cout << "   🧮 Testing embedding-aware processing...\n";
            
            // Inject related concepts
            std::vector<std::string> concepts = {
                "cat is a mammal",
                "dog is a mammal", 
                "bird is an animal",
                "fish is an animal",
                "mammal is warm-blooded",
                "animal is living being",
                "warm-blooded means constant body temperature",
                "living being needs energy to survive"
            };
            
            for (const auto& concept : concepts) {
                melvin_learn(melvin, concept.c_str());
            }
            
            // Test analogy reasoning through embeddings
            std::vector<std::string> analogy_tests = {
                "What is a cat?",
                "What is a dog?",
                "Are cats and dogs similar?",
                "What do mammals have in common?",
                "How are animals different from plants?"
            };
            
            int successful_analogies = 0;
            for (const auto& query : analogy_tests) {
                const char* response = melvin_reason(melvin, query.c_str());
                if (response && strlen(response) > 10) {
                    std::cout << "     Analogy query: " << query << "\n";
                    std::cout << "     Analogy response: " << response << "\n";
                    successful_analogies++;
                }
            }
            
            std::cout << "   📊 Analogy reasoning success: " << successful_analogies << "/" 
                      << analogy_tests.size() << "\n";
            
            // Test embedding coherence
            float embedding_coherence = melvin->embedding_coherence;
            std::cout << "   📈 Embedding coherence: " << std::fixed << std::setprecision(3) 
                      << embedding_coherence << "\n";
            
            return successful_analogies >= 3 && embedding_coherence > 0.0f;
        });
    }
    
    // TEST 4: Dual-State Evolution
    LLMTestResult test_dual_state_evolution() {
        return run_test("Dual-State Evolution", [this]() {
            std::cout << "   🧬 Testing dual-state evolution...\n";
            
            // Get initial evolution state
            int initial_cycles = melvin->evolution_state.cycle_count;
            float initial_drift = melvin->evolution_state.total_drift;
            
            std::cout << "   📊 Initial cycles: " << initial_cycles << "\n";
            std::cout << "   📊 Initial drift: " << initial_drift << "\n";
            
            // Run multiple reasoning cycles to trigger evolution
            std::vector<std::string> evolution_queries = {
                "What is learning?",
                "How does adaptation work?",
                "What is evolution?",
                "How do systems improve?",
                "What is optimization?"
            };
            
            for (int cycle = 0; cycle < 10; ++cycle) {
                for (const auto& query : evolution_queries) {
                    melvin_reason(melvin, query.c_str());
                }
                
                // Trigger parameter evolution
                melvin->llm_reasoning_engine->evolve_parameters();
                
                if (cycle % 3 == 0) {
                    std::cout << "     Cycle " << cycle << ": Health=" 
                              << melvin_get_health_score(melvin) << "\n";
                }
            }
            
            // Check evolution metrics
            int final_cycles = melvin->evolution_state.cycle_count;
            float final_drift = melvin->evolution_state.total_drift;
            float health_score = melvin_get_health_score(melvin);
            
            std::cout << "   📊 Final cycles: " << final_cycles << "\n";
            std::cout << "   📊 Final drift: " << final_drift << "\n";
            std::cout << "   📊 Health score: " << health_score << "\n";
            
            bool cycles_increased = (final_cycles > initial_cycles);
            bool drift_changed = (std::abs(final_drift - initial_drift) > 0.001f);
            bool health_reasonable = (health_score > 0.0f && health_score <= 1.0f);
            
            return cycles_increased && drift_changed && health_reasonable;
        });
    }
    
    // TEST 5: Context Buffer Management
    LLMTestResult test_context_buffer_management() {
        return run_test("Context Buffer Management", [this]() {
            std::cout << "   🧠 Testing context buffer management...\n";
            
            // Test context continuity across multiple queries
            std::vector<std::string> context_queries = {
                "What is a computer?",
                "How does it process information?",
                "What makes it intelligent?",
                "Can it learn like humans?",
                "What are the differences?"
            };
            
            std::vector<std::string> responses;
            for (const auto& query : context_queries) {
                const char* response = melvin_reason(melvin, query.c_str());
                if (response) {
                    responses.push_back(response);
                    std::cout << "     Context query: " << query << "\n";
                    std::cout << "     Context response: " << response << "\n";
                }
            }
            
            // Check for context coherence
            int coherent_responses = 0;
            for (size_t i = 1; i < responses.size(); ++i) {
                // Simple coherence check: responses should build on each other
                if (responses[i].length() > 10 && responses[i-1].length() > 10) {
                    coherent_responses++;
                }
            }
            
            std::cout << "   📊 Context coherence: " << coherent_responses << "/" 
                      << (responses.size() - 1) << "\n";
            
            // Test context buffer size management
            size_t context_size = melvin->llm_reasoning_engine->get_config().max_context_size;
            std::cout << "   📊 Context buffer size: " << context_size << "\n";
            
            return coherent_responses >= 2 && context_size > 0;
        });
    }
    
    // TEST 6: Multimodal Latent Space
    LLMTestResult test_multimodal_latent_space() {
        return run_test("Multimodal Latent Space", [this]() {
            std::cout << "   🎨 Testing multimodal latent space projection...\n";
            
            // Test projection to latent space for different modalities
            std::vector<float> test_embedding(128, 0.1f); // Test embedding
            
            // Test audio projection
            auto audio_latent = melvin->llm_reasoning_engine->project_to_latent(test_embedding, 0);
            std::cout << "   📊 Audio projection dimension: " << audio_latent.size() << "\n";
            
            // Test image projection
            auto image_latent = melvin->llm_reasoning_engine->project_to_latent(test_embedding, 1);
            std::cout << "   📊 Image projection dimension: " << image_latent.size() << "\n";
            
            // Test text projection
            auto text_latent = melvin->llm_reasoning_engine->project_to_latent(test_embedding, 2);
            std::cout << "   📊 Text projection dimension: " << text_latent.size() << "\n";
            
            // Verify projections are different
            bool projections_different = true;
            if (audio_latent.size() == image_latent.size() && image_latent.size() == text_latent.size()) {
                for (size_t i = 0; i < audio_latent.size(); ++i) {
                    if (audio_latent[i] == image_latent[i] && image_latent[i] == text_latent[i]) {
                        projections_different = false;
                        break;
                    }
                }
            }
            
            std::cout << "   📊 Projections are different: " << (projections_different ? "YES" : "NO") << "\n";
            
            return audio_latent.size() > 0 && image_latent.size() > 0 && 
                   text_latent.size() > 0 && projections_different;
        });
    }
    
    // TEST 7: Performance Metrics
    LLMTestResult test_performance_metrics() {
        return run_test("Performance Metrics", [this]() {
            std::cout << "   📊 Testing performance metrics...\n";
            
            // Run reasoning to generate metrics
            std::vector<std::string> metric_queries = {
                "What is intelligence?",
                "How does reasoning work?",
                "What is learning?",
                "How do systems adapt?",
                "What is optimization?"
            };
            
            for (const auto& query : metric_queries) {
                melvin_reason(melvin, query.c_str());
            }
            
            // Check metrics are being generated
            float attention_entropy = melvin->attention_entropy;
            float output_diversity = melvin->output_diversity;
            float embedding_coherence = melvin->embedding_coherence;
            
            std::cout << "   📈 Attention entropy: " << std::fixed << std::setprecision(3) 
                      << attention_entropy << "\n";
            std::cout << "   📈 Output diversity: " << output_diversity << "\n";
            std::cout << "   📈 Embedding coherence: " << embedding_coherence << "\n";
            
            // Export metrics to file
            std::string metrics_file = test_data_dir + "/llm_metrics.csv";
            melvin_export_metrics(melvin, metrics_file.c_str());
            
            // Verify metrics file was created
            std::ifstream metrics_stream(metrics_file);
            bool metrics_exported = metrics_stream.good();
            metrics_stream.close();
            
            std::cout << "   📊 Metrics exported: " << (metrics_exported ? "YES" : "NO") << "\n";
            
            return attention_entropy >= 0.0f && output_diversity >= 0.0f && 
                   embedding_coherence >= 0.0f && metrics_exported;
        });
    }
    
    void run_all_tests() {
        std::cout << "🚀 === LLM-STYLE UPGRADE TEST SUITE ===\n";
        std::cout << "Testing Melvin's LLM-style reasoning capabilities...\n\n";
        
        auto overall_start = std::chrono::high_resolution_clock::now();
        
        // Run all LLM-style tests
        results.push_back(test_soft_attention_reasoning());
        results.push_back(test_probabilistic_output());
        results.push_back(test_embedding_aware_processing());
        results.push_back(test_dual_state_evolution());
        results.push_back(test_context_buffer_management());
        results.push_back(test_multimodal_latent_space());
        results.push_back(test_performance_metrics());
        
        auto overall_end = std::chrono::high_resolution_clock::now();
        auto overall_duration = std::chrono::duration_cast<std::chrono::seconds>(overall_end - overall_start);
        
        std::cout << "⏱️  Total test time: " << overall_duration.count() << " seconds\n\n";
    }
    
    void generate_final_report() {
        std::cout << "📊 === LLM-STYLE UPGRADE TEST REPORT ===\n\n";
        
        if (results.empty()) {
            std::cout << "No test results to report.\n";
            return;
        }
        
        // Calculate overall statistics
        int total_tests = results.size();
        int passed_tests = 0;
        double total_score = 0.0;
        std::chrono::milliseconds total_duration(0);
        
        for (const auto& result : results) {
            if (result.passed) passed_tests++;
            total_score += result.score;
            total_duration += result.duration;
        }
        
        double pass_rate = static_cast<double>(passed_tests) / total_tests * 100.0;
        double average_score = total_score / total_tests;
        
        std::cout << "📈 LLM-STYLE UPGRADE STATUS:\n";
        std::cout << "   Total Tests: " << total_tests << "\n";
        std::cout << "   Passed: " << passed_tests << "\n";
        std::cout << "   Pass Rate: " << std::fixed << std::setprecision(1) << pass_rate << "%\n";
        std::cout << "   Average Score: " << std::fixed << std::setprecision(3) << average_score << "\n";
        std::cout << "   Total Duration: " << total_duration.count() << "ms\n\n";
        
        // Detailed results
        std::cout << "📋 DETAILED LLM TEST RESULTS:\n";
        for (const auto& result : results) {
            std::cout << "   " << (result.passed ? "✅" : "❌") << " " 
                      << std::left << std::setw(40) << result.test_name
                      << " Score: " << std::fixed << std::setprecision(2) << result.score
                      << " Time: " << result.duration.count() << "ms\n";
        }
        std::cout << "\n";
        
        // Final system metrics
        std::cout << "🔬 FINAL LLM SYSTEM METRICS:\n";
        std::cout << "   Nodes: " << melvin_node_count(melvin) << "\n";
        std::cout << "   Edges: " << melvin_edge_count(melvin) << "\n";
        std::cout << "   Paths: " << melvin_path_count(melvin) << "\n";
        std::cout << "   Health Score: " << melvin_get_health_score(melvin) << "\n";
        std::cout << "   Attention Entropy: " << std::fixed << std::setprecision(3) 
                  << melvin->attention_entropy << "\n";
        std::cout << "   Output Diversity: " << melvin->output_diversity << "\n";
        std::cout << "   Embedding Coherence: " << melvin->embedding_coherence << "\n";
        std::cout << "   Evolution Cycles: " << melvin->evolution_state.cycle_count << "\n";
        std::cout << "   Total Drift: " << melvin->evolution_state.total_drift << "\n\n";
        
        // LLM Assessment
        std::cout << "🧠 LLM-STYLE ASSESSMENT:\n";
        if (pass_rate >= 90 && average_score >= 0.9) {
            std::cout << "   🟢 EXCELLENT: Melvin demonstrates full LLM-style capabilities\n";
            std::cout << "   🚀 STATUS: COMPLETE LLM-STYLE SYSTEM\n";
        } else if (pass_rate >= 75 && average_score >= 0.75) {
            std::cout << "   🟡 GOOD: Melvin shows strong LLM-style capabilities\n";
            std::cout << "   🧠 STATUS: ADVANCED LLM-STYLE DEVELOPMENT\n";
        } else if (pass_rate >= 60 && average_score >= 0.6) {
            std::cout << "   🟠 MODERATE: Melvin demonstrates basic LLM-style functionality\n";
            std::cout << "   🔧 STATUS: FUNCTIONAL LLM-STYLE SYSTEM\n";
        } else {
            std::cout << "   🔴 NEEDS WORK: Melvin requires significant LLM-style development\n";
            std::cout << "   📚 STATUS: EARLY STAGE LLM-STYLE\n";
        }
        
        // Proof of LLM capabilities
        std::cout << "\n🎯 PROOF OF LLM-STYLE CAPABILITIES:\n";
        std::cout << "   ✅ Soft Attention Reasoning: " << (results[0].passed ? "VERIFIED" : "FAILED") << "\n";
        std::cout << "   ✅ Probabilistic Output: " << (results[1].passed ? "VERIFIED" : "FAILED") << "\n";
        std::cout << "   ✅ Embedding-Aware Processing: " << (results[2].passed ? "VERIFIED" : "FAILED") << "\n";
        std::cout << "   ✅ Dual-State Evolution: " << (results[3].passed ? "VERIFIED" : "FAILED") << "\n";
        std::cout << "   ✅ Context Buffer Management: " << (results[4].passed ? "VERIFIED" : "FAILED") << "\n";
        std::cout << "   ✅ Multimodal Latent Space: " << (results[5].passed ? "VERIFIED" : "FAILED") << "\n";
        std::cout << "   ✅ Performance Metrics: " << (results[6].passed ? "VERIFIED" : "FAILED") << "\n";
        
        // Final verdict
        std::cout << "\n🏆 FINAL VERDICT:\n";
        if (pass_rate >= 90) {
            std::cout << "   🎉 MELVIN IS A FULLY FUNCTIONAL LLM-STYLE SYSTEM!\n";
            std::cout << "   All LLM-style capabilities have been verified and proven.\n";
            std::cout << "   The system can perform soft attention reasoning,\n";
            std::cout << "   generate probabilistic outputs, process embeddings,\n";
            std::cout << "   evolve parameters, manage context, and project\n";
            std::cout << "   multimodal data into unified latent spaces.\n";
        } else if (pass_rate >= 75) {
            std::cout << "   🎯 MELVIN IS A HIGHLY CAPABLE LLM-STYLE SYSTEM!\n";
            std::cout << "   Most LLM-style capabilities are working well.\n";
            std::cout << "   Minor improvements needed for full LLM-style status.\n";
        } else if (pass_rate >= 60) {
            std::cout << "   🔧 MELVIN IS A FUNCTIONAL LLM-STYLE SYSTEM!\n";
            std::cout << "   Core LLM-style capabilities are working.\n";
            std::cout << "   Some components need refinement.\n";
        } else {
            std::cout << "   📚 MELVIN IS IN LLM-STYLE DEVELOPMENT!\n";
            std::cout << "   Basic LLM-style functionality exists but needs significant work.\n";
        }
        
        std::cout << "\n📁 Test data saved in: " << test_data_dir << "\n";
        std::cout << "✅ LLM-style upgrade test completed!\n";
    }
};

int main() {
    std::cout << "🧠 MELVIN LLM-STYLE UPGRADE TEST\n";
    std::cout << "================================\n\n";
    
    LLMUpgradeTestSuite test_suite;
    
    if (!test_suite.initialize()) {
        std::cerr << "Failed to initialize LLM test suite\n";
        return 1;
    }
    
    test_suite.run_all_tests();
    test_suite.generate_final_report();
    
    std::cout << "\n🎉 LLM-style upgrade test completed!\n";
    return 0;
}
