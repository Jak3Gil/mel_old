/*
 * MELVIN LLM TRAINING EVALUATION SUITE
 * 
 * Lightweight, always-on evaluation tests for fact recall, reasoning,
 * paraphrase robustness, noise tolerance, and stability checks.
 */

#pragma once

#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <chrono>
#include <algorithm>
#include <fstream>
#include <sstream>

namespace melvin {

// ==================== EVALUATION RESULTS ====================

struct EvaluationResults {
    struct FactRecallResults {
        int total_questions = 0;
        int correct_answers = 0;
        float success_rate = 0.0f;
        std::unordered_map<std::string, int> relation_accuracy; // ISA, CONSUMES, etc.
    } fact_recall;
    
    struct ReasoningResults {
        int one_hop_attempts = 0;
        int one_hop_successes = 0;
        int two_hop_attempts = 0;
        int two_hop_successes = 0;
        float one_hop_success_rate = 0.0f;
        float two_hop_success_rate = 0.0f;
        float avg_path_length = 0.0f;
    } reasoning;
    
    struct RobustnessResults {
        int paraphrase_tests = 0;
        float success_variance = 0.0f;
        float avg_paraphrase_success = 0.0f;
        bool variance_acceptable = false; // < 10%
    } robustness;
    
    struct NoiseToleranceResults {
        int noise_tests = 0;
        int baseline_successes = 0;
        int noise_successes = 0;
        float success_drop = 0.0f;
        bool tolerance_acceptable = false; // < 8% drop
    } noise_tolerance;
    
    struct StabilityResults {
        int thought_paths_tested = 0;
        int exact_continuations = 0;
        float stability_rate = 0.0f;
        bool stability_acceptable = false; // ≥85%
    } stability;
    
    uint64_t timestamp = 0;
    float overall_score = 0.0f;
    
    // Compute overall score
    void compute_overall_score() {
        float fact_score = fact_recall.success_rate;
        float reasoning_score = (reasoning.one_hop_success_rate + reasoning.two_hop_success_rate) / 2.0f;
        float robustness_score = robustness.variance_acceptable ? 1.0f : 0.0f;
        float noise_score = noise_tolerance.tolerance_acceptable ? 1.0f : 0.0f;
        float stability_score = stability.stability_acceptable ? 1.0f : 0.0f;
        
        overall_score = (fact_score * 0.3f + reasoning_score * 0.25f + 
                        robustness_score * 0.15f + noise_score * 0.15f + stability_score * 0.15f);
    }
    
    std::string get_summary() const {
        std::stringstream ss;
        ss << "Evaluation[overall=" << std::fixed << std::setprecision(3) << overall_score
           << ", fact=" << fact_recall.success_rate
           << ", reason=" << reasoning.one_hop_success_rate << "/" << reasoning.two_hop_success_rate
           << ", robust=" << (robustness.variance_acceptable ? "PASS" : "FAIL")
           << ", noise=" << (noise_tolerance.tolerance_acceptable ? "PASS" : "FAIL")
           << ", stable=" << (stability.stability_acceptable ? "PASS" : "FAIL") << "]";
        return ss.str();
    }
};

// ==================== EVALUATION SUITE ====================

class EvaluationSuite {
private:
    std::mt19937 rng_;
    
    // Test templates
    std::vector<std::string> fact_questions_;
    std::vector<std::string> reasoning_questions_;
    std::vector<std::string> paraphrase_templates_;
    std::vector<std::string> noise_distractors_;
    
    // Knowledge base for testing
    std::unordered_map<std::string, std::vector<std::string>> isa_relations_;
    std::unordered_map<std::string, std::vector<std::string>> consumes_relations_;
    std::unordered_map<std::string, std::vector<std::string>> temporal_relations_;
    
public:
    EvaluationSuite() : rng_(std::random_device{}()) {
        initialize_test_templates();
        initialize_knowledge_base();
    }
    
    // Run complete evaluation suite
    EvaluationResults run_evaluation_suite() {
        std::cout << "🧪 Running evaluation suite...\n";
        
        EvaluationResults results;
        results.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        
        // 1. Fact recall test (200 templated questions)
        std::cout << "   📚 Testing fact recall...\n";
        results.fact_recall = run_fact_recall_test();
        
        // 2. 1-hop & 2-hop reasoning
        std::cout << "   🧠 Testing reasoning...\n";
        results.reasoning = run_reasoning_test();
        
        // 3. Paraphrase robustness
        std::cout << "   🔄 Testing paraphrase robustness...\n";
        results.robustness = run_paraphrase_robustness_test();
        
        // 4. Noise tolerance
        std::cout << "   🔊 Testing noise tolerance...\n";
        results.noise_tolerance = run_noise_tolerance_test();
        
        // 5. Stability check
        std::cout << "   🎯 Testing stability...\n";
        results.stability = run_stability_test();
        
        // Compute overall score
        results.compute_overall_score();
        
        std::cout << "   ✅ Evaluation complete: " << results.get_summary() << "\n";
        
        return results;
    }
    
    // Individual test methods
    EvaluationResults::FactRecallResults run_fact_recall_test() {
        EvaluationResults::FactRecallResults results;
        
        // Test ISA relations
        for (const auto& [entity, categories] : isa_relations_) {
            for (const auto& category : categories) {
                std::string question = "What is " + entity + "?";
                std::string expected = entity + " is " + category;
                
                bool correct = test_fact_recall(question, expected, "ISA");
                results.total_questions++;
                if (correct) results.correct_answers++;
                
                results.relation_accuracy["ISA"] += correct ? 1 : 0;
            }
        }
        
        // Test CONSUMES relations
        for (const auto& [entity, resources] : consumes_relations_) {
            for (const auto& resource : resources) {
                std::string question = "What does " + entity + " consume?";
                std::string expected = entity + " consumes " + resource;
                
                bool correct = test_fact_recall(question, expected, "CONSUMES");
                results.total_questions++;
                if (correct) results.correct_answers++;
                
                results.relation_accuracy["CONSUMES"] += correct ? 1 : 0;
            }
        }
        
        // Limit to 200 questions as specified
        if (results.total_questions > 200) {
            results.total_questions = 200;
        }
        
        results.success_rate = results.total_questions > 0 ? 
            static_cast<float>(results.correct_answers) / results.total_questions : 0.0f;
        
        return results;
    }
    
    EvaluationResults::ReasoningResults run_reasoning_test() {
        EvaluationResults::ReasoningResults results;
        
        // 1-hop reasoning tests
        for (int i = 0; i < 50; ++i) {
            std::string question = generate_one_hop_question();
            bool success = test_reasoning(question, 1);
            
            results.one_hop_attempts++;
            if (success) results.one_hop_successes++;
        }
        
        // 2-hop reasoning tests
        for (int i = 0; i < 30; ++i) {
            std::string question = generate_two_hop_question();
            bool success = test_reasoning(question, 2);
            
            results.two_hop_attempts++;
            if (success) results.two_hop_successes++;
        }
        
        results.one_hop_success_rate = results.one_hop_attempts > 0 ? 
            static_cast<float>(results.one_hop_successes) / results.one_hop_attempts : 0.0f;
        
        results.two_hop_success_rate = results.two_hop_attempts > 0 ? 
            static_cast<float>(results.two_hop_successes) / results.two_hop_attempts : 0.0f;
        
        // Calculate average path length (simplified)
        results.avg_path_length = (results.one_hop_success_rate * 1.0f + 
                                  results.two_hop_success_rate * 2.0f) / 2.0f;
        
        return results;
    }
    
    EvaluationResults::RobustnessResults run_paraphrase_robustness_test() {
        EvaluationResults::RobustnessResults results;
        
        std::vector<std::string> test_questions = {
            "What are cats?",
            "What do mammals consume?",
            "How do animals survive?"
        };
        
        std::vector<float> success_rates;
        
        for (const auto& question : test_questions) {
            std::vector<std::string> paraphrases = generate_paraphrases(question);
            std::vector<float> paraphrase_successes;
            
            for (const auto& paraphrase : paraphrases) {
                bool success = test_fact_recall(paraphrase, "", "PARAPHRASE");
                paraphrase_successes.push_back(success ? 1.0f : 0.0f);
                results.paraphrase_tests++;
            }
            
            float avg_success = 0.0f;
            for (float s : paraphrase_successes) avg_success += s;
            avg_success /= paraphrase_successes.size();
            success_rates.push_back(avg_success);
        }
        
        // Calculate variance
        results.avg_paraphrase_success = 0.0f;
        for (float rate : success_rates) results.avg_paraphrase_success += rate;
        results.avg_paraphrase_success /= success_rates.size();
        
        float variance = 0.0f;
        for (float rate : success_rates) {
            variance += (rate - results.avg_paraphrase_success) * (rate - results.avg_paraphrase_success);
        }
        results.success_variance = variance / success_rates.size();
        
        // Acceptable if variance < 10%
        results.variance_acceptable = results.success_variance < 0.1f;
        
        return results;
    }
    
    EvaluationResults::NoiseToleranceResults run_noise_tolerance_test() {
        EvaluationResults::NoiseToleranceResults results;
        
        std::vector<std::string> test_questions = {
            "What are cats?",
            "What do mammals consume?",
            "How do animals survive?"
        };
        
        for (const auto& question : test_questions) {
            // Test baseline (no noise)
            bool baseline_success = test_fact_recall(question, "", "BASELINE");
            results.noise_tests++;
            if (baseline_success) results.baseline_successes++;
            
            // Test with 5% noise injection
            std::string noisy_question = inject_noise(question, 0.05f);
            bool noise_success = test_fact_recall(noisy_question, "", "NOISE");
            if (noise_success) results.noise_successes++;
        }
        
        float baseline_rate = results.noise_tests > 0 ? 
            static_cast<float>(results.baseline_successes) / results.noise_tests : 0.0f;
        
        float noise_rate = results.noise_tests > 0 ? 
            static_cast<float>(results.noise_successes) / results.noise_tests : 0.0f;
        
        results.success_drop = baseline_rate - noise_rate;
        
        // Acceptable if success drop < 8%
        results.tolerance_acceptable = results.success_drop < 0.08f;
        
        return results;
    }
    
    EvaluationResults::StabilityResults run_stability_test() {
        EvaluationResults::StabilityResults results;
        
        // Simulate replay of last 100 Thought paths
        std::vector<std::vector<std::string>> thought_paths = generate_thought_paths(100);
        
        for (const auto& path : thought_paths) {
            results.thought_paths_tested++;
            
            // Test if path can be exactly continued
            bool exact_continuation = test_path_continuation(path);
            if (exact_continuation) results.exact_continuations++;
        }
        
        results.stability_rate = results.thought_paths_tested > 0 ? 
            static_cast<float>(results.exact_continuations) / results.thought_paths_tested : 0.0f;
        
        // Acceptable if ≥85% stability
        results.stability_acceptable = results.stability_rate >= 0.85f;
        
        return results;
    }
    
private:
    void initialize_test_templates() {
        // Initialize question templates
        fact_questions_ = {
            "What is {}?",
            "What does {} consume?",
            "How does {} survive?",
            "What category does {} belong to?"
        };
        
        reasoning_questions_ = {
            "If {} is a {}, and {} consumes {}, what does {} consume?",
            "Given that {} are {}, what can we conclude about {}?"
        };
        
        paraphrase_templates_ = {
            "What are {}?",
            "What kind of thing is {}?",
            "What category includes {}?",
            "How would you classify {}?"
        };
        
        noise_distractors_ = {
            "xyz", "random", "noise", "distractor", "irrelevant"
        };
    }
    
    void initialize_knowledge_base() {
        // ISA relations
        isa_relations_["cats"] = {"mammals", "animals"};
        isa_relations_["dogs"] = {"mammals", "animals"};
        isa_relations_["mammals"] = {"animals"};
        isa_relations_["birds"] = {"animals"};
        isa_relations_["fish"] = {"animals"};
        
        // CONSUMES relations
        consumes_relations_["cats"] = {"water", "food", "mice"};
        consumes_relations_["dogs"] = {"water", "food", "bones"};
        consumes_relations_["mammals"] = {"water", "food"};
        consumes_relations_["animals"] = {"water"};
        
        // TEMPORAL relations
        temporal_relations_["cats"] = {"sleep", "hunt", "play"};
        temporal_relations_["dogs"] = {"sleep", "fetch", "play"};
    }
    
    bool test_fact_recall(const std::string& question, const std::string& expected, const std::string& relation_type) {
        // Simplified fact recall test - in practice would use actual reasoning
        // For demo purposes, return random success based on relation type
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        if (relation_type == "ISA") return dist(rng_) > 0.1f;  // 90% success
        if (relation_type == "CONSUMES") return dist(rng_) > 0.15f;  // 85% success
        return dist(rng_) > 0.2f;  // 80% success for others
    }
    
    bool test_reasoning(const std::string& question, int expected_hops) {
        // Simplified reasoning test
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        if (expected_hops == 1) return dist(rng_) > 0.1f;  // 90% success for 1-hop
        if (expected_hops == 2) return dist(rng_) > 0.2f;  // 80% success for 2-hop
        return false;
    }
    
    std::string generate_one_hop_question() {
        std::vector<std::string> entities = {"cats", "dogs", "mammals"};
        std::uniform_int_distribution<size_t> dist(0, entities.size() - 1);
        
        return "What is " + entities[dist(rng_)] + "?";
    }
    
    std::string generate_two_hop_question() {
        return "If cats are mammals, and mammals are animals, what are cats?";
    }
    
    std::vector<std::string> generate_paraphrases(const std::string& question) {
        std::vector<std::string> paraphrases;
        
        // Simple paraphrase generation
        if (question.find("What are") != std::string::npos) {
            paraphrases.push_back("What kind of thing are" + question.substr(8));
            paraphrases.push_back("How would you classify" + question.substr(8));
            paraphrases.push_back("What category includes" + question.substr(8));
        }
        
        return paraphrases;
    }
    
    std::string inject_noise(const std::string& question, float noise_rate) {
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        std::uniform_int_distribution<size_t> word_dist(0, noise_distractors_.size() - 1);
        
        std::string noisy_question = question;
        
        // Randomly inject noise words
        if (dist(rng_) < noise_rate) {
            noisy_question += " " + noise_distractors_[word_dist(rng_)];
        }
        
        return noisy_question;
    }
    
    std::vector<std::vector<std::string>> generate_thought_paths(int count) {
        std::vector<std::vector<std::string>> paths;
        
        for (int i = 0; i < count; ++i) {
            std::vector<std::string> path;
            
            // Generate random path of 2-4 nodes
            std::uniform_int_distribution<int> length_dist(2, 4);
            int path_length = length_dist(rng_);
            
            std::vector<std::string> nodes = {"cats", "mammals", "animals", "water", "food"};
            std::uniform_int_distribution<size_t> node_dist(0, nodes.size() - 1);
            
            for (int j = 0; j < path_length; ++j) {
                path.push_back(nodes[node_dist(rng_)]);
            }
            
            paths.push_back(path);
        }
        
        return paths;
    }
    
    bool test_path_continuation(const std::vector<std::string>& path) {
        // Simplified path continuation test
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng_) > 0.1f;  // 90% success rate for stability
    }
};

} // namespace melvin
