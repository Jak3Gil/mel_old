/*
 * Melvin Ingest - Data Import Tool
 * 
 * Pull data from external sources and convert to Melvin's graph:
 * - CSV files (subject,predicate,object)
 * - JSON files (structured data)
 * - Text files (NLP parsing)
 * - Databases (SQLite, etc.)
 * - APIs (HuggingFace, etc.)
 * 
 * Automatically creates nodes and connections!
 */

#include "data_ingestion.h"
#include "../core/leap_inference.h"
#include <iostream>
#include <iomanip>

using namespace melvin;
using namespace melvin::ingestion;

void print_usage() {
    std::cout << "\nUsage: melvin_ingest [options] <source>\n\n";
    std::cout << "Options:\n";
    std::cout << "  -t, --type TYPE       Source type (csv, json, text, hf, db)\n";
    std::cout << "  -f, --format FORMAT   Data format hint\n";
    std::cout << "  -m, --max N           Max records to import\n";
    std::cout << "  -l, --leap            Create LEAP connections after import\n";
    std::cout << "  -o, --output PATH     Output path (default: data/nodes.melvin)\n";
    std::cout << "  -h, --help            Show this help\n\n";
    std::cout << "Examples:\n";
    std::cout << "  melvin_ingest -t csv knowledge.csv\n";
    std::cout << "  melvin_ingest -t csv -m 1000 facts.csv\n";
    std::cout << "  melvin_ingest -t text -l documents.txt\n";
    std::cout << "  melvin_ingest -t hf --max 500 squad\n\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }
    
    // Parse arguments
    SourceConfig config;
    config.type = SourceType::CSV_FILE;  // Default
    config.auto_connect = true;
    config.create_leaps = false;
    config.max_records = -1;
    
    std::string output_nodes = "data/nodes.melvin";
    std::string output_edges = "data/edges.melvin";
    
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage();
            return 0;
        } else if (arg == "-t" || arg == "--type") {
            if (i + 1 < argc) {
                std::string type = argv[++i];
                if (type == "csv") config.type = SourceType::CSV_FILE;
                else if (type == "json") config.type = SourceType::JSON_FILE;
                else if (type == "text") config.type = SourceType::TEXT_FILE;
                else if (type == "hf") config.type = SourceType::HUGGINGFACE;
                else if (type == "db") config.type = SourceType::SQLITE_DB;
                else {
                    std::cerr << "✗ Unknown type: " << type << std::endl;
                    return 1;
                }
            }
        } else if (arg == "-f" || arg == "--format") {
            if (i + 1 < argc) config.format = argv[++i];
        } else if (arg == "-m" || arg == "--max") {
            if (i + 1 < argc) config.max_records = std::atoi(argv[++i]);
        } else if (arg == "-l" || arg == "--leap") {
            config.create_leaps = true;
        } else if (arg == "-o" || arg == "--output") {
            if (i + 1 < argc) output_nodes = argv[++i];
        } else if (arg[0] != '-') {
            config.path = arg;
        }
    }
    
    if (config.path.empty()) {
        std::cerr << "✗ No source path specified\n";
        print_usage();
        return 1;
    }
    
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN DATA INGESTION                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "⚙️  Configuration:\n";
    std::cout << "  Source:             " << config.path << "\n";
    
    std::string type_name;
    switch (config.type) {
        case SourceType::CSV_FILE: type_name = "CSV"; break;
        case SourceType::JSON_FILE: type_name = "JSON"; break;
        case SourceType::TEXT_FILE: type_name = "Text"; break;
        case SourceType::HUGGINGFACE: type_name = "HuggingFace"; break;
        case SourceType::SQLITE_DB: type_name = "SQLite"; break;
        default: type_name = "Unknown";
    }
    
    std::cout << "  Type:               " << type_name << "\n";
    std::cout << "  Max records:        " << (config.max_records > 0 ? std::to_string(config.max_records) : "unlimited") << "\n";
    std::cout << "  Create LEAPs:       " << (config.create_leaps ? "yes" : "no") << "\n";
    std::cout << "  Output:             " << output_nodes << "\n\n";
    
    // Load existing brain (or start fresh)
    Storage storage;
    
    std::cout << "📂 Loading existing brain...\n";
    if (storage.load(output_nodes, output_edges)) {
        std::cout << "  ✓ Loaded: " << storage.node_count() << " nodes, " 
                  << storage.edge_count() << " edges\n";
    } else {
        std::cout << "  ℹ️  Starting with empty brain\n";
    }
    
    size_t nodes_before = storage.node_count();
    size_t edges_before = storage.edge_count();
    
    std::cout << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  INGESTING DATA\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    // Run ingestion
    DataIngestion ingester;
    IngestionStats stats = ingester.ingest(config, &storage);
    
    std::cout << "\n📊 Ingestion Statistics:\n";
    std::cout << "  Records processed:  " << stats.records_processed << "\n";
    std::cout << "  Facts extracted:    " << stats.facts_extracted << "\n";
    std::cout << "  Nodes created:      " << stats.nodes_created << "\n";
    std::cout << "  Edges created:      " << stats.edges_created << "\n";
    std::cout << "  Duration:           " << std::fixed << std::setprecision(1) 
              << stats.duration_ms << " ms\n\n";
    
    // Create LEAP connections if requested
    if (config.create_leaps && stats.edges_created > 0) {
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        std::cout << "  CREATING LEAP CONNECTIONS\n";
        std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        
        leap::LEAPInference::Config leap_config;
        leap_config.verbose = true;
        leap::LEAPInference leap_engine(leap_config);
        
        int leaps_created = leap_engine.create_leap_connections(&storage);
        
        std::cout << "  ✓ Created " << leaps_created << " LEAP connections\n\n";
    }
    
    // Save to disk
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  SAVING TO DISK\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    if (storage.save(output_nodes, output_edges)) {
        std::cout << "  ✓ Saved successfully\n\n";
    } else {
        std::cerr << "  ✗ Failed to save!\n\n";
        return 1;
    }
    
    // Final state
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  FINAL STATE\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    std::cout << "  Before:\n";
    std::cout << "    Nodes:            " << nodes_before << "\n";
    std::cout << "    Edges:            " << edges_before << "\n\n";
    
    std::cout << "  After:\n";
    std::cout << "    Nodes:            " << storage.node_count() 
              << " (+" << (storage.node_count() - nodes_before) << ")\n";
    std::cout << "    Edges:            " << storage.edge_count() 
              << " (+" << (storage.edge_count() - edges_before) << ")\n\n";
    
    size_t exact_count = storage.edge_count_by_type(RelationType::EXACT);
    size_t leap_count = storage.edge_count_by_type(RelationType::LEAP);
    
    std::cout << "  Connection types:\n";
    std::cout << "    EXACT:            " << exact_count << "\n";
    std::cout << "    LEAP:             " << leap_count << "\n\n";
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ INGESTION COMPLETE                                ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    std::cout << "💡 Next steps:\n";
    std::cout << "  make stats          # Check brain state\n";
    std::cout << "  make test           # Validate reasoning\n";
    std::cout << "  make growth         # Track learning\n\n";
    
    return 0;
}

