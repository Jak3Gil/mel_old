#include "teaching_format.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>
#include <iostream>

namespace melvin {
namespace teaching {

// Utility: Convert block type to string
std::string block_type_to_string(BlockType type) {
    switch (type) {
        case BlockType::FACT: return "FACT";
        case BlockType::ASSOCIATION: return "ASSOCIATION";
        case BlockType::RULE: return "RULE";
        case BlockType::QUERY: return "QUERY";
        case BlockType::EXPECT: return "EXPECT";
        case BlockType::TEST: return "TEST";
        case BlockType::EXPLAIN: return "EXPLAIN";
        case BlockType::SOURCE: return "SOURCE";
        case BlockType::WEIGHT: return "WEIGHT";
        default: return "UNKNOWN";
    }
}

// Utility: Check for bidirectional marker
bool is_bidirectional_marker(const std::string& text) {
    return text.find("↔") != std::string::npos || text.find("<->") != std::string::npos;
}

// Utility: Trim whitespace
std::string trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

// Utility: Lowercase
std::string to_lower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

// Utility: Remove punctuation
std::string remove_punctuation(const std::string& str) {
    std::string result;
    for (char c : str) {
        if (std::isalnum(c) || std::isspace(c)) {
            result += c;
        }
    }
    return result;
}

// Normalize text
std::string TeachingParser::normalize_text(const std::string& text) {
    std::string s = trim(text);
    s = remove_punctuation(s);
    s = to_lower(s);
    return s;
}

// Determine block type from tag line
BlockType TeachingParser::parse_tag(const std::string& line) {
    std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed[0] != '#') {
        return BlockType::UNKNOWN;
    }
    
    std::string tag = to_lower(trimmed.substr(1));
    tag = trim(tag);
    
    // Split on whitespace for TEST cases (e.g., "#TEST CatsMilk")
    size_t space_pos = tag.find(' ');
    if (space_pos != std::string::npos) {
        tag = tag.substr(0, space_pos);
    }
    
    if (tag == "fact") return BlockType::FACT;
    if (tag == "association" || tag == "assoc") return BlockType::ASSOCIATION;
    if (tag == "rule") return BlockType::RULE;
    if (tag == "query" || tag == "q") return BlockType::QUERY;
    if (tag == "expect" || tag == "expected") return BlockType::EXPECT;
    if (tag == "test") return BlockType::TEST;
    if (tag == "explain" || tag == "explanation") return BlockType::EXPLAIN;
    if (tag == "source" || tag == "src") return BlockType::SOURCE;
    if (tag == "weight" || tag == "meta") return BlockType::WEIGHT;
    
    return BlockType::UNKNOWN;
}

// Parse relation: SUBJ REL OBJ
bool TeachingParser::parse_relation(const std::string& text, std::string& subj, std::string& rel, std::string& obj) {
    std::string normalized = trim(text);
    
    // Remove arrows if present (normalize)
    size_t arrow_pos = normalized.find("→");
    if (arrow_pos == std::string::npos) {
        arrow_pos = normalized.find("->");
    }
    if (arrow_pos != std::string::npos) {
        normalized.erase(arrow_pos, (normalized[arrow_pos] == '-' ? 2 : 1));
    }
    
    // Split into tokens
    std::vector<std::string> tokens;
    std::istringstream iss(normalized);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    
    if (tokens.size() < 3) {
        return false;  // Need at least SUBJ REL OBJ
    }
    
    // Simple heuristic: first token is SUBJ, last is OBJ, middle is REL
    subj = tokens[0];
    obj = tokens.back();
    
    // Everything in between is the relation
    rel = "";
    for (size_t i = 1; i < tokens.size() - 1; i++) {
        if (i > 1) rel += " ";
        rel += tokens[i];
    }
    
    return true;
}

// Parse association: LEFT [REL] RIGHT
bool TeachingParser::parse_association(const std::string& text, std::string& left, std::string& rel, std::string& right, bool& bidirectional) {
    bidirectional = is_bidirectional_marker(text);
    
    // For now, reuse parse_relation logic
    // In the future, could add special handling for ↔ marker position
    return parse_relation(text, left, rel, right);
}

// Parse rule: IF pattern THEN implication
bool TeachingParser::parse_rule(const std::string& text, std::string& pattern, std::string& implies) {
    std::string normalized = to_lower(trim(text));
    
    size_t if_pos = normalized.find("if ");
    size_t then_pos = normalized.find(" then ");
    
    if (if_pos == std::string::npos || then_pos == std::string::npos) {
        return false;
    }
    
    pattern = trim(text.substr(if_pos + 3, then_pos - (if_pos + 3)));
    implies = trim(text.substr(then_pos + 6));
    
    return !pattern.empty() && !implies.empty();
}

// Parse weight/meta hint
void TeachingParser::parse_weight_hint(const std::string& text, Meta& meta) {
    // Simple key:value extraction
    // Format: {confidence:0.9, temporal:"2025-10-11", source:"wiki"}
    
    size_t conf_pos = text.find("confidence:");
    if (conf_pos != std::string::npos) {
        std::string val_str;
        for (size_t i = conf_pos + 11; i < text.size(); i++) {
            if (std::isdigit(text[i]) || text[i] == '.') {
                val_str += text[i];
            } else if (!val_str.empty()) {
                break;
            }
        }
        if (!val_str.empty()) {
            meta.confidence = std::stof(val_str);
        }
    }
    
    size_t temp_pos = text.find("temporal:");
    if (temp_pos != std::string::npos) {
        size_t quote_start = text.find('"', temp_pos);
        if (quote_start != std::string::npos) {
            size_t quote_end = text.find('"', quote_start + 1);
            if (quote_end != std::string::npos) {
                meta.temporal = text.substr(quote_start + 1, quote_end - quote_start - 1);
            }
        }
    }
}

// Main parsing function
ParseResult TeachingParser::parse_string(const std::string& content, const std::string& filepath, const ParseOptions& opts) {
    ParseResult result;
    result.success = true;
    result.doc.filepath = filepath;
    result.lines_processed = 0;
    result.blocks_parsed = 0;
    
    std::istringstream stream(content);
    std::string line;
    int line_num = 0;
    
    BlockType current_block_type = BlockType::UNKNOWN;
    std::vector<std::string> current_payload;
    Meta current_meta;
    int block_start_line = 0;
    
    auto flush_block = [&]() {
        if (current_block_type == BlockType::UNKNOWN || current_payload.empty()) {
            return;
        }
        
        TchBlock block;
        block.type = current_block_type;
        block.meta = current_meta;
        block.meta.file = filepath;
        block.meta.line = block_start_line;
        
        // Join payload
        std::string payload_text;
        for (const auto& p : current_payload) {
            if (!payload_text.empty()) payload_text += "\n";
            payload_text += p;
        }
        block.raw_content = payload_text;
        
        // Parse based on type
        bool parse_ok = true;
        
        switch (current_block_type) {
            case BlockType::FACT: {
                TchFact fact;
                fact.meta = block.meta;
                if (parse_relation(payload_text, fact.subj, fact.rel, fact.obj)) {
                    block.fact = fact;
                } else {
                    result.doc.errors.push_back("Line " + std::to_string(block_start_line) + 
                                               ": Failed to parse FACT relation");
                    parse_ok = false;
                }
                break;
            }
            
            case BlockType::ASSOCIATION: {
                TchAssoc assoc;
                assoc.meta = block.meta;
                if (parse_association(payload_text, assoc.left, assoc.rel, assoc.right, assoc.bidirectional)) {
                    block.assoc = assoc;
                } else {
                    result.doc.errors.push_back("Line " + std::to_string(block_start_line) + 
                                               ": Failed to parse ASSOCIATION");
                    parse_ok = false;
                }
                break;
            }
            
            case BlockType::RULE: {
                TchRule rule;
                rule.meta = block.meta;
                if (parse_rule(payload_text, rule.pattern, rule.implies)) {
                    block.rule = rule;
                } else {
                    result.doc.errors.push_back("Line " + std::to_string(block_start_line) + 
                                               ": Failed to parse RULE (need IF...THEN...)");
                    parse_ok = false;
                }
                break;
            }
            
            case BlockType::QUERY: {
                TchQuery query;
                query.question = trim(payload_text);
                query.meta = block.meta;
                block.query = query;
                break;
            }
            
            case BlockType::EXPECT: {
                // Attach to previous query block if present
                // For now, store as separate block
                // Will be merged in validation
                break;
            }
            
            case BlockType::WEIGHT: {
                parse_weight_hint(payload_text, current_meta);
                // Don't create a block for WEIGHT, just update meta
                return;
            }
            
            default:
                result.doc.warnings.push_back("Line " + std::to_string(block_start_line) + 
                                             ": Unknown or unhandled block type");
                break;
        }
        
        if (parse_ok) {
            result.doc.blocks.push_back(block);
            result.blocks_parsed++;
        }
        
        // Reset for next block
        current_payload.clear();
        current_meta = Meta();
        current_meta.file = filepath;
    };
    
    while (std::getline(stream, line)) {
        line_num++;
        result.lines_processed++;
        
        std::string trimmed = trim(line);
        
        // Skip empty lines and comments
        if (trimmed.empty() || (trimmed[0] == '#' && trimmed.size() > 1 && trimmed[1] == ' ')) {
            continue;
        }
        
        // Check if this is a tag line
        if (trimmed[0] == '#' && trimmed.size() > 1) {
            // Flush previous block
            flush_block();
            
            // Start new block
            current_block_type = parse_tag(trimmed);
            block_start_line = line_num;
            current_meta.line = line_num;
            
            // For TEST, extract name
            if (current_block_type == BlockType::TEST && trimmed.size() > 5) {
                std::string name_part = trim(trimmed.substr(5));
                if (!name_part.empty()) {
                    current_meta.explain = name_part;  // Store test name in explain
                }
            }
            
            continue;
        }
        
        // Add to current payload
        if (current_block_type != BlockType::UNKNOWN) {
            current_payload.push_back(trimmed);
        }
    }
    
    // Flush final block
    flush_block();
    
    // Validate
    validate(result.doc);
    
    if (!result.doc.errors.empty() && opts.strict) {
        result.success = false;
    }
    
    if (opts.verbose) {
        std::cout << "[PARSE] " << filepath << ": " << result.blocks_parsed << " blocks, "
                  << result.doc.errors.size() << " errors, "
                  << result.doc.warnings.size() << " warnings" << std::endl;
    }
    
    return result;
}

// Parse file
ParseResult TeachingParser::parse_file(const std::string& filepath, const ParseOptions& opts) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        ParseResult result;
        result.success = false;
        result.doc.filepath = filepath;
        result.doc.errors.push_back("Failed to open file: " + filepath);
        return result;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    return parse_string(buffer.str(), filepath, opts);
}

// Validate document
bool TeachingParser::validate(TchDoc& doc) {
    bool valid = true;
    
    // Check for QUERY without EXPECT
    for (size_t i = 0; i < doc.blocks.size(); i++) {
        if (doc.blocks[i].type == BlockType::QUERY) {
            // Look for EXPECT in next block
            bool has_expect = false;
            if (i + 1 < doc.blocks.size()) {
                if (doc.blocks[i + 1].type == BlockType::EXPECT) {
                    // Merge EXPECT into QUERY
                    has_expect = true;
                    std::string expect_text = doc.blocks[i + 1].raw_content;
                    
                    // Split by comma for multiple acceptable answers
                    std::istringstream iss(expect_text);
                    std::string answer;
                    while (std::getline(iss, answer, ',')) {
                        answer = trim(answer);
                        if (!answer.empty()) {
                            if (doc.blocks[i].query) {
                                doc.blocks[i].query->expects.push_back(answer);
                            }
                        }
                    }
                }
            }
            
            if (!has_expect) {
                doc.warnings.push_back("Line " + std::to_string(doc.blocks[i].meta.line) + 
                                      ": QUERY without EXPECT");
            }
        }
    }
    
    // Check for TEST blocks
    for (auto& block : doc.blocks) {
        if (block.type == BlockType::TEST) {
            // TEST should be followed by QUERY + EXPECT
            // For now, just log that we found a test
        }
    }
    
    return valid && doc.errors.empty();
}

} // namespace teaching
} // namespace melvin

