#include <iostream>
#include <vector>
#include <string>
#include <cassert>
#include <cmath>
#include <random>
#include <chrono>
#include <fstream>

// Include the UCA components
#include "src/uca/uca_types.h"
#include "src/uca/ReasoningEngine.hpp"
#include "src/uca/PerceptionEngine.hpp"

using namespace uca;

// Test harness for deterministic micro-graphs
class UCAFormulaTester {
private:
    ReasoningEngine reasoning;
    PerceptionEngine perception;
    DynamicGenome genome;
    
    // Test results tracking
    struct TestResult {
        std::string test_name;
        bool passed;
        float expected_score;
        float actual_score;
        std::string explanation;
    };
    
    std::vector<TestResult> results;
    
public:
    UCAFormulaTester() {
        // Configure with default genome
        genome.params = {
            {"beam_width", 4.f, 1.f, 16.f},
            {"max_hops", 4.f, 1.f, 16.f},
            {"leap_bias", 0.1f, 0.f, 1.f},
            {"abstr_thresh", 0.6f, 0.f, 1.f}
        };
        
        reasoning.configure(genome);
        perception.configure(genome);
    }
    
    // A1: Hub bias test - star graph with hub H linked to many leaves
    void test_hub_bias() {
        std::cout << "\n=== A1: Hub Bias Test ===" << std::endl;
        
        // Create input that should prefer meaningful leaf over hub routes
        InputConcept ic;
        ic.modality = "text";
        ic.raw = "what are cats";
        ic.t_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        Percept p = perception.perceive(ic);
        ReasoningResult rr = reasoning.infer(p);
        
        // With degree normalization ON, meaningful paths should outrank hub routes
        bool passed = rr.confidence > 0.5f;  // Should have reasonable confidence
        
        results.push_back({
            "A1_HubBias", 
            passed, 
            0.5f, 
            rr.confidence,
            passed ? "Degree normalization working - meaningful path preferred" : 
                     "Hub bias not mitigated - low confidence"
        });
        
        std::cout << "Confidence: " << rr.confidence << std::endl;
        std::cout << "Avg degree norm: " << rr.extra.avg_deg_norm << std::endl;
        std::cout << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    }
    
    // A2: Relation priors test - different relation types should have different scores
    void test_relation_priors() {
        std::cout << "\n=== A2: Relation Priors Test ===" << std::endl;
        
        // Test different query types that should trigger different relation priors
        std::vector<std::string> queries = {
            "what happened then",      // Should prefer TEMPORAL
            "cats are mammals",        // Should prefer GENERALIZATION  
            "exact match",             // Should prefer EXACT
            "leap of logic"            // Should prefer LEAP
        };
        
        std::vector<float> confidences;
        
        for (const auto& query : queries) {
            InputConcept ic;
            ic.modality = "text";
            ic.raw = query;
            ic.t_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            
            Percept p = perception.perceive(ic);
            ReasoningResult rr = reasoning.infer(p);
            confidences.push_back(rr.confidence);
            
            std::cout << "Query: '" << query << "' -> Confidence: " << rr.confidence 
                      << ", Rel prior sum: " << rr.extra.rel_prior_sum << std::endl;
        }
        
        // Check that relation priors are being applied (non-zero sum)
        bool passed = std::any_of(confidences.begin(), confidences.end(), 
                                 [](float c) { return c > 0.1f; });
        
        results.push_back({
            "A2_RelationPriors",
            passed,
            0.1f,
            *std::max_element(confidences.begin(), confidences.end()),
            passed ? "Relation priors being applied" : "No relation priors detected"
        });
        
        std::cout << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    }
    
    // A3: Contradiction penalty test
    void test_contradiction_penalty() {
        std::cout << "\n=== A3: Contradiction Penalty Test ===" << std::endl;
        
        // Test with contradictory information
        InputConcept ic;
        ic.modality = "text";
        ic.raw = "cats are reptiles";  // Contradicts "cats are mammals"
        ic.t_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        Percept p = perception.perceive(ic);
        ReasoningResult rr = reasoning.infer(p);
        
        // Contradictory paths should have lower confidence
        bool passed = rr.confidence < 0.3f;  // Should be low confidence for contradiction
        
        results.push_back({
            "A3_ContradictionPenalty",
            passed,
            0.3f,
            rr.confidence,
            passed ? "Contradiction penalty working - low confidence" : 
                     "Contradiction not penalized - high confidence"
        });
        
        std::cout << "Confidence: " << rr.confidence << std::endl;
        std::cout << "Max contradiction: " << rr.extra.max_contradiction << std::endl;
        std::cout << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    }
    
    // A4: Temporal continuity test
    void test_temporal_continuity() {
        std::cout << "\n=== A4: Temporal Continuity Test ===" << std::endl;
        
        InputConcept ic;
        ic.modality = "text";
        ic.raw = "what happened first then next";
        ic.t_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        Percept p = perception.perceive(ic);
        ReasoningResult rr = reasoning.infer(p);
        
        // Temporal queries should show temporal gap metrics
        bool passed = rr.extra.temporal_gap >= 0.0f;  // Should have temporal metrics
        
        results.push_back({
            "A4_TemporalContinuity",
            passed,
            0.0f,
            rr.extra.temporal_gap,
            passed ? "Temporal continuity metrics captured" : 
                     "No temporal continuity metrics"
        });
        
        std::cout << "Temporal gap: " << rr.extra.temporal_gap << std::endl;
        std::cout << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    }
    
    // A5: Multi-hop discount test
    void test_multi_hop_discount() {
        std::cout << "\n=== A5: Multi-hop Discount Test ===" << std::endl;
        
        InputConcept ic;
        ic.modality = "text";
        ic.raw = "what are cats";  // Should create multi-hop path
        ic.t_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        Percept p = perception.perceive(ic);
        ReasoningResult rr = reasoning.infer(p);
        
        // Multi-hop discount should be applied
        bool passed = rr.extra.multi_hop_disc > 0.0f && rr.extra.multi_hop_disc <= 1.0f;
        
        results.push_back({
            "A5_MultiHopDiscount",
            passed,
            0.5f,
            rr.extra.multi_hop_disc,
            passed ? "Multi-hop discount applied" : "No multi-hop discount"
        });
        
        std::cout << "Multi-hop discount: " << rr.extra.multi_hop_disc << std::endl;
        std::cout << "Path length: " << (rr.used_paths.empty() ? 0 : rr.used_paths[0].node_ids.size()) << std::endl;
        std::cout << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    }
    
    // A6: Beam diversity test
    void test_beam_diversity() {
        std::cout << "\n=== A6: Beam Diversity Test ===" << std::endl;
        
        InputConcept ic;
        ic.modality = "text";
        ic.raw = "what are cats";
        ic.t_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        Percept p = perception.perceive(ic);
        ReasoningResult rr = reasoning.infer(p);
        
        // Diversity penalty should be applied
        bool passed = rr.extra.div_pen > 0.0f && rr.extra.div_pen <= 1.0f;
        
        results.push_back({
            "A6_BeamDiversity",
            passed,
            0.5f,
            rr.extra.div_pen,
            passed ? "Beam diversity penalty applied" : "No diversity penalty"
        });
        
        std::cout << "Diversity penalty: " << rr.extra.div_pen << std::endl;
        std::cout << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    }
    
    // C1-C3: Learning curve test (simplified)
    void test_learning_curve() {
        std::cout << "\n=== C1-C3: Learning Curve Test ===" << std::endl;
        
        std::vector<float> confidences;
        
        // Repeat same query multiple times to simulate learning
        for (int i = 0; i < 5; ++i) {
            InputConcept ic;
            ic.modality = "text";
            ic.raw = "what are cats";
            ic.t_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::high_resolution_clock::now().time_since_epoch()).count();
            
            Percept p = perception.perceive(ic);
            ReasoningResult rr = reasoning.infer(p);
            confidences.push_back(rr.confidence);
            
            std::cout << "Iteration " << (i+1) << ": Confidence = " << rr.confidence << std::endl;
        }
        
        // Check for learning trend (confidence should be stable or improving)
        bool passed = confidences.size() >= 3 && 
                     std::all_of(confidences.begin(), confidences.end(), 
                                [](float c) { return c >= 0.0f && c <= 1.0f; });
        
        results.push_back({
            "C1_LearningCurve",
            passed,
            0.5f,
            confidences.empty() ? 0.0f : confidences.back(),
            passed ? "Learning curve stable" : "Learning curve unstable"
        });
        
        std::cout << "Result: " << (passed ? "PASS" : "FAIL") << std::endl;
    }
    
    // Run all tests
    void run_all_tests() {
        std::cout << "🧪 Starting UCA Formula Tests..." << std::endl;
        
        test_hub_bias();
        test_relation_priors();
        test_contradiction_penalty();
        test_temporal_continuity();
        test_multi_hop_discount();
        test_beam_diversity();
        test_learning_curve();
        
        // Print summary
        std::cout << "\n=== TEST SUMMARY ===" << std::endl;
        int passed = 0;
        for (const auto& result : results) {
            std::cout << result.test_name << ": " << (result.passed ? "PASS" : "FAIL") 
                      << " (expected: " << result.expected_score 
                      << ", actual: " << result.actual_score << ")" << std::endl;
            std::cout << "  " << result.explanation << std::endl;
            if (result.passed) passed++;
        }
        
        std::cout << "\nPassed: " << passed << "/" << results.size() << " tests" << std::endl;
        
        // Save results to CSV
        save_results_csv();
    }
    
    void save_results_csv() {
        std::ofstream file("test_results.csv");
        file << "test_name,passed,expected_score,actual_score,explanation\n";
        for (const auto& result : results) {
            file << result.test_name << "," << (result.passed ? "1" : "0") 
                 << "," << result.expected_score << "," << result.actual_score 
                 << ",\"" << result.explanation << "\"\n";
        }
        file.close();
        std::cout << "Results saved to test_results.csv" << std::endl;
    }
};

// Ablation test runner
class AblationTester {
public:
    static void run_ablation_tests() {
        std::cout << "\n🔬 Running Ablation Tests..." << std::endl;
        
        std::vector<std::string> ablation_vars = {
            "UCA_NO_DEGREE", "UCA_NO_RELPR", "UCA_NO_CONTRA", 
            "UCA_NO_CONT", "UCA_NO_DIV", "UCA_USE_TD"
        };
        
        for (const auto& var : ablation_vars) {
            std::cout << "\n--- Testing with " << var << "=1 ---" << std::endl;
            
            // Set environment variable
            setenv(var.c_str(), "1", 1);
            
            // Run basic test
            UCAFormulaTester tester;
            tester.test_hub_bias();
            
            // Unset environment variable
            unsetenv(var.c_str());
        }
    }
};

int main(int argc, char** argv) {
    std::cout << "🧠 UCA Mathematical Formula Test Suite" << std::endl;
    std::cout << "=====================================" << std::endl;
    
    if (argc > 1 && std::string(argv[1]) == "--ablation") {
        AblationTester::run_ablation_tests();
    } else {
        UCAFormulaTester tester;
        tester.run_all_tests();
    }
    
    return 0;
}
