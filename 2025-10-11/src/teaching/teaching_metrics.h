#pragma once

#include "teaching_format.h"
#include "teaching_ingest.h"
#include "teaching_verify.h"
#include <string>
#include <chrono>

namespace melvin {
namespace teaching {

// Complete teaching session metrics
struct TeachingSession {
    std::string filepath;
    std::string timestamp;
    int64_t duration_ms;
    
    // Ingestion metrics
    uint32_t nodes_before;
    uint32_t nodes_after;
    uint32_t edges_before;
    uint32_t edges_after;
    IngestResult ingest;
    
    // Verification metrics
    VerifyResult verify;
    
    // Computed metrics
    float retention_score;      // Pass rate
    float growth_efficiency;    // edges_added / blocks_processed
    bool snapshot_created;
};

// Teaching metrics aggregator
class TeachingMetrics {
public:
    // Create metrics from a teaching session
    static TeachingSession create_session(
        const std::string& filepath,
        const TchDoc& doc,
        uint32_t nodes_before,
        uint32_t nodes_after,
        uint32_t edges_before,
        uint32_t edges_after,
        const IngestResult& ingest,
        const VerifyResult& verify,
        int64_t duration_ms
    );
    
    // Print session summary to console
    static void print_summary(const TeachingSession& session);
    
    // Export session to JSONL log
    static bool log_to_jsonl(const TeachingSession& session, const std::string& log_dir = "logs/teaching");
    
    // Get current timestamp (ISO 8601)
    static std::string current_timestamp();
};

} // namespace teaching
} // namespace melvin

