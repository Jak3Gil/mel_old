/*
 * AGI-STYLE COMPREHENSIVE TEST FOR MELVIN
 * 
 * This test evaluates Melvin across multiple cognitive domains:
 * - Pattern Recognition
 * - Logical Reasoning  
 * - Causal Understanding
 * - Abstract Thinking
 * - Language Understanding
 * - Multi-hop Reasoning
 * - Creative Problem Solving
 * - Knowledge Integration
 */

#include <iostream>
#include <vector>
#include <string>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cmath>

// Include Melvin headers
#include "melvin.h"

struct TestResult {
    std::string test_name;
    std::string query;
    std::string response;
    bool passed;
    double score;
    std::string notes;
};

class AGITestSuite {
private:
    melvin_t* melvin;
    std::vector<TestResult> results;
    
public:
    AGITestSuite() : melvin(nullptr) {}
    
    ~AGITestSuite() {
        if (melvin) {
            melvin_destroy(melvin);
        }
    }
    
    bool initialize() {
        std::cout << "🧠 Initializing Melvin for AGI testing...\n";
        melvin = melvin_create("./test_store");
        
        if (!melvin) {
            std::cerr << "❌ Failed to initialize Melvin\n";
            return false;
        }
        
        std::cout << "✅ Melvin initialized with " << melvin_node_count(melvin) 
                  << " nodes, " << melvin_edge_count(melvin) << " edges\n";
        std::cout << "✅ Health score: " << melvin_get_health_score(melvin) << "\n\n";
        
        return true;
    }
    
    void inject_knowledge() {
        std::cout << "📚 Injecting comprehensive knowledge base...\n";
        
        // Mathematical patterns
        melvin_learn(melvin, "2 4 8 16 32 are exponential powers of 2");
        melvin_learn(melvin, "exponential growth doubles each step");
        melvin_learn(melvin, "fibonacci sequence is 1 1 2 3 5 8 13");
        melvin_learn(melvin, "each fibonacci number is sum of previous two");
        melvin_learn(melvin, "prime numbers are divisible only by 1 and themselves");
        melvin_learn(melvin, "2 3 5 7 11 13 are prime numbers");
        
        // Logical reasoning
        melvin_learn(melvin, "all birds have wings");
        melvin_learn(melvin, "penguins are birds");
        melvin_learn(melvin, "if A then B and if B then C then if A then C");
        melvin_learn(melvin, "correlation does not imply causation");
        melvin_learn(melvin, "syllogism has major premise minor premise conclusion");
        
        // Causal relationships
        melvin_learn(melvin, "ice melts when heated");
        melvin_learn(melvin, "water freezes when cooled");
        melvin_learn(melvin, "seasons change because earth tilts");
        melvin_learn(melvin, "sun causes earth to be warm");
        melvin_learn(melvin, "gravity causes objects to fall");
        melvin_learn(melvin, "friction causes motion to slow");
        
        // Abstract concepts
        melvin_learn(melvin, "beauty is subjective and personal");
        melvin_learn(melvin, "truth is objective and universal");
        melvin_learn(melvin, "justice requires fairness and equality");
        melvin_learn(melvin, "freedom allows choice and expression");
        melvin_learn(melvin, "love involves care and connection");
        
        // System analogies
        melvin_learn(melvin, "cell is like a factory with workers");
        melvin_learn(melvin, "heart is like a pump for blood");
        melvin_learn(melvin, "brain is like a processor for information");
        melvin_learn(melvin, "ecosystem is like a web of connections");
        melvin_learn(melvin, "economy is like a circulation system");
        
        // Color associations
        melvin_learn(melvin, "red means stop and danger");
        melvin_learn(melvin, "green means go and safe");
        melvin_learn(melvin, "yellow means caution and warning");
        melvin_learn(melvin, "blue means calm and sky");
        melvin_learn(melvin, "white means pure and clean");
        
        // Scientific principles
        melvin_learn(melvin, "energy cannot be created or destroyed");
        melvin_learn(melvin, "matter consists of atoms and molecules");
        melvin_learn(melvin, "evolution explains biological diversity");
        melvin_learn(melvin, "photosynthesis converts light to energy");
        melvin_learn(melvin, "gravity attracts masses together");
        
        std::cout << "✅ Knowledge injection complete\n\n";
    }
    
    TestResult run_test(const std::string& test_name, const std::string& query, 
                       const std::vector<std::string>& expected_keywords = {},
                       bool require_exact = false) {
        
        std::cout << "🔍 Test: " << test_name << "\n";
        std::cout << "❓ Query: " << query << "\n";
        
        auto start_time = std::chrono::high_resolution_clock::now();
        const char* response = melvin_reason(melvin, query.c_str());
        auto end_time = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        std::string response_str = response ? response : "No response";
        std::cout << "🤖 Response: " << response_str << "\n";
        std::cout << "⏱️  Time: " << duration.count() << "ms\n";
        
        TestResult result;
        result.test_name = test_name;
        result.query = query;
        result.response = response_str;
        result.score = 0.0;
        result.notes = "";
        
        // Evaluate response
        if (expected_keywords.empty()) {
            // If no expected keywords, check for non-empty response
            result.passed = !response_str.empty() && 
                           response_str != "No response" && 
                           response_str != "I don't have enough information yet.";
            result.score = result.passed ? 1.0 : 0.0;
        } else {
            // Check for expected keywords
            std::string response_lower = response_str;
            std::transform(response_lower.begin(), response_lower.end(), 
                          response_lower.begin(), ::tolower);
            
            int keyword_matches = 0;
            for (const auto& keyword : expected_keywords) {
                std::string keyword_lower = keyword;
                std::transform(keyword_lower.begin(), keyword_lower.end(), 
                              keyword_lower.begin(), ::tolower);
                
                if (response_lower.find(keyword_lower) != std::string::npos) {
                    keyword_matches++;
                }
            }
            
            result.score = static_cast<double>(keyword_matches) / expected_keywords.size();
            result.passed = require_exact ? (result.score == 1.0) : (result.score >= 0.5);
            
            result.notes = "Found " + std::to_string(keyword_matches) + "/" + 
                          std::to_string(expected_keywords.size()) + " keywords";
        }
        
        std::cout << "📊 Score: " << std::fixed << std::setprecision(2) << result.score 
                  << " (" << (result.passed ? "PASS" : "FAIL") << ")\n";
        std::cout << "📝 Notes: " << result.notes << "\n\n";
        
        return result;
    }
    
    void run_pattern_recognition_tests() {
        std::cout << "🧮 === PATTERN RECOGNITION TESTS ===\n\n";
        
        results.push_back(run_test(
            "Exponential Pattern Recognition",
            "What pattern do you see in: 2, 4, 8, 16, 32?",
            {"exponential", "double", "2", "power", "pattern"}
        ));
        
        results.push_back(run_test(
            "Fibonacci Pattern Recognition", 
            "What comes next in this sequence: 1, 1, 2, 3, 5, 8?",
            {"13", "fibonacci", "sum", "previous", "sequence"}
        ));
        
        results.push_back(run_test(
            "Prime Number Recognition",
            "Which of these are prime numbers: 7, 8, 11, 12, 13?",
            {"7", "11", "13", "prime", "divisible"}
        ));
        
        results.push_back(run_test(
            "Geometric Pattern",
            "What is the pattern: square, circle, triangle, square, circle?",
            {"repeat", "geometric", "shape", "pattern"}
        ));
    }
    
    void run_logical_reasoning_tests() {
        std::cout << "🔗 === LOGICAL REASONING TESTS ===\n\n";
        
        results.push_back(run_test(
            "Syllogistic Reasoning",
            "All birds have wings. Penguins are birds. Do penguins have wings?",
            {"yes", "penguins", "wings", "birds"}
        ));
        
        results.push_back(run_test(
            "Transitive Reasoning",
            "If A causes B and B causes C, what is the relationship between A and C?",
            {"a", "causes", "c", "transitive", "chain"}
        ));
        
        results.push_back(run_test(
            "Contradiction Detection",
            "This statement is false. Is this statement true or false?",
            {"contradiction", "paradox", "both", "neither"}
        ));
        
        results.push_back(run_test(
            "Correlation vs Causation",
            "Ice cream sales increase when drowning deaths increase. Does ice cream cause drowning?",
            {"no", "correlation", "causation", "summer", "temperature"}
        ));
    }
    
    void run_causal_understanding_tests() {
        std::cout << "⚡ === CAUSAL UNDERSTANDING TESTS ===\n\n";
        
        results.push_back(run_test(
            "Heat and State Change",
            "What happens to ice when you heat it?",
            {"melt", "water", "liquid", "heat", "temperature"}
        ));
        
        results.push_back(run_test(
            "Seasonal Changes",
            "Why do we have seasons on Earth?",
            {"tilt", "axis", "sun", "earth", "angle"}
        ));
        
        results.push_back(run_test(
            "Gravity and Motion",
            "Why do objects fall when dropped?",
            {"gravity", "force", "pull", "down", "mass"}
        ));
        
        results.push_back(run_test(
            "Chain of Causation",
            "How does the sun cause plants to grow?",
            {"photosynthesis", "light", "energy", "food", "process"}
        ));
    }
    
    void run_abstract_thinking_tests() {
        std::cout << "🎭 === ABSTRACT THINKING TESTS ===\n\n";
        
        results.push_back(run_test(
            "Concept Analogies",
            "How is a cell like a factory?",
            {"workers", "production", "organization", "function", "components"}
        ));
        
        results.push_back(run_test(
            "System Analogies",
            "How is the heart like a pump?",
            {"circulation", "fluid", "pressure", "mechanism", "function"}
        ));
        
        results.push_back(run_test(
            "Abstract Concepts",
            "What is the difference between beauty and truth?",
            {"subjective", "objective", "personal", "universal", "opinion"}
        ));
        
        results.push_back(run_test(
            "Metaphorical Thinking",
            "How is knowledge like a tree?",
            {"growth", "branches", "roots", "foundation", "development"}
        ));
    }
    
    void run_language_understanding_tests() {
        std::cout << "📝 === LANGUAGE UNDERSTANDING TESTS ===\n\n";
        
        results.push_back(run_test(
            "Semantic Understanding",
            "What do red traffic lights mean?",
            {"stop", "danger", "warning", "signal"}
        ));
        
        results.push_back(run_test(
            "Contextual Understanding",
            "In what context would you see green lights?",
            {"traffic", "go", "safe", "proceed", "signal"}
        ));
        
        results.push_back(run_test(
            "Ambiguity Resolution",
            "What does 'bank' mean in 'river bank' vs 'money bank'?",
            {"different", "context", "meaning", "river", "money"}
        ));
        
        results.push_back(run_test(
            "Inference from Context",
            "If someone says 'the weather is beautiful today', what can you infer?",
            {"sunny", "nice", "pleasant", "good", "weather"}
        ));
    }
    
    void run_multi_hop_reasoning_tests() {
        std::cout << "🔀 === MULTI-HOP REASONING TESTS ===\n\n";
        
        results.push_back(run_test(
            "Multi-step Inference",
            "If all mammals are warm-blooded and all dogs are mammals, what can you conclude about dogs?",
            {"warm-blooded", "mammals", "dogs", "temperature"}
        ));
        
        results.push_back(run_test(
            "Chain Reasoning",
            "If A is bigger than B and B is bigger than C, which is biggest?",
            {"a", "biggest", "largest", "size", "comparison"}
        ));
        
        results.push_back(run_test(
            "Complex Deduction",
            "If it's raining and rain makes things wet, and wet things are slippery, what happens to roads when it rains?",
            {"slippery", "wet", "dangerous", "roads", "rain"}
        ));
        
        results.push_back(run_test(
            "Indirect Relationships",
            "How are clouds related to plant growth?",
            {"rain", "water", "photosynthesis", "sun", "process"}
        ));
    }
    
    void run_creative_problem_solving_tests() {
        std::cout << "💡 === CREATIVE PROBLEM SOLVING TESTS ===\n\n";
        
        results.push_back(run_test(
            "Novel Connections",
            "What do you get when you combine a bird and a plane?",
            {"airplane", "flight", "wings", "aviation", "flying"}
        ));
        
        results.push_back(run_test(
            "Alternative Solutions",
            "How many ways can you think of to keep ice from melting?",
            {"cold", "freezer", "insulation", "temperature", "container"}
        ));
        
        results.push_back(run_test(
            "Creative Analogies",
            "If time were a river, how would you describe the flow?",
            {"current", "direction", "movement", "flow", "forward"}
        ));
        
        results.push_back(run_test(
            "Hypothetical Scenarios",
            "What would happen if gravity were twice as strong?",
            {"heavier", "fall", "difficult", "weight", "force"}
        ));
    }
    
    void run_knowledge_integration_tests() {
        std::cout << "🧩 === KNOWLEDGE INTEGRATION TESTS ===\n\n";
        
        results.push_back(run_test(
            "Cross-domain Integration",
            "How do physics and biology work together in photosynthesis?",
            {"light", "energy", "plants", "process", "conversion"}
        ));
        
        results.push_back(run_test(
            "System-level Understanding",
            "How does the water cycle connect to weather patterns?",
            {"evaporation", "condensation", "precipitation", "clouds", "cycle"}
        ));
        
        results.push_back(run_test(
            "Interdisciplinary Thinking",
            "How do mathematics and art relate to each other?",
            {"patterns", "proportion", "geometry", "beauty", "harmony"}
        ));
        
        results.push_back(run_test(
            "Holistic Analysis",
            "What are the environmental impacts of deforestation?",
            {"ecosystem", "climate", "animals", "oxygen", "balance"}
        ));
    }
    
    void run_all_tests() {
        std::cout << "🚀 === COMPREHENSIVE AGI TEST SUITE ===\n";
        std::cout << "Testing Melvin's cognitive capabilities across multiple domains...\n\n";
        
        auto overall_start = std::chrono::high_resolution_clock::now();
        
        run_pattern_recognition_tests();
        run_logical_reasoning_tests();
        run_causal_understanding_tests();
        run_abstract_thinking_tests();
        run_language_understanding_tests();
        run_multi_hop_reasoning_tests();
        run_creative_problem_solving_tests();
        run_knowledge_integration_tests();
        
        auto overall_end = std::chrono::high_resolution_clock::now();
        auto overall_duration = std::chrono::duration_cast<std::chrono::seconds>(overall_end - overall_start);
        
        std::cout << "⏱️  Total test time: " << overall_duration.count() << " seconds\n\n";
    }
    
    void generate_report() {
        std::cout << "📊 === AGI TEST RESULTS REPORT ===\n\n";
        
        if (results.empty()) {
            std::cout << "No test results to report.\n";
            return;
        }
        
        // Calculate statistics
        int total_tests = results.size();
        int passed_tests = 0;
        double total_score = 0.0;
        
        std::vector<std::string> categories = {
            "Pattern Recognition", "Logical Reasoning", "Causal Understanding",
            "Abstract Thinking", "Language Understanding", "Multi-hop Reasoning",
            "Creative Problem Solving", "Knowledge Integration"
        };
        
        std::vector<double> category_scores(categories.size(), 0.0);
        std::vector<int> category_counts(categories.size(), 0);
        
        // Group results by category (approximate)
        for (const auto& result : results) {
            if (result.passed) passed_tests++;
            total_score += result.score;
            
            // Map test names to categories (simplified)
            size_t category_idx = 0;
            if (result.test_name.find("Pattern") != std::string::npos) category_idx = 0;
            else if (result.test_name.find("Logical") != std::string::npos) category_idx = 1;
            else if (result.test_name.find("Causal") != std::string::npos) category_idx = 2;
            else if (result.test_name.find("Abstract") != std::string::npos) category_idx = 3;
            else if (result.test_name.find("Language") != std::string::npos) category_idx = 4;
            else if (result.test_name.find("Multi-hop") != std::string::npos) category_idx = 5;
            else if (result.test_name.find("Creative") != std::string::npos) category_idx = 6;
            else if (result.test_name.find("Integration") != std::string::npos) category_idx = 7;
            
            category_scores[category_idx] += result.score;
            category_counts[category_idx]++;
        }
        
        // Overall statistics
        double pass_rate = static_cast<double>(passed_tests) / total_tests * 100.0;
        double average_score = total_score / total_tests;
        
        std::cout << "📈 OVERALL STATISTICS:\n";
        std::cout << "   Total Tests: " << total_tests << "\n";
        std::cout << "   Passed: " << passed_tests << "\n";
        std::cout << "   Pass Rate: " << std::fixed << std::setprecision(1) << pass_rate << "%\n";
        std::cout << "   Average Score: " << std::fixed << std::setprecision(3) << average_score << "\n\n";
        
        // Category breakdown
        std::cout << "🎯 CATEGORY PERFORMANCE:\n";
        for (size_t i = 0; i < categories.size(); ++i) {
            if (category_counts[i] > 0) {
                double category_avg = category_scores[i] / category_counts[i];
                std::cout << "   " << std::left << std::setw(25) << categories[i] 
                          << ": " << std::fixed << std::setprecision(3) << category_avg
                          << " (" << category_counts[i] << " tests)\n";
            }
        }
        std::cout << "\n";
        
        // Detailed results
        std::cout << "📋 DETAILED TEST RESULTS:\n";
        for (const auto& result : results) {
            std::cout << "   " << (result.passed ? "✅" : "❌") << " " 
                      << std::left << std::setw(35) << result.test_name
                      << " Score: " << std::fixed << std::setprecision(2) << result.score;
            if (!result.notes.empty()) {
                std::cout << " (" << result.notes << ")";
            }
            std::cout << "\n";
        }
        std::cout << "\n";
        
        // AGI Assessment
        std::cout << "🧠 AGI ASSESSMENT:\n";
        if (average_score >= 0.8) {
            std::cout << "   🟢 EXCELLENT: Melvin shows strong AGI-like capabilities\n";
        } else if (average_score >= 0.6) {
            std::cout << "   🟡 GOOD: Melvin demonstrates solid reasoning abilities\n";
        } else if (average_score >= 0.4) {
            std::cout << "   🟠 MODERATE: Melvin shows basic reasoning with room for improvement\n";
        } else {
            std::cout << "   🔴 NEEDS WORK: Melvin requires significant development\n";
        }
        
        // Recommendations
        std::cout << "\n💡 RECOMMENDATIONS:\n";
        if (average_score < 0.6) {
            std::cout << "   - Expand knowledge base with more diverse examples\n";
            std::cout << "   - Improve reasoning algorithms for complex queries\n";
            std::cout << "   - Enhance multi-hop reasoning capabilities\n";
        }
        if (pass_rate < 70) {
            std::cout << "   - Tune confidence thresholds for better response generation\n";
            std::cout << "   - Improve semantic understanding of queries\n";
        }
        std::cout << "   - Continue training with more complex reasoning tasks\n";
        std::cout << "   - Add more creative and abstract thinking examples\n";
        
        std::cout << "\n🎯 MELVIN AGI STATUS: ";
        if (average_score >= 0.8 && pass_rate >= 80) {
            std::cout << "STRONG AGI CANDIDATE 🚀\n";
        } else if (average_score >= 0.6 && pass_rate >= 60) {
            std::cout << "PROMISING AGI DEVELOPMENT 🧠\n";
        } else if (average_score >= 0.4 && pass_rate >= 40) {
            std::cout << "BASIC AGI FOUNDATION 🔧\n";
        } else {
            std::cout << "EARLY STAGE DEVELOPMENT 📚\n";
        }
    }
};

int main() {
    std::cout << "🧠 MELVIN AGI COMPREHENSIVE TEST SUITE\n";
    std::cout << "=====================================\n\n";
    
    AGITestSuite test_suite;
    
    if (!test_suite.initialize()) {
        std::cerr << "Failed to initialize test suite\n";
        return 1;
    }
    
    test_suite.inject_knowledge();
    test_suite.run_all_tests();
    test_suite.generate_report();
    
    std::cout << "\n✅ AGI test suite completed!\n";
    return 0;
}
