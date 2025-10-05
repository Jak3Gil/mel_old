#include "melvin.h"
#include <iostream>
#include <vector>
#include <string>
#include <unistd.h>

void run_experiment(const std::string& name, melvin_t* melvin, const std::vector<std::string>& teachings, const std::string& query, const std::string& expected) {
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "🧪 EXPERIMENT: " << name << "\n";
    std::cout << std::string(60, '=') << "\n";
    
    // Teach the knowledge
    std::cout << "📚 Teaching:\n";
    for (const auto& teaching : teachings) {
        std::cout << "  " << teaching << "\n";
        melvin_learn(melvin, teaching.c_str());
    }
    
    // Test with beam search
    std::cout << "\n🔍 Testing with beam search:\n";
    const char* result_beam = melvin_generate_phrase_beam_c(melvin, "cats", 5, 3, 4, "balanced", 0, 0.08f, "", query.c_str(), 1);
    std::cout << "Beam result: \"" << result_beam << "\"\n";
    
    // Test with greedy
    std::cout << "\n🎯 Testing with greedy:\n";
    const char* result_greedy = melvin_generate_phrase_c(melvin, "cats", 5, "balanced", "deterministic", "", query.c_str(), 0);
    std::cout << "Greedy result: \"" << result_greedy << "\"\n";
    
    // Show debug info
    std::cout << "\n🔍 Debug info for 'cats':\n";
    melvin_debug_node_with_context(melvin, "cats", "", query.c_str(), "balanced");
    
    std::cout << "\nExpected: " << expected << "\n";
    std::cout << std::string(60, '=') << "\n";
}

int main() {
    std::cout << "🚀 MELVIN BEAM SEARCH STRESS TEST\n";
    std::cout << "==================================\n";
    
    melvin_t* melvin = melvin_create(nullptr);
    
    // Experiment 1: Baseline equals
    run_experiment("Baseline equals", melvin, 
        {"cats are mammals", "cats are mammals", "cats are mammals", "cats are mammals", 
         "cats are mammals", "cats are mammals", "cats are mammals", "cats are mammals"},
        "what is a cat", "mammal");
    
    // Experiment 2: Competing categories
    run_experiment("Competing categories", melvin,
        {"cats are animals", "cats are animals", "cats are animals"},
        "what is a cat", "mammal (should prefer over animal)");
    
    // Experiment 3: Attribute shouldn't win equals
    run_experiment("Attribute vs category", melvin,
        {"cats are cute", "cats are cute", "cats are cute", "cats are cute", "cats are cute"},
        "what is a cat", "mammal (attribute should be suppressed)");
    
    // Experiment 4: Paraphrase + lemma
    run_experiment("Paraphrase variations", melvin,
        {},
        "what are cats", "mammals (pluralize properly)");
    
    run_experiment("Reverse question", melvin,
        {},
        "cats are what", "mammals (same target)");
    
    // Experiment 5: Thin data regime
    melvin_t* melvin_thin = melvin_create(nullptr);
    run_experiment("Thin data regime", melvin_thin,
        {"cats are mammals"},
        "what is a cat", "mammal (should still work with Laplace smoothing)");
    
    // Experiment 6: Transient recency
    run_experiment("Recent vs established", melvin,
        {"cats are predators", "cats are predators"},
        "what is a cat", "predators (recent bias)");
    
    // Wait a moment to test recency decay
    std::cout << "\n⏰ Waiting 2 seconds to test recency decay...\n";
    sleep(2);
    
    run_experiment("After recency decay", melvin,
        {},
        "what is a cat", "mammals (should return to established facts)");
    
    // Experiment 7: Noise resistance
    melvin_t* melvin_noise = melvin_create(nullptr);
    melvin_learn(melvin_noise, "cats are mammals");
    melvin_learn(melvin_noise, "cats are mammals");
    melvin_learn(melvin_noise, "cats are mammals");
    melvin_learn(melvin_noise, "cats are mammals");
    melvin_learn(melvin_noise, "cats are mammals");
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "🧪 EXPERIMENT: Noise resistance\n";
    std::cout << std::string(60, '=') << "\n";
    
    std::cout << "📚 Adding noise (50 random bigrams):\n";
    std::vector<std::string> noise = {
        "random words", "unrelated stuff", "gibberish text", "meaningless data", "junk information",
        "random words", "unrelated stuff", "gibberish text", "meaningless data", "junk information",
        "random words", "unrelated stuff", "gibberish text", "meaningless data", "junk information",
        "random words", "unrelated stuff", "gibberish text", "meaningless data", "junk information",
        "random words", "unrelated stuff", "gibberish text", "meaningless data", "junk information",
        "random words", "unrelated stuff", "gibberish text", "meaningless data", "junk information",
        "random words", "unrelated stuff", "gibberish text", "meaningless data", "junk information",
        "random words", "unrelated stuff", "gibberish text", "meaningless data", "junk information",
        "random words", "unrelated stuff", "gibberish text", "meaningless data", "junk information",
        "random words", "unrelated stuff", "gibberish text", "meaningless data", "junk information"
    };
    
    for (const auto& n : noise) {
        melvin_learn(melvin_noise, n.c_str());
    }
    
    std::cout << "\n🔍 Testing stability after noise:\n";
    const char* result_noise = melvin_generate_phrase_beam_c(melvin_noise, "cats", 4, 3, 4, "balanced", 0, 0.08f, "", "what is a cat", 0);
    std::cout << "Result after noise: \"" << result_noise << "\"\n";
    std::cout << "Expected: mammals (should remain stable)\n";
    
    // Experiment 8: Bridge via middle token
    melvin_t* melvin_bridge = melvin_create(nullptr);
    run_experiment("Bridge via middle token", melvin_bridge,
        {"cats have claws", "mammals have hair"},
        "what do mammals have", "hair (copula vs verb constraints)");
    
    run_experiment("Bridge cats to claws", melvin_bridge,
        {},
        "what do cats have", "claws (bridge via have)");
    
    // Experiment 9: Loop guard
    melvin_t* melvin_loop = melvin_create(nullptr);
    melvin_learn(melvin_loop, "cats are mammals");
    melvin_learn(melvin_loop, "cats are mammals");
    melvin_learn(melvin_loop, "cats are mammals");
    
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "🧪 EXPERIMENT: Loop guard\n";
    std::cout << std::string(60, '=') << "\n";
    
    std::cout << "🔍 Testing loop prevention (generation mode):\n";
    const char* result_loop = melvin_generate_phrase_beam_c(melvin_loop, "cats", 10, 3, 4, "balanced", 0, 0.08f, "", "", 1);
    std::cout << "Loop test result: \"" << result_loop << "\"\n";
    std::cout << "Expected: No 'cats cats cats...' repetition\n";
    
    // Experiment 10: Out-of-domain unknown
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "🧪 EXPERIMENT: Out-of-domain unknown\n";
    std::cout << std::string(60, '=') << "\n";
    
    std::cout << "🔍 Testing unknown word 'zorb':\n";
    const char* result_unknown = melvin_generate_phrase_beam_c(melvin, "zorb", 5, 3, 4, "balanced", 0, 0.08f, "", "what is a zorb", 1);
    std::cout << "Unknown word result: \"" << result_unknown << "\"\n";
    std::cout << "Expected: Empty or 'I don't know yet' (score < threshold)\n";
    
    // Summary
    std::cout << "\n" << std::string(60, '=') << "\n";
    std::cout << "📊 STRESS TEST SUMMARY\n";
    std::cout << std::string(60, '=') << "\n";
    std::cout << "✅ All experiments completed\n";
    std::cout << "🔍 Check results above for:\n";
    std::cout << "  - Category preference over attributes\n";
    std::cout << "  - Recency effects and decay\n";
    std::cout << "  - Noise resistance\n";
    std::cout << "  - Loop prevention\n";
    std::cout << "  - Graceful handling of unknown words\n";
    std::cout << "  - Beam search vs greedy comparison\n";
    
    // Cleanup
    melvin_destroy(melvin);
    melvin_destroy(melvin_thin);
    melvin_destroy(melvin_noise);
    melvin_destroy(melvin_bridge);
    melvin_destroy(melvin_loop);
    
    return 0;
}
