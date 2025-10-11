#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <optional>

namespace melvin {
namespace teaching {

// Metadata attached to teaching blocks
struct Meta {
    std::string file;
    int line = 0;
    float confidence = 1.0f;
    std::string temporal;
    std::string source;
    std::string explain;
};

// Teaching block types
enum class BlockType {
    FACT,
    ASSOCIATION,
    RULE,
    QUERY,
    EXPECT,
    TEST,
    EXPLAIN,
    SOURCE,
    WEIGHT,
    UNKNOWN
};

// Parsed fact: SUBJ REL OBJ
struct TchFact {
    std::string subj;
    std::string rel;
    std::string obj;
    Meta meta;
};

// Parsed association: LEFT [REL] RIGHT (optionally bidirectional)
struct TchAssoc {
    std::string left;
    std::string rel;  // May be empty for generic ASSOC
    std::string right;
    bool bidirectional = false;
    Meta meta;
};

// Parsed rule: IF pattern THEN implication
struct TchRule {
    std::string pattern;
    std::string implies;
    Meta meta;
};

// Parsed query with expected answers
struct TchQuery {
    std::string question;
    std::vector<std::string> expects;  // Multiple acceptable answers
    Meta meta;
};

// Named test case
struct TchTest {
    std::string name;
    TchQuery query;
};

// Generic teaching block
struct TchBlock {
    BlockType type;
    std::string raw_content;
    Meta meta;
    
    // Parsed content (only one set based on type)
    std::optional<TchFact> fact;
    std::optional<TchAssoc> assoc;
    std::optional<TchRule> rule;
    std::optional<TchQuery> query;
    std::optional<TchTest> test;
};

// Complete teaching document
struct TchDoc {
    std::string filepath;
    std::vector<TchBlock> blocks;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
};

// Parsing options
struct ParseOptions {
    bool strict = false;  // Fail on first error vs accumulate
    bool normalize = true;  // Normalize text (lowercase, trim)
    bool verbose = false;  // Print parse progress
};

// Parse result
struct ParseResult {
    bool success;
    TchDoc doc;
    int lines_processed;
    int blocks_parsed;
};

// Main parser interface
class TeachingParser {
public:
    // Parse a .tch file
    static ParseResult parse_file(const std::string& filepath, const ParseOptions& opts = ParseOptions());
    
    // Parse text content directly
    static ParseResult parse_string(const std::string& content, const std::string& filepath, const ParseOptions& opts = ParseOptions());
    
    // Validate a parsed document
    static bool validate(TchDoc& doc);
    
private:
    // Helper: Determine block type from tag
    static BlockType parse_tag(const std::string& line);
    
    // Helper: Parse relation from text (SUBJ REL OBJ)
    static bool parse_relation(const std::string& text, std::string& subj, std::string& rel, std::string& obj);
    
    // Helper: Parse association (may have arrows)
    static bool parse_association(const std::string& text, std::string& left, std::string& rel, std::string& right, bool& bidirectional);
    
    // Helper: Parse rule (IF ... THEN ...)
    static bool parse_rule(const std::string& text, std::string& pattern, std::string& implies);
    
    // Helper: Normalize text (lowercase, trim, etc.)
    static std::string normalize_text(const std::string& text);
    
    // Helper: Parse metadata weight hint
    static void parse_weight_hint(const std::string& text, Meta& meta);
};

// Utility functions
std::string block_type_to_string(BlockType type);
bool is_bidirectional_marker(const std::string& text);  // Contains ↔ or <->

} // namespace teaching
} // namespace melvin

