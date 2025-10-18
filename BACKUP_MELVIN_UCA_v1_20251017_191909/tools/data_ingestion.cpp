/*
 * Data Ingestion Implementation
 * 
 * ETL pipeline for converting external data to Melvin's graph format
 */

#include "data_ingestion.h"
#include <fstream>
#include <sstream>
#include <chrono>
#include <iostream>

namespace melvin {
namespace ingestion {

class DataIngestion::Impl {
public:
    // Helper: Split string by delimiter
    std::vector<std::string> split(const std::string& str, char delim) {
        std::vector<std::string> tokens;
        std::stringstream ss(str);
        std::string token;
        
        while (std::getline(ss, token, delim)) {
            // Trim whitespace
            size_t start = token.find_first_not_of(" \t\r\n");
            size_t end = token.find_last_not_of(" \t\r\n");
            
            if (start != std::string::npos && end != std::string::npos) {
                tokens.push_back(token.substr(start, end - start + 1));
            } else if (start != std::string::npos) {
                tokens.push_back(token.substr(start));
            }
        }
        
        return tokens;
    }
    
    // Helper: Remove quotes
    std::string unquote(const std::string& str) {
        if (str.length() >= 2 && str.front() == '"' && str.back() == '"') {
            return str.substr(1, str.length() - 2);
        }
        return str;
    }
};

DataIngestion::DataIngestion() : impl_(std::make_unique<Impl>()) {}
DataIngestion::~DataIngestion() = default;

std::vector<Fact> DataIngestion::parse_csv(const std::string& path) {
    std::vector<Fact> facts;
    std::ifstream file(path);
    
    if (!file.is_open()) {
        std::cerr << "✗ Failed to open CSV: " << path << std::endl;
        return facts;
    }
    
    std::string line;
    bool first_line = true;
    
    while (std::getline(file, line)) {
        // Skip header if it looks like one
        if (first_line && (line.find("subject") != std::string::npos || 
                          line.find("Subject") != std::string::npos)) {
            first_line = false;
            continue;
        }
        first_line = false;
        
        auto parts = impl_->split(line, ',');
        
        if (parts.size() >= 3) {
            Fact fact;
            fact.subject = impl_->unquote(parts[0]);
            fact.predicate = impl_->unquote(parts[1]);
            fact.object = impl_->unquote(parts[2]);
            
            if (parts.size() >= 4) {
                try {
                    fact.confidence = std::stof(parts[3]);
                } catch (...) {
                    fact.confidence = 1.0f;
                }
            }
            
            facts.push_back(fact);
        }
    }
    
    file.close();
    return facts;
}

std::vector<Fact> DataIngestion::parse_json(const std::string& path) {
    std::vector<Fact> facts;
    
    // TODO: Implement JSON parsing
    // For now, return empty (requires JSON library)
    
    std::cerr << "⚠️  JSON parsing not yet implemented\n";
    std::cerr << "   Use CSV format or plain text for now\n";
    
    (void)path;
    return facts;
}

std::vector<Fact> DataIngestion::parse_text(const std::string& path) {
    std::vector<Fact> facts;
    std::ifstream file(path);
    
    if (!file.is_open()) {
        std::cerr << "✗ Failed to open text file: " << path << std::endl;
        return facts;
    }
    
    std::string line;
    
    while (std::getline(file, line)) {
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#') continue;
        
        // Simple pattern: "X is Y" or "X has Y" or "X needs Y"
        auto parts = impl_->split(line, ' ');
        
        if (parts.size() >= 3) {
            Fact fact;
            fact.subject = parts[0];
            fact.predicate = parts[1];
            
            // Join remaining parts as object
            std::string obj;
            for (size_t i = 2; i < parts.size(); ++i) {
                if (!obj.empty()) obj += " ";
                obj += parts[i];
            }
            fact.object = obj;
            
            facts.push_back(fact);
        }
    }
    
    file.close();
    return facts;
}

std::vector<Fact> DataIngestion::load_huggingface(const std::string& dataset_name, int max_samples) {
    std::vector<Fact> facts;
    
    // TODO: Implement HuggingFace integration
    // For now, return empty
    
    std::cerr << "⚠️  HuggingFace integration not yet implemented\n";
    std::cerr << "   Use CSV or text files for now\n";
    
    (void)dataset_name;
    (void)max_samples;
    return facts;
}

std::vector<Fact> DataIngestion::query_sqlite(const std::string& db_path, const std::string& query) {
    std::vector<Fact> facts;
    
    // TODO: Implement SQLite integration
    // For now, return empty
    
    std::cerr << "⚠️  SQLite integration not yet implemented\n";
    std::cerr << "   Use CSV or text files for now\n";
    
    (void)db_path;
    (void)query;
    return facts;
}

int DataIngestion::import_facts(const std::vector<Fact>& facts, Storage* storage) {
    int created = 0;
    
    for (const auto& fact : facts) {
        // Create nodes
        NodeID subj_id = storage->create_node(fact.subject, NodeType::CONCEPT);
        NodeID pred_id = storage->create_node(fact.predicate, NodeType::CONCEPT);
        NodeID obj_id = storage->create_node(fact.object, NodeType::CONCEPT);
        
        // Create EXACT connections (taught facts are certain)
        storage->create_edge(subj_id, pred_id, RelationType::EXACT, 1.0f);
        storage->create_edge(pred_id, obj_id, RelationType::EXACT, 1.0f);
        
        created += 2;
    }
    
    return created;
}

IngestionStats DataIngestion::ingest(const SourceConfig& config, Storage* storage) {
    IngestionStats stats;
    auto start = std::chrono::high_resolution_clock::now();
    
    std::vector<Fact> facts;
    
    // Extract facts based on source type
    switch (config.type) {
        case SourceType::CSV_FILE:
            facts = parse_csv(config.path);
            break;
            
        case SourceType::JSON_FILE:
            facts = parse_json(config.path);
            break;
            
        case SourceType::TEXT_FILE:
            facts = parse_text(config.path);
            break;
            
        case SourceType::HUGGINGFACE:
            facts = load_huggingface(config.path, config.max_records);
            break;
            
        case SourceType::SQLITE_DB:
            facts = query_sqlite(config.path, config.format);
            break;
            
        default:
            std::cerr << "✗ Unsupported source type\n";
            return stats;
    }
    
    stats.facts_extracted = facts.size();
    stats.records_processed = facts.size();
    
    // Apply max_records limit
    if (config.max_records > 0 && facts.size() > (size_t)config.max_records) {
        facts.resize(config.max_records);
    }
    
    // Get counts before import
    size_t nodes_before = storage->node_count();
    size_t edges_before = storage->edge_count();
    
    // Import facts
    if (config.auto_connect) {
        stats.edges_created = import_facts(facts, storage);
    }
    
    // Get counts after import
    stats.nodes_created = storage->node_count() - nodes_before;
    
    auto end = std::chrono::high_resolution_clock::now();
    stats.duration_ms = std::chrono::duration<double, std::milli>(end - start).count();
    
    return stats;
}

} // namespace ingestion
} // namespace melvin

