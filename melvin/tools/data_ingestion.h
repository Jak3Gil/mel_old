#pragma once

#include "../core/storage.h"
#include "../core/types.h"
#include <string>
#include <vector>
#include <memory>

namespace melvin {
namespace ingestion {

/**
 * Data Source Types
 */
enum class SourceType {
    JSON_FILE,          // Local JSON file
    CSV_FILE,           // CSV file
    TEXT_FILE,          // Plain text (parsed)
    HUGGINGFACE,        // HuggingFace dataset
    SQLITE_DB,          // SQLite database
    API_REST,           // REST API endpoint
    DIRECTORY           // Directory of files
};

/**
 * Fact Triple (subject-predicate-object)
 */
struct Fact {
    std::string subject;
    std::string predicate;
    std::string object;
    float confidence = 1.0f;  // Default: EXACT
};

/**
 * Data Source Configuration
 */
struct SourceConfig {
    SourceType type;
    std::string path;           // File path or URL
    std::string format;         // Format hint (e.g., "subject,predicate,object")
    bool auto_connect = true;   // Automatically create connections
    bool create_leaps = false;  // Run LEAP inference after import
    int max_records = -1;       // -1 = unlimited
};

/**
 * Ingestion Statistics
 */
struct IngestionStats {
    int records_processed = 0;
    int nodes_created = 0;
    int edges_created = 0;
    int facts_extracted = 0;
    int errors = 0;
    double duration_ms = 0.0;
};

/**
 * 📥 Data Ingestion Engine
 * 
 * Pulls data from various sources and converts to nodes/edges:
 * - Parse structured data (JSON, CSV, databases)
 * - Extract facts (subject-predicate-object triples)
 * - Create nodes and EXACT connections
 * - Optionally run LEAP inference
 */
class DataIngestion {
public:
    DataIngestion();
    ~DataIngestion();
    
    /**
     * Ingest from a data source
     */
    IngestionStats ingest(const SourceConfig& config, Storage* storage);
    
    /**
     * Parse CSV file (subject,predicate,object format)
     */
    std::vector<Fact> parse_csv(const std::string& path);
    
    /**
     * Parse JSON file (array of {subject, predicate, object})
     */
    std::vector<Fact> parse_json(const std::string& path);
    
    /**
     * Parse plain text (simple NLP extraction)
     */
    std::vector<Fact> parse_text(const std::string& path);
    
    /**
     * Load from HuggingFace dataset
     */
    std::vector<Fact> load_huggingface(const std::string& dataset_name, int max_samples = -1);
    
    /**
     * Query SQLite database
     */
    std::vector<Fact> query_sqlite(const std::string& db_path, const std::string& query);
    
    /**
     * Create nodes and edges from facts
     */
    int import_facts(const std::vector<Fact>& facts, Storage* storage);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ingestion
} // namespace melvin

