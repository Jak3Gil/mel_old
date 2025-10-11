/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN + OLLAMA CONTINUOUS LEARNING - PROPERLY INTEGRATED                ║
 * ║  Uses Melvin's existing Store, Nodes, Edges, and persistence layer       ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 * 
 * This integrates Ollama self-scoring with Melvin's existing system:
 * - Uses melvin::Store for persistence (nodes.melvin, edges.melvin)
 * - Uses melvin::Node and melvin::Edge structures
 * - All nodes persist across runs automatically
 * - Adds Ollama scoring layer on top of existing reasoning
 */

#include "melvin.h"
#include "storage.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <chrono>
#include <thread>
#include <csignal>
#include <curl/curl.h>

namespace fs = std::filesystem;

// Global flag for graceful shutdown
std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
    std::cout << "\n\n🛑 Received signal " << signal << " - shutting down gracefully...\n";
    keep_running = false;
}

// ==================== SIMPLE JSON PARSING ====================

namespace SimpleJSON {
    std::string extract_response(const std::string& json_str) {
        size_t start = json_str.find("\"response\":\"");
        if (start == std::string::npos) return "";
        start += 12;
        
        size_t end = start;
        while (end < json_str.length()) {
            if (json_str[end] == '\"' && (end == 0 || json_str[end-1] != '\\')) {
                break;
            }
            end++;
        }
        
        if (end > start) {
            return json_str.substr(start, end - start);
        }
        return "";
    }
    
    std::string create_generate_request(const std::string& model, const std::string& prompt) {
        std::string json = "{\"model\":\"";
        json += model;
        json += "\",\"prompt\":\"";
        
        for (char c : prompt) {
            if (c == '\"') json += "\\\"";
            else if (c == '\\') json += "\\\\";
            else if (c == '\n') json += "\\n";
            else if (c == '\r') json += "\\r";
            else if (c == '\t') json += "\\t";
            else json += c;
        }
        
        json += "\",\"stream\":false}";
        return json;
    }
}

// ==================== OLLAMA CLIENT ====================

class OllamaClient {
private:
    std::string base_url_;
    std::string model_name_;
    CURL* curl_;
    
    static size_t write_callback(void* contents, size_t size, size_t nmemb, void* userp) {
        ((std::string*)userp)->append((char*)contents, size * nmemb);
        return size * nmemb;
    }
    
public:
    OllamaClient(const std::string& base_url = "http://localhost:11434", 
                 const std::string& model = "llama3.2")
        : base_url_(base_url), model_name_(model), curl_(nullptr) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        curl_ = curl_easy_init();
    }
    
    ~OllamaClient() {
        if (curl_) {
            curl_easy_cleanup(curl_);
        }
        curl_global_cleanup();
    }
    
    bool is_available() {
        if (!curl_) return false;
        
        std::string response;
        curl_easy_setopt(curl_, CURLOPT_URL, (base_url_ + "/api/tags").c_str());
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 5L);
        
        CURLcode res = curl_easy_perform(curl_);
        return (res == CURLE_OK);
    }
    
    std::string generate(const std::string& prompt) {
        if (!curl_) return "";
        
        std::string json_request = SimpleJSON::create_generate_request(model_name_, prompt);
        
        std::string response_data;
        curl_easy_setopt(curl_, CURLOPT_URL, (base_url_ + "/api/generate").c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, json_request.c_str());
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_data);
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 30L);
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
        
        CURLcode res = curl_easy_perform(curl_);
        curl_slist_free_all(headers);
        
        if (res != CURLE_OK) {
            return "";
        }
        
        return SimpleJSON::extract_response(response_data);
    }
    
    float score_answer(const std::string& question, const std::string& answer) {
        std::string prompt = "Rate the quality of this answer on a scale of 0 to 10:\n\n";
        prompt += "Question: " + question + "\n";
        prompt += "Answer: " + answer + "\n\n";
        prompt += "Provide ONLY a number from 0 to 10, nothing else:";
        
        std::string response = generate(prompt);
        
        try {
            std::string cleaned;
            for (char c : response) {
                if (std::isdigit(c) || c == '.') {
                    cleaned += c;
                } else if (!cleaned.empty()) {
                    break;
                }
            }
            
            if (!cleaned.empty()) {
                float score = std::stof(cleaned);
                return std::min(1.0f, score / 10.0f);
            }
        } catch (...) {
        }
        
        return 0.5f;
    }
};

// ==================== MELVIN OLLAMA INTEGRATION ====================

class MelvinOllamaIntegrated {
private:
    melvin_t* melvin_;  // Main Melvin instance with Store (using C API)
    std::unique_ptr<OllamaClient> ollama_;
    
    std::unordered_set<std::string> processed_files_;
    int cycle_count_ = 0;
    int generation_ = 0;
    float average_ollama_score_ = 0.5f;
    size_t total_ollama_queries_ = 0;
    
public:
    MelvinOllamaIntegrated(const std::string& store_dir) {
        // Create Melvin with persistent storage (C API)
        melvin_ = melvin_create(store_dir.c_str());
        
        std::cout << "🧠 Melvin initialized with store: " << store_dir << "\n";
        std::cout << "   Existing nodes: " << melvin_node_count(melvin_) << "\n";
        std::cout << "   Existing edges: " << melvin_edge_count(melvin_) << "\n";
        
        // Initialize Ollama
        std::cout << "🤖 Initializing Ollama connection...\n";
        ollama_ = std::make_unique<OllamaClient>();
        
        if (ollama_->is_available()) {
            std::cout << "✅ Ollama is available and ready!\n\n";
        } else {
            std::cout << "⚠️  Warning: Ollama not available. Self-scoring will be simulated.\n\n";
        }
        
        // Load processed files list
        load_processed_files();
    }
    
    ~MelvinOllamaIntegrated() {
        save_processed_files();
        
        std::cout << "\n💾 Saving Melvin's state...\n";
        std::cout << "   Final nodes: " << melvin_node_count(melvin_) << "\n";
        std::cout << "   Final edges: " << melvin_edge_count(melvin_) << "\n";
        
        // Melvin's destroy automatically saves to disk via Store
        melvin_destroy(melvin_);
    }
    
    void run_continuous(const std::string& watch_dir, int test_interval = 5, 
                       int ollama_interval = 10, int save_interval = 25) {
        std::cout << "🔄 STARTING MELVIN OLLAMA CONTINUOUS LEARNING\n";
        std::cout << "==============================================\n\n";
        std::cout << "Using Melvin's existing infrastructure:\n";
        std::cout << "  ✅ melvin::Store for persistence\n";
        std::cout << "  ✅ melvin::Node and melvin::Edge\n";
        std::cout << "  ✅ All nodes persist across runs\n\n";
        std::cout << "Watching: " << watch_dir << "\n";
        std::cout << "Test every: " << test_interval << " cycles\n";
        std::cout << "Ollama scoring every: " << ollama_interval << " cycles\n";
        std::cout << "Press Ctrl+C to stop gracefully\n\n";
        
        auto last_scan = std::chrono::steady_clock::now();
        
        while (keep_running) {
            cycle_count_++;
            auto now = std::chrono::steady_clock::now();
            
            size_t node_count = melvin_node_count(melvin_);
            size_t edge_count = melvin_edge_count(melvin_);
            
            std::cout << "🔄 Cycle " << cycle_count_ << " [Gen " << generation_ 
                     << "] - " << node_count << " nodes, " << edge_count << " edges\n";
            
            // STEP 1: Scan for new data
            if (std::chrono::duration_cast<std::chrono::seconds>(now - last_scan).count() >= 3) {
                scan_and_ingest(watch_dir);
                last_scan = now;
            }
            
            // STEP 2: Test knowledge periodically
            if (cycle_count_ % test_interval == 0) {
                test_current_knowledge();
            }
            
            // STEP 3: Self-score with Ollama
            if (cycle_count_ % ollama_interval == 0 && ollama_->is_available()) {
                self_score_with_ollama();
            }
            
            // STEP 4: Run Melvin's decay pass (cleans up weak edges)
            if (cycle_count_ % 20 == 0) {
                std::cout << "   🧹 Running Melvin's decay pass...\n";
                melvin_decay_pass(melvin_);
            }
            
            // STEP 5: Evolve
            if (cycle_count_ % 10 == 0) {
                evolve();
            }
            
            // STEP 6: Display stats
            if (cycle_count_ % 5 == 0) {
                print_stats();
            }
            
            // Brief pause
            std::this_thread::sleep_for(std::chrono::milliseconds(800));
        }
        
        std::cout << "\n📊 FINAL STATISTICS:\n";
        print_stats();
        std::cout << "\n✅ Daemon shutdown complete\n";
    }
    
private:
    void scan_and_ingest(const std::string& dir_path) {
        try {
            if (!fs::exists(dir_path)) {
                fs::create_directories(dir_path);
                return;
            }
            
            for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
                if (!entry.is_regular_file()) continue;
                
                std::string path = entry.path().string();
                
                if (processed_files_.find(path) != processed_files_.end()) continue;
                
                std::string ext = entry.path().extension().string();
                if (ext == ".o" || ext == ".bin" || ext == ".dat" || ext == ".exe" ||
                    ext == ".so" || ext == ".dylib" || ext == ".a" || ext == ".melvin") continue;
                
                std::cout << "   📄 NEW FILE: " << entry.path().filename() << "\n";
                ingest_file(path);
                processed_files_.insert(path);
            }
        } catch (const std::exception& e) {
            std::cerr << "   ⚠️  Scan error: " << e.what() << "\n";
        }
    }
    
    void ingest_file(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file) return;
        
        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        
        size_t nodes_before = melvin_node_count(melvin_);
        size_t edges_before = melvin_edge_count(melvin_);
        
        // Use Melvin's existing learn function!
        melvin_learn(melvin_, content.c_str());
        
        size_t nodes_after = melvin_node_count(melvin_);
        size_t edges_after = melvin_edge_count(melvin_);
        
        std::cout << "      ✅ +" << (nodes_after - nodes_before) << " nodes, "
                 << "+" << (edges_after - edges_before) << " edges\n";
    }
    
    void test_current_knowledge() {
        std::cout << "   🧪 Testing knowledge with Melvin's reasoning...\n";
        
        // Test some sample questions
        const char* test_questions[] = {
            "What is a cat?",
            "What is a dog?",
            "What is water?",
            "What is energy?",
            "What is a computer?"
        };
        
        int correct = 0;
        int total = 0;
        
        for (const char* q : test_questions) {
            // Use Melvin's reasoning engine!
            const char* answer = melvin_reason(melvin_, q);
            if (answer && strlen(answer) > 0 && strcmp(answer, "unknown") != 0) {
                correct++;
            }
            total++;
        }
        
        float accuracy = total > 0 ? (float)correct / total : 0.0f;
        std::cout << "      📊 Melvin answered " << correct << "/" << total 
                 << " questions (" << (int)(accuracy * 100) << "%)\n";
        
        log_metrics(accuracy);
    }
    
    void self_score_with_ollama() {
        std::cout << "   🤖 Self-scoring with Ollama...\n";
        
        const char* test_questions[] = {
            "What is a cat?",
            "What is water?",
            "What is energy?"
        };
        
        float total_score = 0.0f;
        int scored_count = 0;
        
        for (const char* q : test_questions) {
            // Get Melvin's answer
            const char* answer = melvin_reason(melvin_, q);
            
            if (!answer || strlen(answer) == 0) continue;
            
            // Get Ollama to score it
            float score = ollama_->score_answer(q, answer);
            
            total_ollama_queries_++;
            total_score += score;
            scored_count++;
            
            std::cout << "      Q: " << q << "\n";
            std::cout << "      A: " << answer << "\n";
            std::cout << "      🎯 Ollama score: " << std::fixed << std::setprecision(2) 
                     << (score * 10) << "/10\n";
        }
        
        if (scored_count > 0) {
            average_ollama_score_ = total_score / scored_count;
            std::cout << "      📊 Average Ollama score: " << std::fixed << std::setprecision(2)
                     << (average_ollama_score_ * 10) << "/10\n";
        }
    }
    
    void evolve() {
        std::cout << "   🧬 Evolving...\n";
        
        // Trigger Melvin's own evolution if needed
        // For now, just increment generation
        generation_++;
        
        std::cout << "      ✓ Generation: " << generation_ << "\n";
        std::cout << "      ✓ Avg Ollama score: " << (average_ollama_score_ * 10) << "/10\n";
    }
    
    void print_stats() const {
        size_t nodes = melvin_node_count(melvin_);
        size_t edges = melvin_edge_count(melvin_);
        
        std::cout << "\n   📊 STATS:\n";
        std::cout << "      Nodes: " << nodes << " (persisted to disk)\n";
        std::cout << "      Edges: " << edges << " (persisted to disk)\n";
        std::cout << "      Generation: " << generation_ << "\n";
        std::cout << "      Ollama queries: " << total_ollama_queries_ << "\n";
        std::cout << "      Avg Ollama score: " << std::fixed << std::setprecision(2)
                 << (average_ollama_score_ * 10) << "/10\n\n";
    }
    
    void log_metrics(float accuracy) {
        static bool header_written = false;
        std::ofstream log("melvin_integrated_metrics.csv", std::ios::app);
        
        if (!header_written) {
            log << "timestamp,cycle,generation,nodes,edges,accuracy,ollama_score\n";
            header_written = true;
        }
        
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        log << now << "," << cycle_count_ << "," << generation_ << "," 
            << melvin_node_count(melvin_) << "," << melvin_edge_count(melvin_) << "," 
            << accuracy << "," << average_ollama_score_ << "\n";
        log.flush();
    }
    
    void load_processed_files() {
        std::ifstream file("melvin_integrated_processed.txt");
        std::string path;
        while (std::getline(file, path)) {
            processed_files_.insert(path);
        }
    }
    
    void save_processed_files() {
        std::ofstream file("melvin_integrated_processed.txt");
        for (const auto& path : processed_files_) {
            file << path << "\n";
        }
    }
};

// ==================== MAIN ====================

int main(int argc, char* argv[]) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "🧠 MELVIN + OLLAMA CONTINUOUS LEARNING (INTEGRATED)\n";
    std::cout << "===================================================\n\n";
    std::cout << "This version properly integrates with Melvin:\n";
    std::cout << "  ✅ Uses melvin::Store (nodes.melvin, edges.melvin)\n";
    std::cout << "  ✅ Uses melvin::Node and melvin::Edge\n";
    std::cout << "  ✅ All nodes persist automatically\n";
    std::cout << "  ✅ Adds Ollama scoring on top\n";
    std::cout << "  ✅ If you stop at 1000 nodes, next run starts with 1000!\n\n";
    
    std::string store_dir = "melvin_store";
    std::string watch_dir = "sample_data";
    int test_interval = 5;
    int ollama_interval = 10;
    int save_interval = 25;
    
    if (argc > 1) store_dir = argv[1];
    if (argc > 2) watch_dir = argv[2];
    if (argc > 3) test_interval = std::atoi(argv[3]);
    if (argc > 4) ollama_interval = std::atoi(argv[4]);
    
    MelvinOllamaIntegrated system(store_dir);
    system.run_continuous(watch_dir, test_interval, ollama_interval, save_interval);
    
    return 0;
}

