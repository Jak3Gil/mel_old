/*
 * MELVIN TEMPLATE FUZZ TESTER
 * 
 * Mini fuzzer/regression tester for LEAP system:
 * - Seeds corpus with known patterns
 * - Fuzzes queries with/without coverage
 * - Tests threshold variations
 * - Asserts expected behaviors
 */

#include "../include/melvin_core.h"
#include "../src/util/telemetry.h"
#include "../src/util/config.h"
#include "../src/util/explain.h"
#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <cstring>

using namespace melvin_core;
using namespace melvin_telemetry;
using namespace melvin_config;
using namespace melvin_explain;

// ==================== TEST CORPUS ====================

struct Fact {
    std::string text;
    std::string subject;
    std::string connector;
    std::string object;
};

std::vector<Fact> get_default_corpus() {
    return {
        {"dogs are mammals", "dogs", "are", "mammals"},
        {"cats are mammals", "cats", "are", "mammals"},
        {"wolves are mammals", "wolves", "are", "mammals"},
        {"lions are mammals", "lions", "are", "mammals"},
        {"tigers are predators", "tigers", "are", "predators"},
        {"sharks are predators", "sharks", "are", "predators"},
        {"eagles are predators", "eagles", "are", "predators"},
        {"snakes are scary", "snakes", "are", "scary"},
        {"spiders are scary", "spiders", "are", "scary"},
        {"dogs have fur", "dogs", "have", "fur"},
        {"cats have fur", "cats", "have", "fur"},
        {"bears have fur", "bears", "have", "fur"},
        {"birds can fly", "birds", "can", "fly"},
        {"planes can fly", "planes", "can", "fly"},
        {"helicopters can fly", "helicopters", "can", "fly"}
    };
}

// ==================== FUZZ TESTS ====================

struct FuzzResult {
    std::string query;
    bool expected_leap;
    bool expected_abstain;
    bool actual_leap_created;
    bool actual_abstained;
    bool passed;
    std::string reason;
};

class FuzzTester {
private:
    UnifiedBrain* brain_;
    std::mt19937 rng_;
    std::vector<FuzzResult> results_;
    bool explain_;
    
public:
    FuzzTester(UnifiedBrain* brain, bool explain = false) 
        : brain_(brain), rng_(std::random_device{}()), explain_(explain) {}
    
    // Test: With crowd ≥ TH_SUPPORT & TH_DIVERSITY ⇒ leap created
    void test_crowd_sufficient() {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "TEST: Crowd Sufficient → LEAP Created\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // Query: "are foxes mammals" - should create LEAP (4 examples, 4 subjects)
        std::string query = "are foxes mammals";
        
        int leaps_before = count_leaps();
        int creates_before = count_events("leap_create");
        
        brain_->think(query);
        
        int leaps_after = count_leaps();
        int creates_after = count_events("leap_create");
        
        FuzzResult result;
        result.query = query;
        result.expected_leap = true;
        result.expected_abstain = false;
        result.actual_leap_created = (creates_after > creates_before);
        result.actual_abstained = false;
        result.passed = result.actual_leap_created;
        result.reason = result.passed ? "LEAP created as expected" : "LEAP not created";
        
        results_.push_back(result);
        
        std::cout << "   Query: \"" << query << "\"\n";
        std::cout << "   Expected: LEAP created\n";
        std::cout << "   Actual: " << (result.actual_leap_created ? "LEAP created" : "No LEAP") << "\n";
        std::cout << "   Result: " << (result.passed ? "✅ PASS" : "❌ FAIL") << "\n";
    }
    
    // Test: Insufficient diversity ⇒ no leap
    void test_insufficient_diversity() {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "TEST: Insufficient Diversity → No LEAP\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // Only one example: "apples are red"
        system("rm -f fuzz_test.bin");
        UnifiedBrain test_brain("fuzz_test.bin");
        
        test_brain.learn("apples are red");
        
        int rejects_before = count_events("leap_reject");
        test_brain.think("are oranges red");
        int rejects_after = count_events("leap_reject");
        
        FuzzResult result;
        result.query = "are oranges red";
        result.expected_leap = false;
        result.expected_abstain = false;
        result.actual_leap_created = false;
        result.actual_abstained = (rejects_after > rejects_before);
        result.passed = (rejects_after > rejects_before);
        result.reason = result.passed ? "Correctly rejected" : "Should have rejected";
        
        results_.push_back(result);
        
        std::cout << "   Query: \"" << result.query << "\"\n";
        std::cout << "   Expected: No LEAP (insufficient diversity)\n";
        std::cout << "   Actual: " << (result.actual_abstained ? "Rejected" : "LEAP created") << "\n";
        std::cout << "   Result: " << (result.passed ? "✅ PASS" : "❌ FAIL") << "\n";
        
        test_brain.save();
    }
    
    // Test: Competing Ys not separated by margin ⇒ abstain
    void test_ambiguity_margin() {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "TEST: Ambiguous Margin → Abstain\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        // Create ambiguous situation
        system("rm -f fuzz_test.bin");
        UnifiedBrain test_brain("fuzz_test.bin");
        
        test_brain.learn("dogs are fluffy");
        test_brain.learn("cats are fluffy");
        test_brain.learn("snakes are scary");
        test_brain.learn("spiders are scary");
        
        int rejects_before = count_events("leap_reject");
        test_brain.think("are wolves fluffy");  // Could be fluffy OR scary
        int rejects_after = count_events("leap_reject");
        
        // With TH_MARGIN=1.0, fluffy (support=2) and scary (support=2) don't separate
        
        FuzzResult result;
        result.query = "are wolves fluffy";
        result.expected_leap = false;
        result.expected_abstain = true;
        result.actual_leap_created = false;
        result.actual_abstained = (rejects_after > rejects_before);
        result.passed = result.actual_abstained;
        result.reason = result.passed ? "Correctly abstained" : "Should have abstained";
        
        results_.push_back(result);
        
        std::cout << "   Query: \"" << result.query << "\"\n";
        std::cout << "   Expected: Abstain (margin not met)\n";
        std::cout << "   Actual: " << (result.actual_abstained ? "Abstained" : "Answered") << "\n";
        std::cout << "   Result: " << (result.passed ? "✅ PASS" : "❌ FAIL") << "\n";
        
        test_brain.save();
    }
    
    // Random fuzz iterations
    void fuzz_random(int iters) {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "FUZZ: Random Queries (" << iters << " iterations)\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        std::vector<std::string> subjects = {"rabbits", "frogs", "fish", "birds", "insects"};
        std::vector<std::string> connectors = {"are", "have", "can"};
        std::vector<std::string> objects = {"mammals", "predators", "scary", "fur", "fly"};
        
        std::uniform_int_distribution<> subj_dist(0, subjects.size() - 1);
        std::uniform_int_distribution<> conn_dist(0, connectors.size() - 1);
        std::uniform_int_distribution<> obj_dist(0, objects.size() - 1);
        
        int crashes = 0;
        for (int i = 0; i < iters; ++i) {
            std::string query = connectors[conn_dist(rng_)] + " " +
                               subjects[subj_dist(rng_)] + " " +
                               objects[obj_dist(rng_)];
            
            try {
                brain_->think(query);
            } catch (...) {
                crashes++;
                std::cout << "   CRASH on query: \"" << query << "\"\n";
            }
            
            if ((i + 1) % 50 == 0) {
                std::cout << "   Completed " << (i + 1) << "/" << iters << " iterations\n";
            }
        }
        
        std::cout << "   Total iterations: " << iters << "\n";
        std::cout << "   Crashes: " << crashes << "\n";
        std::cout << "   Result: " << (crashes == 0 ? "✅ PASS" : "❌ FAIL") << "\n";
    }
    
    void print_summary() {
        std::cout << "\n╔══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║  FUZZ TEST SUMMARY                                           ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
        
        int passed = 0;
        for (const auto& result : results_) {
            if (result.passed) passed++;
            std::cout << "  " << (result.passed ? "✅" : "❌") << " " << result.query << "\n";
        }
        
        std::cout << "\n  Total: " << results_.size() << " tests\n";
        std::cout << "  Passed: " << passed << "\n";
        std::cout << "  Failed: " << (results_.size() - passed) << "\n\n";
    }
    
private:
    int count_leaps() {
        int count = 0;
        for (const auto& edge : melvin_storage::edges) {
            if (edge.is_leap()) count++;
        }
        return count;
    }
};

// ==================== MAIN ====================

int main(int argc, char* argv[]) {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN TEMPLATE FUZZ TESTER                                 ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
    
    // Parse arguments
    int iters = 200;
    bool explain = false;
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--iters" && i + 1 < argc) {
            iters = std::atoi(argv[++i]);
        } else if (arg == "--explain") {
            explain = true;
            EXPLAIN_ENABLE(true);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "\nUsage: fuzz_templates [OPTIONS]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --iters N      Number of random fuzz iterations (default: 200)\n";
            std::cout << "  --explain      Enable explain tracing\n";
            std::cout << "  --help, -h     Show this help\n\n";
            std::cout << "Environment Variables:\n";
            std::cout << "  TH_SUPPORT, TH_DIVERSITY, TH_MARGIN  (see config.h)\n\n";
            std::cout << "Examples:\n";
            std::cout << "  ./fuzz_templates --iters 200 --explain\n";
            std::cout << "  TH_SUPPORT=4 TH_MARGIN=1.5 ./fuzz_templates --iters 500\n\n";
            return 0;
        }
    }
    
    // Load configuration
    load_config();
    
    // Enable telemetry
    enable_telemetry(true);
    set_telemetry_file("fuzz_telemetry.jsonl");
    
    // Setup brain with corpus
    system("rm -f fuzz_test.bin");
    UnifiedBrain brain("fuzz_test.bin");
    
    std::cout << "\n📚 Loading corpus...\n";
    auto corpus = get_default_corpus();
    for (const auto& fact : corpus) {
        brain.learn(fact.text);
    }
    std::cout << "   Loaded " << corpus.size() << " facts\n";
    
    brain.stats();
    
    // Run tests
    FuzzTester tester(&brain, explain);
    
    tester.test_crowd_sufficient();
    tester.test_insufficient_diversity();
    tester.test_ambiguity_margin();
    
    if (iters > 0) {
        tester.fuzz_random(iters);
    }
    
    tester.print_summary();
    
    // Print telemetry summary
    std::cout << "📊 Telemetry Events:\n";
    std::cout << "  leap_create:  " << count_events("leap_create") << "\n";
    std::cout << "  leap_reject:  " << count_events("leap_reject") << "\n";
    std::cout << "  leap_promote: " << count_events("leap_promote") << "\n";
    std::cout << "  leap_decay:   " << count_events("leap_decay") << "\n\n";
    
    brain.save();
    
    return 0;
}

