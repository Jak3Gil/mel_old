#include "teaching_verify.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <iomanip>
#include <cmath>

// Full Node and Edge definitions
struct Node {
    uint64_t id = 0;
    std::string text;
    uint32_t type = 0;
    int freq = 0;
    bool pinned = false;
    std::vector<float> emb;
    std::vector<float> embedding;
    float attention_weight = 0.0f;
    uint64_t last_accessed = 0;
    float semantic_strength = 1.0f;
    float activation = 0.0f;
};

struct Edge {
    uint64_t u, v;
    uint64_t locB;
    float weight;
    float w_core;
    float w_ctx;
    uint32_t count;
    
    void update_frequency(uint64_t total_nodes) { (void)total_nodes; }
};

namespace melvin {
namespace teaching {

// Normalize answer for comparison
std::string TeachingVerifier::normalize_answer(const std::string& answer) {
    std::string result = answer;
    
    // Lowercase
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    // Trim
    size_t start = result.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = result.find_last_not_of(" \t\r\n");
    result = result.substr(start, end - start + 1);
    
    // Remove punctuation
    std::string cleaned;
    for (char c : result) {
        if (std::isalnum(c) || std::isspace(c)) {
            cleaned += c;
        }
    }
    
    return cleaned;
}

// Simple string similarity (approximates Jaro-Winkler)
float TeachingVerifier::string_similarity(const std::string& a, const std::string& b) {
    if (a.empty() && b.empty()) return 1.0f;
    if (a.empty() || b.empty()) return 0.0f;
    if (a == b) return 1.0f;
    
    // Simple edit distance ratio
    int longer = std::max(a.size(), b.size());
    int matching = 0;
    
    size_t min_len = std::min(a.size(), b.size());
    for (size_t i = 0; i < min_len; i++) {
        if (a[i] == b[i]) matching++;
    }
    
    // Check for substring
    if (a.find(b) != std::string::npos || b.find(a) != std::string::npos) {
        return 0.9f;
    }
    
    return static_cast<float>(matching) / static_cast<float>(longer);
}

// Check if actual matches any expected answer
bool TeachingVerifier::matches_any(
    const std::string& actual,
    const std::vector<std::string>& expects,
    float threshold
) {
    std::string norm_actual = normalize_answer(actual);
    
    for (const auto& expect : expects) {
        std::string norm_expect = normalize_answer(expect);
        
        // Exact match
        if (norm_actual == norm_expect) {
            return true;
        }
        
        // Fuzzy match
        float sim = string_similarity(norm_actual, norm_expect);
        if (sim >= threshold) {
            return true;
        }
        
        // Check if answer contains expected (for longer answers)
        if (norm_actual.find(norm_expect) != std::string::npos) {
            return true;
        }
    }
    
    return false;
}

// Simple graph-based query answering (traversal from query keywords)
std::string TeachingVerifier::answer_query(
    const std::string& question,
    std::unordered_map<uint64_t, ::Node>& nodes,
    std::vector<::Edge>& edges
) {
    std::string norm_question = normalize_answer(question);
    
    // Find nodes mentioned in the question
    std::vector<uint64_t> query_nodes;
    for (const auto& [id, node] : nodes) {
        std::string norm_node_text = normalize_answer(node.text);
        if (norm_question.find(norm_node_text) != std::string::npos && norm_node_text.size() > 2) {
            query_nodes.push_back(id);
        }
    }
    
    if (query_nodes.empty()) {
        return "(no relevant nodes found)";
    }
    
    // Traverse edges from query nodes to find answer
    std::unordered_map<uint64_t, float> scores;
    for (uint64_t start_id : query_nodes) {
        for (const auto& edge : edges) {
            if (edge.u == start_id) {
                scores[edge.v] += edge.weight;
            }
        }
    }
    
    // Find best scoring node
    uint64_t best_id = 0;
    float best_score = 0.0f;
    for (const auto& [id, score] : scores) {
        if (score > best_score) {
            best_score = score;
            best_id = id;
        }
    }
    
    if (best_id > 0 && nodes.find(best_id) != nodes.end()) {
        return nodes[best_id].text;
    }
    
    return "(no answer found)";
}

// Main verification function
VerifyResult TeachingVerifier::verify(
    const TchDoc& doc,
    std::unordered_map<uint64_t, ::Node>& nodes,
    std::vector<::Edge>& edges,
    const VerifyOptions& opts
) {
    VerifyResult result;
    
    if (opts.verbose) {
        std::cout << "\n╔═══════════════════════════════════════════════════════════╗" << std::endl;
        std::cout << "║           TEACHING VERIFICATION                           ║" << std::endl;
        std::cout << "╚═══════════════════════════════════════════════════════════╝\n" << std::endl;
    }
    
    // Find all QUERY blocks with EXPECT
    for (size_t i = 0; i < doc.blocks.size(); i++) {
        const auto& block = doc.blocks[i];
        
        if (block.type == BlockType::QUERY && block.query && !block.query->expects.empty()) {
            result.tests_total++;
            
            std::string question = block.query->question;
            std::string actual = answer_query(question, nodes, edges);
            
            // Check against expected answers
            bool passed = matches_any(actual, block.query->expects, opts.fuzzy_threshold);
            
            TestResult test;
            test.test_name = "Query_" + std::to_string(result.tests_total);
            test.question = question;
            test.expected = block.query->expects.empty() ? "" : block.query->expects[0];
            test.actual = actual;
            test.passed = passed;
            test.confidence = 0.8f;  // Placeholder
            
            // Compute best match score
            test.match_score = 0.0f;
            for (const auto& expect : block.query->expects) {
                float sim = string_similarity(normalize_answer(actual), normalize_answer(expect));
                test.match_score = std::max(test.match_score, sim);
            }
            
            result.results.push_back(test);
            
            if (passed) {
                result.tests_passed++;
            } else {
                result.tests_failed++;
            }
            
            result.avg_confidence += test.confidence;
            
            if (opts.verbose) {
                std::cout << (passed ? "✅" : "❌") << " "
                          << std::setw(40) << std::left << question.substr(0, 40)
                          << " | Expected: " << std::setw(15) << std::left << test.expected.substr(0, 15)
                          << " | Got: " << actual.substr(0, 20)
                          << " | Match: " << std::fixed << std::setprecision(2) << test.match_score
                          << std::endl;
            }
        }
    }
    
    // Compute statistics
    if (result.tests_total > 0) {
        result.pass_rate = static_cast<float>(result.tests_passed) / static_cast<float>(result.tests_total);
        result.avg_confidence /= static_cast<float>(result.tests_total);
    }
    
    result.success = result.pass_rate >= 0.5f;  // At least 50% pass rate
    
    if (opts.verbose) {
        std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━" << std::endl;
        std::cout << "VERIFICATION SUMMARY:" << std::endl;
        std::cout << "  Tests run: " << result.tests_total << std::endl;
        std::cout << "  Passed: " << result.tests_passed << " (" 
                  << std::fixed << std::setprecision(1) << (result.pass_rate * 100.0f) << "%)" << std::endl;
        std::cout << "  Failed: " << result.tests_failed << std::endl;
        std::cout << "  Status: " << (result.success ? "✅ PASS" : "❌ FAIL") << std::endl;
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n" << std::endl;
    }
    
    return result;
}

} // namespace teaching
} // namespace melvin

