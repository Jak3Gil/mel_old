/*
 * High-Throughput Batch Ingestion Server
 * 
 * HTTP endpoint for receiving batches of facts from streaming pipeline
 * Optimized for high-volume ingestion with periodic persistence
 */

#include "../core/storage.h"
#include "../core/melvin.h"
#include "data_ingestion.h"
#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <signal.h>

// Simple HTTP server (no external dependencies)
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>

using namespace melvin;

// ============================================================================
// GLOBAL STATE
// ============================================================================

static Storage* g_storage = nullptr;
static std::atomic<size_t> g_batches_processed{0};
static std::atomic<size_t> g_facts_ingested{0};
static std::atomic<size_t> g_nodes_created{0};
static std::atomic<size_t> g_edges_created{0};
static std::atomic<bool> g_running{true};

const size_t SAVE_INTERVAL = 100;  // Save every 100 batches
const int PORT = 5050;

// ============================================================================
// JSON PARSING (Lightweight)
// ============================================================================

struct Fact {
    std::string subject;
    std::string predicate;
    std::string object;
};

std::vector<Fact> parse_json_batch(const std::string& json_body) {
    std::vector<Fact> facts;
    
    // Simple JSON parser for our specific format
    // Format: {"facts":[{"subject":"a","predicate":"b","object":"c"},...]}
    
    size_t facts_pos = json_body.find("\"facts\"");
    if (facts_pos == std::string::npos) return facts;
    
    size_t array_start = json_body.find('[', facts_pos);
    if (array_start == std::string::npos) return facts;
    
    size_t pos = array_start + 1;
    while (pos < json_body.length()) {
        // Find next fact object
        size_t obj_start = json_body.find('{', pos);
        if (obj_start == std::string::npos) break;
        
        size_t obj_end = json_body.find('}', obj_start);
        if (obj_end == std::string::npos) break;
        
        std::string obj = json_body.substr(obj_start, obj_end - obj_start + 1);
        
        // Extract subject, predicate, object
        Fact fact;
        
        auto extract_value = [](const std::string& s, const std::string& key) -> std::string {
            size_t key_pos = s.find("\"" + key + "\"");
            if (key_pos == std::string::npos) return "";
            
            size_t val_start = s.find(':', key_pos);
            if (val_start == std::string::npos) return "";
            
            val_start = s.find('"', val_start);
            if (val_start == std::string::npos) return "";
            
            size_t val_end = s.find('"', val_start + 1);
            if (val_end == std::string::npos) return "";
            
            return s.substr(val_start + 1, val_end - val_start - 1);
        };
        
        fact.subject = extract_value(obj, "subject");
        fact.predicate = extract_value(obj, "predicate");
        fact.object = extract_value(obj, "object");
        
        if (!fact.subject.empty() && !fact.predicate.empty() && !fact.object.empty()) {
            facts.push_back(fact);
        }
        
        pos = obj_end + 1;
    }
    
    return facts;
}

// ============================================================================
// INGESTION HANDLER
// ============================================================================

void handle_batch_ingestion(const std::string& json_body) {
    if (!g_storage) return;
    
    auto facts = parse_json_batch(json_body);
    
    size_t nodes_before = g_storage->node_count();
    size_t edges_before = g_storage->edge_count();
    
    // Ingest facts
    for (const auto& fact : facts) {
        // Create nodes
        NodeID subject_id = g_storage->create_node(fact.subject);
        NodeID object_id = g_storage->create_node(fact.object);
        
        // Create edge
        g_storage->create_edge(subject_id, object_id, RelationType::EXACT, 1.0f);
    }
    
    size_t nodes_after = g_storage->node_count();
    size_t edges_after = g_storage->edge_count();
    
    // Update stats
    g_facts_ingested += facts.size();
    g_nodes_created += (nodes_after - nodes_before);
    g_edges_created += (edges_after - edges_before);
    g_batches_processed++;
    
    // Periodic save
    if (g_batches_processed % SAVE_INTERVAL == 0) {
        g_storage->save("data/nodes.melvin", "data/edges.melvin");
        
        std::cout << "\n💾 Auto-saved (batch #" << g_batches_processed << ")\n";
        std::cout << "   Nodes:       " << nodes_after << "\n";
        std::cout << "   Connections: " << edges_after << "\n\n";
    }
}

// ============================================================================
// HTTP SERVER (Simple)
// ============================================================================

std::string handle_http_request(const std::string& request) {
    // Parse HTTP request
    if (request.find("POST /ingest_batch") == 0) {
        // Find body (after double newline)
        size_t body_start = request.find("\r\n\r\n");
        if (body_start != std::string::npos) {
            std::string body = request.substr(body_start + 4);
            
            handle_batch_ingestion(body);
            
            return "HTTP/1.1 200 OK\r\n"
                   "Content-Type: application/json\r\n"
                   "Content-Length: 15\r\n"
                   "\r\n"
                   "{\"status\":\"ok\"}";
        }
    }
    else if (request.find("GET /stats") == 0) {
        // Return stats
        std::string stats = "{\"batches\":" + std::to_string(g_batches_processed.load()) +
                          ",\"facts\":" + std::to_string(g_facts_ingested.load()) +
                          ",\"nodes\":" + std::to_string(g_storage->node_count()) +
                          ",\"edges\":" + std::to_string(g_storage->edge_count()) + "}";
        
        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: application/json\r\n"
               "Content-Length: " + std::to_string(stats.length()) + "\r\n"
               "\r\n" + stats;
    }
    
    return "HTTP/1.1 404 Not Found\r\n\r\n";
}

void run_server(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "✗ Failed to create socket\n";
        return;
    }
    
    // Allow port reuse
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "✗ Failed to bind to port " << port << "\n";
        close(server_fd);
        return;
    }
    
    if (listen(server_fd, 10) < 0) {
        std::cerr << "✗ Failed to listen\n";
        close(server_fd);
        return;
    }
    
    std::cout << "✓ Server listening on port " << port << "\n\n";
    
    while (g_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            if (g_running) {
                std::cerr << "⚠️  Accept failed\n";
            }
            continue;
        }
        
        // Read request
        char buffer[65536];
        ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
        
        if (bytes_read > 0) {
            buffer[bytes_read] = '\0';
            std::string request(buffer);
            
            // Handle request
            std::string response = handle_http_request(request);
            
            // Send response
            write(client_fd, response.c_str(), response.length());
        }
        
        close(client_fd);
    }
    
    close(server_fd);
}

// ============================================================================
// STATS MONITOR
// ============================================================================

void run_stats_monitor() {
    auto start_time = std::chrono::steady_clock::now();
    
    while (g_running) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        
        if (!g_running) break;
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - start_time).count();
        
        if (elapsed > 0) {
            double fact_rate = static_cast<double>(g_facts_ingested) / elapsed;
            
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            std::cout << "  THROUGHPUT UPDATE (" << elapsed << "s)\n";
            std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
            std::cout << "  Batches:         " << g_batches_processed.load() << "\n";
            std::cout << "  Facts:           " << g_facts_ingested.load() << "\n";
            std::cout << "  Nodes created:   " << g_nodes_created.load() << "\n";
            std::cout << "  Edges created:   " << g_edges_created.load() << "\n";
            std::cout << "  Facts/sec:       " << fact_rate << "\n";
            std::cout << "  Facts/min:       " << (fact_rate * 60) << "\n\n";
            std::cout << "🧠 Brain State:\n";
            std::cout << "   Total nodes:    " << g_storage->node_count() << "\n";
            std::cout << "   Total edges:    " << g_storage->edge_count() << "\n\n";
        }
    }
}

// ============================================================================
// SIGNAL HANDLER
// ============================================================================

void signal_handler(int signum) {
    std::cout << "\n\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  SHUTDOWN SIGNAL RECEIVED\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    g_running = false;
    
    // Save before exit
    if (g_storage) {
        std::cout << "💾 Saving brain state...\n";
        g_storage->save("data/nodes.melvin", "data/edges.melvin");
        std::cout << "✓ Saved successfully\n\n";
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(int argc, char* argv[]) {
    std::cout << "\n╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN STREAMING INGESTION SERVER                    ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Initialize storage
    Storage storage;
    g_storage = &storage;
    
    std::cout << "📂 Loading existing brain...\n";
    if (storage.load("data/nodes.melvin", "data/edges.melvin")) {
        std::cout << "  ✓ Loaded: " << storage.node_count() << " nodes, " 
                  << storage.edge_count() << " edges\n\n";
    } else {
        std::cout << "  ℹ️  Starting with empty brain\n\n";
    }
    
    std::cout << "⚙️  Configuration:\n";
    std::cout << "  Port:               " << PORT << "\n";
    std::cout << "  Save interval:      Every " << SAVE_INTERVAL << " batches\n";
    std::cout << "  Endpoint:           POST /ingest_batch\n";
    std::cout << "  Stats endpoint:     GET /stats\n\n";
    
    // Start stats monitor thread
    std::thread stats_thread(run_stats_monitor);
    
    std::cout << "🚀 Starting server...\n";
    
    // Run server (blocking)
    run_server(PORT);
    
    // Cleanup
    std::cout << "🛑 Server stopped\n\n";
    
    g_running = false;
    if (stats_thread.joinable()) {
        stats_thread.join();
    }
    
    // Final save
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  FINAL STATISTICS\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    std::cout << "  Batches processed:  " << g_batches_processed.load() << "\n";
    std::cout << "  Facts ingested:     " << g_facts_ingested.load() << "\n";
    std::cout << "  New nodes:          " << g_nodes_created.load() << "\n";
    std::cout << "  New edges:          " << g_edges_created.load() << "\n\n";
    std::cout << "🧠 Final brain state:\n";
    std::cout << "   Total nodes:       " << storage.node_count() << "\n";
    std::cout << "   Total edges:       " << storage.edge_count() << "\n\n";
    
    std::cout << "💾 Saving final state...\n";
    if (storage.save("data/nodes.melvin", "data/edges.melvin")) {
        std::cout << "✓ Saved successfully\n\n";
    } else {
        std::cerr << "✗ Save failed!\n\n";
        return 1;
    }
    
    std::cout << "╔═══════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ SHUTDOWN COMPLETE                                 ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

