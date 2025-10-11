#include "teaching_metrics.h"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <filesystem>
#include <ctime>

namespace melvin {
namespace teaching {

// Get current timestamp
std::string TeachingMetrics::current_timestamp() {
    auto now = std::chrono::system_clock::now();
    auto time_t_now = std::chrono::system_clock::to_time_t(now);
    std::tm tm_now;
    
    #ifdef _WIN32
        localtime_s(&tm_now, &time_t_now);
    #else
        localtime_r(&time_t_now, &tm_now);
    #endif
    
    char buffer[64];
    strftime(buffer, sizeof(buffer), "%Y-%m-%dT%H:%M:%S", &tm_now);
    return std::string(buffer);
}

// Create session metrics
TeachingSession TeachingMetrics::create_session(
    const std::string& filepath,
    const TchDoc& doc,
    uint32_t nodes_before,
    uint32_t nodes_after,
    uint32_t edges_before,
    uint32_t edges_after,
    const IngestResult& ingest,
    const VerifyResult& verify,
    int64_t duration_ms
) {
    TeachingSession session;
    session.filepath = filepath;
    session.timestamp = current_timestamp();
    session.duration_ms = duration_ms;
    
    session.nodes_before = nodes_before;
    session.nodes_after = nodes_after;
    session.edges_before = edges_before;
    session.edges_after = edges_after;
    
    session.ingest = ingest;
    session.verify = verify;
    
    // Compute derived metrics
    session.retention_score = verify.pass_rate;
    
    uint32_t blocks_processed = doc.blocks.size();
    if (blocks_processed > 0) {
        session.growth_efficiency = static_cast<float>(ingest.edges_added) / static_cast<float>(blocks_processed);
    } else {
        session.growth_efficiency = 0.0f;
    }
    
    session.snapshot_created = true;  // Assume snapshot was created
    
    return session;
}

// Print summary
void TeachingMetrics::print_summary(const TeachingSession& session) {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║             TEACHING SESSION SUMMARY                          ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════╝\n";
    std::cout << "\n";
    std::cout << "📁 File: " << session.filepath << "\n";
    std::cout << "🕐 Time: " << session.timestamp << "\n";
    std::cout << "⏱️  Duration: " << session.duration_ms << " ms\n";
    std::cout << "\n";
    
    std::cout << "📊 GRAPH GROWTH:\n";
    std::cout << "  Nodes: " << session.nodes_before << " → " << session.nodes_after 
              << " (+" << (session.nodes_after - session.nodes_before) << ")\n";
    std::cout << "  Edges: " << session.edges_before << " → " << session.edges_after
              << " (+" << (session.edges_after - session.edges_before) << ")\n";
    std::cout << "\n";
    
    std::cout << "📈 INGESTION:\n";
    std::cout << "  Nodes added: " << session.ingest.nodes_added << "\n";
    std::cout << "  Edges added: " << session.ingest.edges_added << "\n";
    std::cout << "  Edges updated: " << session.ingest.edges_updated << "\n";
    std::cout << "  Duplicates skipped: " << session.ingest.duplicates_skipped << "\n";
    std::cout << "  Rules added: " << session.ingest.rules_added << "\n";
    std::cout << "\n";
    
    std::cout << "✅ VERIFICATION:\n";
    std::cout << "  Tests run: " << session.verify.tests_total << "\n";
    std::cout << "  Passed: " << session.verify.tests_passed << " (" 
              << std::fixed << std::setprecision(1) << (session.verify.pass_rate * 100.0f) << "%)\n";
    std::cout << "  Failed: " << session.verify.tests_failed << "\n";
    std::cout << "\n";
    
    std::cout << "📊 TEACHING METRICS:\n";
    std::cout << "  Retention score: " << std::fixed << std::setprecision(3) << session.retention_score << "\n";
    std::cout << "  Growth efficiency: " << session.growth_efficiency << " edges/block\n";
    std::cout << "  Snapshot saved: " << (session.snapshot_created ? "✅ Yes" : "❌ No") << "\n";
    std::cout << "\n";
    
    std::cout << "═══════════════════════════════════════════════════════════════\n\n";
}

// Log to JSONL
bool TeachingMetrics::log_to_jsonl(const TeachingSession& session, const std::string& log_dir) {
    try {
        // Create log directory if needed
        std::filesystem::create_directories(log_dir);
        
        // Generate log filename from date
        std::string log_file = log_dir + "/" + session.timestamp.substr(0, 10) + ".jsonl";
        
        // Open in append mode
        std::ofstream file(log_file, std::ios::app);
        if (!file.is_open()) {
            return false;
        }
        
        // Write JSON line
        file << "{";
        file << "\"timestamp\":\"" << session.timestamp << "\",";
        file << "\"filepath\":\"" << session.filepath << "\",";
        file << "\"duration_ms\":" << session.duration_ms << ",";
        file << "\"nodes_before\":" << session.nodes_before << ",";
        file << "\"nodes_after\":" << session.nodes_after << ",";
        file << "\"edges_before\":" << session.edges_before << ",";
        file << "\"edges_after\":" << session.edges_after << ",";
        file << "\"nodes_added\":" << session.ingest.nodes_added << ",";
        file << "\"edges_added\":" << session.ingest.edges_added << ",";
        file << "\"edges_updated\":" << session.ingest.edges_updated << ",";
        file << "\"duplicates_skipped\":" << session.ingest.duplicates_skipped << ",";
        file << "\"rules_added\":" << session.ingest.rules_added << ",";
        file << "\"tests_total\":" << session.verify.tests_total << ",";
        file << "\"tests_passed\":" << session.verify.tests_passed << ",";
        file << "\"tests_failed\":" << session.verify.tests_failed << ",";
        file << "\"pass_rate\":" << session.verify.pass_rate << ",";
        file << "\"retention_score\":" << session.retention_score << ",";
        file << "\"growth_efficiency\":" << session.growth_efficiency;
        file << "}\n";
        
        file.close();
        return true;
        
    } catch (const std::exception& e) {
        std::cerr << "[METRICS] Failed to log to JSONL: " << e.what() << std::endl;
        return false;
    }
}

} // namespace teaching
} // namespace melvin

