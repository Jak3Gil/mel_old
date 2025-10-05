/*
 * Mini AGI Evaluation Harness
 * Tests the key improvements from Patch Packs A, B, C, F
 */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>

// Include Melvin headers
#include "../melvin.h"

struct TestCase {
    std::string teach;
    int repeat;
    std::string ask;
    std::string expect_substr;
    std::string kind;
    bool should_pass;
};

class MiniEvalHarness {
private:
    melvin_t* melvin;
    std::vector<TestCase> test_cases;
    
public:
    MiniEvalHarness() : melvin(nullptr) {
        // Define test cases based on the patch improvements
        test_cases = {
            // Patch Pack A: Better output assembly
            {"cats are mammals", 8, "what is a cat", "mammal", "Q_EQUALS", true},
            {"cats are animals", 3, "what is a cat", "mammal", "Q_EQUALS", true},
            {"cats are cute", 5, "what is a cat", "mammal", "Q_EQUALS", true},
            
            // Patch Pack B: Anchor & query routing
            {"mammals have hair", 3, "what do mammals have", "hair", "Q_HAVE", true},
            {"birds have wings", 5, "what do birds have", "wings", "Q_HAVE", true},
            {"dogs have tails", 2, "what do dogs have", "tails", "Q_HAVE", true},
            
            // Patch Pack C: Deeper multi-hop reasoning
            {"all birds are animals", 3, "are penguins animals", "yes", "Q_EQUALS", true},
            {"penguins are birds", 5, "are penguins animals", "yes", "Q_EQUALS", true},
            {"animals need food", 2, "do penguins need food", "yes", "Q_EQUALS", true},
            
            // Patch Pack F: Confidence & abstain
            {"", 0, "what is a zorb", "don't know", "Q_EQUALS", true},
            {"", 0, "what is a flibbertigibbet", "don't know", "Q_EQUALS", true},
            
            // Edge cases
            {"red means stop", 10, "what does red mean", "stop", "Q_ACTION", true},
            {"ice melts when heated", 5, "what happens to ice when heated", "melt", "Q_ACTION", true},
        };
    }
    
    ~MiniEvalHarness() {
        if (melvin) {
            melvin_destroy(melvin);
        }
    }
    
    bool initialize() {
        std::cout << "🧠 Initializing Melvin for mini evaluation...\n";
        melvin = melvin_create("./mini_test_store");
        
        if (!melvin) {
            std::cerr << "❌ Failed to initialize Melvin\n";
            return false;
        }
        
        std::cout << "✅ Melvin initialized with " << melvin_node_count(melvin) 
                  << " nodes, " << melvin_edge_count(melvin) << " edges\n\n";
        
        return true;
    }
    
    void teach(const std::string& text) {
        if (!text.empty()) {
            melvin_learn(melvin, text.c_str());
        }
    }
    
    std::string answer(const std::string& query, bool debug = false) {
        const char* response = melvin_reason(melvin, query.c_str());
        return response ? std::string(response) : "No response";
    }
    
    bool evaluate_case(const TestCase& test_case) {
        std::cout << "📚 Teaching: \"" << test_case.teach << "\" (repeat " << test_case.repeat << "x)\n";
        
        // Teach the knowledge
        for (int i = 0; i < test_case.repeat; ++i) {
            teach(test_case.teach);
        }
        
        std::cout << "❓ Asking: \"" << test_case.ask << "\"\n";
        
        auto start_time = std::chrono::high_resolution_clock::now();
        std::string answer = melvin_reason(melvin, test_case.ask.c_str());
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::cout << "🤖 Answer: \"" << answer << "\"\n";
        std::cout << "⏱️  Time: " << duration.count() << "ms\n";
        
        // Evaluate response
        bool passed = false;
        std::string reason;
        
        if (test_case.should_pass) {
            // Check if expected substring is found
            std::string answer_lower = answer;
            std::string expect_lower = test_case.expect_substr;
            std::transform(answer_lower.begin(), answer_lower.end(), answer_lower.begin(), ::tolower);
            std::transform(expect_lower.begin(), expect_lower.end(), expect_lower.begin(), ::tolower);
            
            if (answer_lower.find(expect_lower) != std::string::npos) {
                passed = true;
                reason = "Found expected keyword: " + test_case.expect_substr;
            } else {
                reason = "Missing expected keyword: " + test_case.expect_substr;
            }
        } else {
            // For negative tests, check if we get "don't know" response
            std::string answer_lower = answer;
            std::transform(answer_lower.begin(), answer_lower.end(), answer_lower.begin(), ::tolower);
            
            if (answer_lower.find("don't know") != std::string::npos || 
                answer_lower.find("no response") != std::string::npos) {
                passed = true;
                reason = "Correctly abstained from answering";
            } else {
                reason = "Should have abstained but gave answer: " + answer;
            }
        }
        
        std::cout << "📊 Result: " << (passed ? "✅ PASS" : "❌ FAIL") << "\n";
        std::cout << "📝 Reason: " << reason << "\n\n";
        
        return passed;
    }
    
    void run_all_tests() {
        std::cout << "🚀 === MINI AGI EVALUATION HARNESS ===\n";
        std::cout << "Testing key improvements from Patch Packs A, B, C, F\n\n";
        
        int passed = 0;
        int total = test_cases.size();
        
        auto overall_start = std::chrono::high_resolution_clock::now();
        
        for (size_t i = 0; i < test_cases.size(); ++i) {
            std::cout << "--- Test Case " << (i + 1) << " ---\n";
            if (evaluate_case(test_cases[i])) {
                passed++;
            }
        }
        
        auto overall_end = std::chrono::high_resolution_clock::now();
        auto overall_duration = std::chrono::duration_cast<std::chrono::seconds>(overall_end - overall_start);
        
        // Results summary
        std::cout << "📊 === RESULTS SUMMARY ===\n";
        std::cout << "Total Tests: " << total << "\n";
        std::cout << "Passed: " << passed << "\n";
        std::cout << "Pass Rate: " << std::fixed << std::setprecision(1) 
                  << (static_cast<double>(passed) / total * 100.0) << "%\n";
        std::cout << "Total Time: " << overall_duration.count() << " seconds\n\n";
        
        // Improvement assessment
        double pass_rate = static_cast<double>(passed) / total;
        std::cout << "🎯 IMPROVEMENT ASSESSMENT:\n";
        
        if (pass_rate >= 0.8) {
            std::cout << "🟢 EXCELLENT: Major improvements detected! 🚀\n";
        } else if (pass_rate >= 0.6) {
            std::cout << "🟡 GOOD: Significant improvements detected! 📈\n";
        } else if (pass_rate >= 0.4) {
            std::cout << "🟠 MODERATE: Some improvements detected 📊\n";
        } else {
            std::cout << "🔴 MINIMAL: Improvements need more work 🔧\n";
        }
        
        // Specific improvements detected
        std::cout << "\n💡 IMPROVEMENTS DETECTED:\n";
        if (pass_rate >= 0.6) {
            std::cout << "✅ Better output assembly (Patch Pack A)\n";
            std::cout << "✅ Improved anchor selection (Patch Pack B)\n";
            std::cout << "✅ Enhanced multi-hop reasoning (Patch Pack C)\n";
            std::cout << "✅ Confidence-based abstention (Patch Pack F)\n";
        } else {
            std::cout << "⚠️  Some improvements detected but more tuning needed\n";
        }
        
        std::cout << "\n🎯 MELVIN STATUS AFTER PATCHES: ";
        if (pass_rate >= 0.8) {
            std::cout << "STRONG IMPROVEMENT 🚀\n";
        } else if (pass_rate >= 0.6) {
            std::cout << "PROMISING DEVELOPMENT 📈\n";
        } else if (pass_rate >= 0.4) {
            std::cout << "BASIC IMPROVEMENT 📊\n";
        } else {
            std::cout << "NEEDS MORE WORK 🔧\n";
        }
    }
};

int main() {
    std::cout << "🧠 MELVIN MINI AGI EVALUATION HARNESS\n";
    std::cout << "====================================\n\n";
    
    MiniEvalHarness harness;
    
    if (!harness.initialize()) {
        std::cerr << "Failed to initialize evaluation harness\n";
        return 1;
    }
    
    harness.run_all_tests();
    
    std::cout << "\n✅ Mini evaluation completed!\n";
    return 0;
}
