#pragma once

#include "teaching_format.h"
#include <unordered_map>
#include <vector>
#include <string>

// Forward declarations
struct Node;
struct Edge;

namespace melvin {
namespace teaching {

// Verification options
struct VerifyOptions {
    bool verbose = true;      // Print each test result
    float fuzzy_threshold = 0.85f;  // Jaro-Winkler threshold for matching
    bool strict_mode = false;  // Require exact matches
};

// Single test result
struct TestResult {
    std::string test_name;
    std::string question;
    std::string expected;
    std::string actual;
    bool passed;
    float confidence;
    float match_score;  // Similarity to expected
};

// Verification result
struct VerifyResult {
    bool success = true;
    int tests_total = 0;
    int tests_passed = 0;
    int tests_failed = 0;
    float pass_rate = 0.0f;
    float avg_confidence = 0.0f;
    std::vector<TestResult> results;
};

// Teaching verifier
class TeachingVerifier {
public:
    // Verify a teaching document (run all QUERY+EXPECT pairs)
    static VerifyResult verify(
        const TchDoc& doc,
        std::unordered_map<uint64_t, ::Node>& nodes,
        std::vector<::Edge>& edges,
        const VerifyOptions& opts = VerifyOptions()
    );
    
private:
    // Helper: Run a query and get answer
    static std::string answer_query(
        const std::string& question,
        std::unordered_map<uint64_t, ::Node>& nodes,
        std::vector<::Edge>& edges
    );
    
    // Helper: Check if actual matches any expected
    static bool matches_any(
        const std::string& actual,
        const std::vector<std::string>& expects,
        float threshold
    );
    
    // Helper: Compute string similarity (simple Jaro-Winkler approximation)
    static float string_similarity(const std::string& a, const std::string& b);
    
    // Helper: Normalize answer for comparison
    static std::string normalize_answer(const std::string& answer);
};

} // namespace teaching
} // namespace melvin

