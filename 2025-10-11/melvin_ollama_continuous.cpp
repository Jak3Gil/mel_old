/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN CONTINUOUS LEARNING WITH OLLAMA SELF-SCORING                      ║
 * ║  Continuously: Scrape → Learn → Evolve → Self-Score with Local AI        ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 * 
 * This daemon runs forever:
 * 1. Continuously scrapes databases/files/directories
 * 2. Creates nodes and connections in the knowledge graph
 * 3. Tests knowledge periodically
 * 4. Self-scores using Ollama (local LLM)
 * 5. Evolves parameters based on Ollama feedback
 * 6. Saves state and logs metrics
 * 7. NEVER STOPS (until you kill it with Ctrl+C)
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <filesystem>
#include <algorithm>
#include <random>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cstring>
#include <csignal>
#include <curl/curl.h>

// JSON parsing - simple implementation to avoid dependencies
namespace SimpleJSON {
    std::string extract_response(const std::string& json_str) {
        // Find "response":"..." in JSON
        size_t start = json_str.find("\"response\":\"");
        if (start == std::string::npos) return "";
        start += 12; // Skip "response":"
        
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
        
        // Escape special characters
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

namespace fs = std::filesystem;

// Global flag for graceful shutdown
std::atomic<bool> keep_running(true);

void signal_handler(int signal) {
    std::cout << "\n\n🛑 Received signal " << signal << " - shutting down gracefully...\n";
    keep_running = false;
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
    
    // Check if Ollama is available
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
    
    // Generate response from Ollama
    std::string generate(const std::string& prompt) {
        if (!curl_) return "";
        
        // Prepare JSON request
        std::string json_request = SimpleJSON::create_generate_request(model_name_, prompt);
        
        // Setup CURL
        std::string response_data;
        curl_easy_setopt(curl_, CURLOPT_URL, (base_url_ + "/api/generate").c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, json_request.c_str());
        curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &response_data);
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT, 30L);
        
        struct curl_slist* headers = nullptr;
        headers = curl_slist_append(headers, "Content-Type: application/json");
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers);
        
        // Perform request
        CURLcode res = curl_easy_perform(curl_);
        curl_slist_free_all(headers);
        
        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << "\n";
            return "";
        }
        
        // Parse JSON response
        return SimpleJSON::extract_response(response_data);
    }
    
    // Score an answer (0.0 - 1.0)
    float score_answer(const std::string& question, const std::string& answer, 
                      const std::string& expected_context = "") {
        std::string prompt = "Rate the quality of this answer on a scale of 0 to 10:\n\n";
        prompt += "Question: " + question + "\n";
        prompt += "Answer: " + answer + "\n";
        if (!expected_context.empty()) {
            prompt += "Expected context: " + expected_context + "\n";
        }
        prompt += "\nProvide ONLY a number from 0 to 10, nothing else:";
        
        std::string response = generate(prompt);
        
        // Extract numeric score
        try {
            // Remove whitespace and extract first number
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
                return std::min(1.0f, score / 10.0f);  // Normalize to 0-1
            }
        } catch (...) {
            // If parsing fails, return neutral score
        }
        
        return 0.5f;  // Default neutral score
    }
    
    // Get suggestions for improvement
    std::string get_improvement_suggestions(const std::vector<std::pair<std::string, float>>& recent_scores) {
        std::string prompt = "I am an AI learning system. Here are my recent performance scores:\n\n";
        
        for (const auto& [question, score] : recent_scores) {
            prompt += "Q: " + question + " (Score: " + std::to_string(score * 10) + "/10)\n";
        }
        
        prompt += "\nWhat should I focus on to improve my knowledge? Provide 3 specific suggestions:";
        
        return generate(prompt);
    }
};

// ==================== BINARY NODE SYSTEM ====================

struct BinaryNodeID {
    uint8_t hash[32];
    
    BinaryNodeID() { memset(hash, 0, 32); }
    
    bool operator==(const BinaryNodeID& other) const {
        return memcmp(hash, other.hash, 32) == 0;
    }
    
    static BinaryNodeID from_string(const std::string& text) {
        BinaryNodeID id;
        uint64_t h = 14695981039346656037ULL;
        for (char c : text) { h ^= (uint8_t)c; h *= 1099511628211ULL; }
        for (int i = 0; i < 32; ++i) {
            id.hash[i] = (uint8_t)((h >> (i % 8)) ^ (h >> ((i + 11) % 8)));
            h = h * 1099511628211ULL + i;
        }
        return id;
    }
};

namespace std {
    template<> struct hash<BinaryNodeID> {
        size_t operator()(const BinaryNodeID& id) const {
            size_t h = 0;
            for (int i = 0; i < 8; ++i) h ^= ((size_t)id.hash[i]) << (i * 8);
            return h;
        }
    };
}

struct ContinuousNode {
    BinaryNodeID id;
    std::string text;
    std::unordered_map<std::string, std::vector<BinaryNodeID>> edges;
    int reinforcements;
    uint64_t last_accessed;
    float confidence_score;  // Based on Ollama feedback
    
    ContinuousNode() : reinforcements(0), last_accessed(0), confidence_score(0.5f) {}
    ContinuousNode(const std::string& t) : text(t), reinforcements(0), confidence_score(0.5f) {
        id = BinaryNodeID::from_string(t);
        last_accessed = std::chrono::system_clock::now().time_since_epoch().count();
    }
    
    void touch() {
        last_accessed = std::chrono::system_clock::now().time_since_epoch().count();
        reinforcements++;
    }
    
    void update_confidence(float score) {
        // Exponential moving average
        confidence_score = 0.7f * confidence_score + 0.3f * score;
    }
};

// ==================== CONTINUOUS LEARNING ENGINE WITH OLLAMA ====================

class MelvinOllamaContinuous {
private:
    std::unordered_map<BinaryNodeID, ContinuousNode> graph_;
    std::unordered_map<std::string, BinaryNodeID> lookup_;
    std::unordered_set<std::string> processed_files_;
    
    std::unique_ptr<OllamaClient> ollama_;
    
    // Learning parameters
    float recall_ability_ = 0.5f;
    int generation_ = 0;
    int cycle_count_ = 0;
    
    // Ollama scoring history
    std::vector<std::pair<std::string, float>> recent_scores_;
    float average_ollama_score_ = 0.5f;
    
    // Statistics
    size_t total_nodes_created_ = 0;
    size_t total_edges_created_ = 0;
    size_t total_bytes_ingested_ = 0;
    size_t total_files_ingested_ = 0;
    size_t total_ollama_queries_ = 0;
    
    std::mt19937 rng_;
    
public:
    MelvinOllamaContinuous() : rng_(std::random_device{}()) {
        load_state();
        
        // Initialize Ollama client
        std::cout << "🤖 Initializing Ollama connection...\n";
        ollama_ = std::make_unique<OllamaClient>();
        
        if (ollama_->is_available()) {
            std::cout << "✅ Ollama is available and ready!\n";
        } else {
            std::cout << "⚠️  Warning: Ollama not available. Self-scoring will be simulated.\n";
            std::cout << "   To use Ollama, ensure it's running: ollama serve\n";
        }
    }
    
    ~MelvinOllamaContinuous() {
        save_state();
    }
    
    // ==================== CONTINUOUS OPERATIONS ====================
    
    void run_continuous(const std::string& watch_dir, int test_interval = 5, 
                       int save_interval = 25, int ollama_interval = 10) {
        std::cout << "\n🔄 STARTING MELVIN OLLAMA CONTINUOUS LEARNING\n";
        std::cout << "==============================================\n\n";
        std::cout << "Watching: " << watch_dir << "\n";
        std::cout << "Test every: " << test_interval << " cycles\n";
        std::cout << "Ollama scoring every: " << ollama_interval << " cycles\n";
        std::cout << "Save every: " << save_interval << " cycles\n";
        std::cout << "Press Ctrl+C to stop gracefully\n\n";
        
        auto last_scan = std::chrono::steady_clock::now();
        
        while (keep_running) {
            cycle_count_++;
            auto now = std::chrono::steady_clock::now();
            
            std::cout << "🔄 Cycle " << cycle_count_ << " [Gen " << generation_ 
                     << "] - " << graph_.size() << " nodes, " << edge_count() << " edges\n";
            
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
            
            // STEP 4: Evolve based on scores
            evolve_based_on_feedback();
            
            // STEP 5: Save state periodically
            if (cycle_count_ % save_interval == 0) {
                save_state();
                std::cout << "   💾 State saved (cycle " << cycle_count_ << ")\n";
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
        save_state();
        std::cout << "\n✅ Daemon shutdown complete\n";
    }
    
    void scan_and_ingest(const std::string& dir_path) {
        try {
            if (!fs::exists(dir_path)) {
                std::cout << "   📁 Creating watch directory: " << dir_path << "\n";
                fs::create_directories(dir_path);
                return;
            }
            
            for (const auto& entry : fs::recursive_directory_iterator(dir_path)) {
                if (!entry.is_regular_file()) continue;
                
                std::string path = entry.path().string();
                
                // Skip if already processed
                if (processed_files_.find(path) != processed_files_.end()) continue;
                
                // Skip binary/system files
                std::string ext = entry.path().extension().string();
                if (ext == ".o" || ext == ".bin" || ext == ".dat" || ext == ".exe" ||
                    ext == ".so" || ext == ".dylib" || ext == ".a") continue;
                
                // Ingest new file
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
        
        total_bytes_ingested_ += content.length();
        total_files_ingested_++;
        
        auto sentences = split_sentences(content);
        for (const auto& sent : sentences) {
            auto words = tokenize(sent);
            
            // Create nodes
            for (const auto& w : words) {
                if (w.length() >= 2) create_node(w);
            }
            
            // Extract relationships
            extract_relationships(words);
        }
        
        std::cout << "      ✅ +" << (graph_.size() - total_nodes_created_) << " nodes, "
                 << "+" << (edge_count() - total_edges_created_) << " edges\n";
                 
        total_nodes_created_ = graph_.size();
        total_edges_created_ = edge_count();
    }
    
    void test_current_knowledge() {
        std::cout << "   🧪 Testing knowledge...\n";
        
        // Auto-generate questions from current graph
        auto tests = auto_generate_tests(10);
        if (tests.empty()) {
            std::cout << "      ⚠️  No testable knowledge yet\n";
            return;
        }
        
        // Grade
        int correct = 0;
        for (const auto& t : tests) {
            std::string ans = answer_question(t.question);
            if (ans.find(t.expected) != std::string::npos || t.expected.find(ans) != std::string::npos) {
                correct++;
            }
        }
        
        float accuracy = (float)correct / tests.size();
        std::cout << "      📊 Self-test accuracy: " << std::fixed << std::setprecision(0) 
                 << (accuracy * 100) << "% (" << correct << "/" << tests.size() << " correct)\n";
        
        // Log to CSV
        log_metrics(accuracy);
    }
    
    void self_score_with_ollama() {
        std::cout << "   🤖 Self-scoring with Ollama...\n";
        
        // Generate test questions
        auto tests = auto_generate_tests(5);  // Fewer tests for Ollama (it's slower)
        if (tests.empty()) {
            std::cout << "      ⚠️  No testable knowledge for Ollama scoring\n";
            return;
        }
        
        float total_score = 0.0f;
        int scored_count = 0;
        
        for (const auto& t : tests) {
            std::string answer = answer_question(t.question);
            
            // Get Ollama to score the answer
            float score = ollama_->score_answer(t.question, answer, t.expected);
            
            total_ollama_queries_++;
            total_score += score;
            scored_count++;
            
            // Update node confidence
            auto words = tokenize(answer);
            for (const auto& word : words) {
                auto it = lookup_.find(word);
                if (it != lookup_.end()) {
                    graph_[it->second].update_confidence(score);
                }
            }
            
            // Store for history
            recent_scores_.push_back({t.question, score});
            if (recent_scores_.size() > 20) {
                recent_scores_.erase(recent_scores_.begin());
            }
            
            std::cout << "      Q: " << t.question.substr(0, 50) << "...\n";
            std::cout << "      A: " << answer << "\n";
            std::cout << "      🎯 Ollama score: " << std::fixed << std::setprecision(2) 
                     << (score * 10) << "/10\n";
        }
        
        if (scored_count > 0) {
            average_ollama_score_ = total_score / scored_count;
            std::cout << "      📊 Average Ollama score: " << std::fixed << std::setprecision(2)
                     << (average_ollama_score_ * 10) << "/10\n";
            
            // Get improvement suggestions periodically
            if (cycle_count_ % 30 == 0 && recent_scores_.size() >= 5) {
                std::cout << "      💡 Getting improvement suggestions from Ollama...\n";
                std::string suggestions = ollama_->get_improvement_suggestions(recent_scores_);
                if (!suggestions.empty()) {
                    std::cout << "      💡 Suggestions:\n";
                    std::cout << "         " << suggestions.substr(0, 200) << "...\n";
                }
            }
        }
    }
    
    void evolve_based_on_feedback() {
        // Evolve every N cycles, with acceleration based on poor Ollama scores
        int evolve_frequency = 10;
        
        // If Ollama scores are low, evolve more frequently
        if (average_ollama_score_ < 0.4f) {
            evolve_frequency = 5;
        } else if (average_ollama_score_ < 0.6f) {
            evolve_frequency = 7;
        }
        
        if (cycle_count_ % evolve_frequency == 0) {
            std::cout << "   🧬 Evolving based on feedback...\n";
            
            // Strengthen high-confidence nodes
            int boosted = 0;
            for (auto& [id, node] : graph_) {
                if (node.confidence_score > 0.7f && node.reinforcements > 3) {
                    node.touch();
                    boosted++;
                }
            }
            
            // Improve recall based on Ollama feedback
            float improvement = 0.01f * average_ollama_score_;
            recall_ability_ = std::min(0.98f, recall_ability_ + improvement);
            
            generation_++;
            
            std::cout << "      ✓ Boosted " << boosted << " high-confidence nodes\n";
            std::cout << "      ✓ Recall ability: " << std::fixed << std::setprecision(3) 
                     << recall_ability_ << "\n";
            std::cout << "      ✓ Avg confidence: " << average_confidence() << "\n";
        }
    }
    
    void print_stats() const {
        size_t edges = edge_count();
        
        std::cout << "\n   📊 STATS:\n";
        std::cout << "      Nodes: " << graph_.size() << "\n";
        std::cout << "      Edges: " << edges << "\n";
        std::cout << "      Generation: " << generation_ << "\n";
        std::cout << "      Files processed: " << total_files_ingested_ << "\n";
        std::cout << "      Bytes ingested: " << format_bytes(total_bytes_ingested_) << "\n";
        std::cout << "      Recall ability: " << std::fixed << std::setprecision(3) 
                 << recall_ability_ << "\n";
        std::cout << "      Ollama queries: " << total_ollama_queries_ << "\n";
        std::cout << "      Avg Ollama score: " << std::fixed << std::setprecision(2)
                 << (average_ollama_score_ * 10) << "/10\n";
        std::cout << "      Avg node confidence: " << average_confidence() << "\n\n";
    }
    
    // ==================== STATE PERSISTENCE ====================
    
    void save_state() {
        // Save binary graph
        std::ofstream bin("melvin_ollama_continuous.bin", std::ios::binary);
        if (!bin) return;
        
        uint32_t node_count = graph_.size();
        bin.write((char*)&node_count, sizeof(node_count));
        bin.write((char*)&recall_ability_, sizeof(recall_ability_));
        bin.write((char*)&generation_, sizeof(generation_));
        bin.write((char*)&cycle_count_, sizeof(cycle_count_));
        bin.write((char*)&average_ollama_score_, sizeof(average_ollama_score_));
        bin.write((char*)&total_ollama_queries_, sizeof(total_ollama_queries_));
        
        for (const auto& [id, node] : graph_) {
            bin.write((char*)node.id.hash, 32);
            
            uint32_t text_len = node.text.length();
            bin.write((char*)&text_len, sizeof(text_len));
            bin.write(node.text.c_str(), text_len);
            
            bin.write((char*)&node.reinforcements, sizeof(node.reinforcements));
            bin.write((char*)&node.confidence_score, sizeof(node.confidence_score));
            
            uint32_t edge_count = node.edges.size();
            bin.write((char*)&edge_count, sizeof(edge_count));
            
            for (const auto& [rel, targets] : node.edges) {
                uint32_t rel_len = rel.length();
                bin.write((char*)&rel_len, sizeof(rel_len));
                bin.write(rel.c_str(), rel_len);
                
                uint32_t target_count = targets.size();
                bin.write((char*)&target_count, sizeof(target_count));
                
                for (const auto& tgt : targets) {
                    bin.write((char*)tgt.hash, 32);
                }
            }
        }
        
        // Save processed files list
        std::ofstream files_log("melvin_ollama_processed_files.txt");
        for (const auto& f : processed_files_) {
            files_log << f << "\n";
        }
    }
    
    void load_state() {
        std::ifstream bin("melvin_ollama_continuous.bin", std::ios::binary);
        if (!bin) return;
        
        uint32_t node_count;
        bin.read((char*)&node_count, sizeof(node_count));
        bin.read((char*)&recall_ability_, sizeof(recall_ability_));
        bin.read((char*)&generation_, sizeof(generation_));
        bin.read((char*)&cycle_count_, sizeof(cycle_count_));
        bin.read((char*)&average_ollama_score_, sizeof(average_ollama_score_));
        bin.read((char*)&total_ollama_queries_, sizeof(total_ollama_queries_));
        
        for (uint32_t i = 0; i < node_count; ++i) {
            ContinuousNode node;
            bin.read((char*)node.id.hash, 32);
            
            uint32_t text_len;
            bin.read((char*)&text_len, sizeof(text_len));
            node.text.resize(text_len);
            bin.read(&node.text[0], text_len);
            
            bin.read((char*)&node.reinforcements, sizeof(node.reinforcements));
            bin.read((char*)&node.confidence_score, sizeof(node.confidence_score));
            
            uint32_t edge_count;
            bin.read((char*)&edge_count, sizeof(edge_count));
            
            for (uint32_t j = 0; j < edge_count; ++j) {
                uint32_t rel_len;
                bin.read((char*)&rel_len, sizeof(rel_len));
                std::string rel(rel_len, '\0');
                bin.read(&rel[0], rel_len);
                
                uint32_t target_count;
                bin.read((char*)&target_count, sizeof(target_count));
                
                for (uint32_t k = 0; k < target_count; ++k) {
                    BinaryNodeID tgt;
                    bin.read((char*)tgt.hash, 32);
                    node.edges[rel].push_back(tgt);
                }
            }
            
            graph_[node.id] = node;
            lookup_[node.text] = node.id;
        }
        
        // Load processed files
        std::ifstream files_log("melvin_ollama_processed_files.txt");
        std::string path;
        while (std::getline(files_log, path)) {
            processed_files_.insert(path);
        }
        
        if (node_count > 0) {
            std::cout << "💾 Loaded state: " << node_count << " nodes, gen " << generation_ 
                     << ", cycle " << cycle_count_ 
                     << ", avg Ollama score " << std::fixed << std::setprecision(2)
                     << (average_ollama_score_ * 10) << "/10\n\n";
        }
    }
    
private:
    void create_node(const std::string& text) {
        if (lookup_.find(text) != lookup_.end()) {
            graph_[lookup_[text]].touch();
            return;
        }
        
        ContinuousNode node(text);
        graph_[node.id] = node;
        lookup_[text] = node.id;
    }
    
    void link(const std::string& from, const std::string& to, const std::string& rel) {
        if (from.empty() || to.empty() || from.length() < 2 || to.length() < 2) return;
        if (from == to) return;
        
        create_node(from);
        create_node(to);
        
        auto from_it = lookup_.find(from);
        auto to_it = lookup_.find(to);
        
        if (from_it != lookup_.end() && to_it != lookup_.end()) {
            auto& targets = graph_[from_it->second].edges[rel];
            if (std::find(targets.begin(), targets.end(), to_it->second) == targets.end()) {
                targets.push_back(to_it->second);
            }
        }
    }
    
    void extract_relationships(const std::vector<std::string>& words) {
        std::string subj;
        
        for (size_t i = 0; i + 2 < words.size(); ++i) {
            if (words[i+1] == "is" || words[i+1] == "are") {
                subj = words[i];
                std::string obj = skip_article(words, i+1);
                if (!obj.empty()) link(words[i], obj, "ISA");
            }
            
            std::string s = words[i];
            if ((s == "that" || s == "they" || s == "it") && !subj.empty()) s = subj;
            
            if (words[i+1] == "has" || words[i+1] == "have") link(s, words[i+2], "HAS");
            if (words[i+1] == "can" || words[i+1] == "could") link(s, words[i+2], "CAN");
            if (words[i+1] == "eat" || words[i+1] == "eats" || words[i+1] == "consume" || words[i+1] == "consumes") 
                link(s, words[i+2], "CONSUMES");
            if (words[i+1] == "use" || words[i+1] == "uses") link(s, words[i+2], "USES");
            if (words[i+1] == "create" || words[i+1] == "creates") link(s, words[i+2], "CREATES");
            if ((words[i+1] == "live" || words[i+1] == "lives") && i+3 < words.size() && words[i+2] == "in")
                link(s, words[i+3], "LIVES_IN");
        }
    }
    
    struct TestQ { std::string question, expected; };
    
    std::vector<TestQ> auto_generate_tests(int count) {
        std::vector<TestQ> tests;
        std::vector<BinaryNodeID> candidates;
        
        for (const auto& [id, node] : graph_) {
            if (!node.edges.empty()) candidates.push_back(id);
        }
        
        if (candidates.empty()) return tests;
        
        std::shuffle(candidates.begin(), candidates.end(), rng_);
        
        for (int i = 0; i < count && i < (int)candidates.size(); ++i) {
            auto& node = graph_[candidates[i]];
            
            for (const auto& [rel, targets] : node.edges) {
                if (targets.empty()) continue;
                
                TestQ tq;
                if (rel == "ISA") tq.question = "What is a " + node.text + "?";
                else if (rel == "HAS") tq.question = "What does a " + node.text + " have?";
                else if (rel == "CAN") tq.question = "What can a " + node.text + " do?";
                else if (rel == "CONSUMES") tq.question = "What does a " + node.text + " consume?";
                else continue;
                
                tq.expected = graph_[targets[0]].text;
                tests.push_back(tq);
                
                if ((int)tests.size() >= count) return tests;
            }
        }
        
        return tests;
    }
    
    std::string answer_question(const std::string& q) {
        auto words = tokenize(q);
        std::string subject, rel;
        
        for (size_t i = 0; i < words.size(); ++i) {
            if (words[i] == "what") {
                if (i+1 < words.size() && words[i+1] == "is") {
                    rel = "ISA";
                    subject = words.back();
                } else if (i+1 < words.size() && words[i+1] == "does" && i+3 < words.size()) {
                    subject = words[i+2];
                    if (words[i+3] == "have") rel = "HAS";
                    else if (words[i+3] == "consume") rel = "CONSUMES";
                } else if (i+1 < words.size() && words[i+1] == "can" && i+2 < words.size()) {
                    rel = "CAN";
                    subject = words[i+2];
                }
            }
        }
        
        // Find answer
        auto it = lookup_.find(subject);
        if (it == lookup_.end()) {
            it = lookup_.find(subject + "s");
            if (it == lookup_.end() && subject.length() > 1 && subject.back() == 's') {
                it = lookup_.find(subject.substr(0, subject.length()-1));
            }
            if (it == lookup_.end()) return "unknown";
        }
        
        auto node_it = graph_.find(it->second);
        if (node_it == graph_.end()) return "unknown";
        
        auto edge_it = node_it->second.edges.find(rel);
        if (edge_it != node_it->second.edges.end() && !edge_it->second.empty()) {
            std::uniform_real_distribution<float> dist(0.0f, 1.0f);
            if (dist(rng_) < recall_ability_) {
                auto target = graph_.find(edge_it->second[0]);
                if (target != graph_.end()) return target->second.text;
            }
        }
        
        return "unknown";
    }
    
    void log_metrics(float accuracy) {
        static bool header_written = false;
        std::ofstream log("melvin_ollama_metrics.csv", std::ios::app);
        
        if (!header_written) {
            log << "timestamp,cycle,generation,nodes,edges,accuracy,recall_ability,ollama_score,avg_confidence\n";
            header_written = true;
        }
        
        auto now = std::chrono::system_clock::now().time_since_epoch().count();
        log << now << "," << cycle_count_ << "," << generation_ << "," 
            << graph_.size() << "," << edge_count() << "," << accuracy << "," 
            << recall_ability_ << "," << average_ollama_score_ << ","
            << average_confidence() << "\n";
        log.flush();
    }
    
    float average_confidence() const {
        if (graph_.empty()) return 0.0f;
        
        float total = 0.0f;
        for (const auto& [id, node] : graph_) {
            total += node.confidence_score;
        }
        return total / graph_.size();
    }
    
    size_t edge_count() const {
        size_t total = 0;
        for (const auto& [id, n] : graph_) {
            for (const auto& [r, ts] : n.edges) total += ts.size();
        }
        return total;
    }
    
    std::vector<std::string> split_sentences(const std::string& text) {
        std::vector<std::string> result;
        std::string current;
        for (char c : text) {
            if (c == '.' || c == '!' || c == '?' || c == '\n') {
                if (!current.empty()) { result.push_back(current); current.clear(); }
            } else {
                current += c;
            }
        }
        if (!current.empty()) result.push_back(current);
        return result;
    }
    
    std::vector<std::string> tokenize(const std::string& text) {
        std::vector<std::string> tokens;
        std::stringstream ss(text);
        std::string word;
        while (ss >> word) {
            word.erase(std::remove_if(word.begin(), word.end(), 
                      [](char c) { return std::ispunct(c) && c != '-'; }), word.end());
            std::transform(word.begin(), word.end(), word.begin(), ::tolower);
            if (word.length() >= 2) tokens.push_back(word);
        }
        return tokens;
    }
    
    std::string skip_article(const std::vector<std::string>& words, size_t pos) {
        if (pos + 1 >= words.size()) return "";
        std::string next = words[pos + 1];
        if (next == "a" || next == "an" || next == "the") {
            return (pos + 2 < words.size()) ? words[pos + 2] : "";
        }
        return next;
    }
    
    std::string format_bytes(size_t bytes) const {
        const char* units[] = {"B", "KB", "MB", "GB"};
        int unit = 0;
        double size = bytes;
        
        while (size >= 1024 && unit < 3) {
            size /= 1024;
            unit++;
        }
        
        std::stringstream ss;
        ss << std::fixed << std::setprecision(2) << size << " " << units[unit];
        return ss.str();
    }
};

// ==================== MAIN ====================

int main(int argc, char* argv[]) {
    // Setup signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    std::cout << "🧠 MELVIN OLLAMA CONTINUOUS LEARNING DAEMON\n";
    std::cout << "===========================================\n\n";
    std::cout << "Runs forever, continuously:\n";
    std::cout << "  📁 Watches directories for new files\n";
    std::cout << "  🔄 Ingests data → Creates binary nodes\n";
    std::cout << "  🧪 Tests knowledge periodically\n";
    std::cout << "  🤖 Self-scores with Ollama (local AI)\n";
    std::cout << "  🧬 Evolves based on Ollama feedback\n";
    std::cout << "  💾 Saves state continuously\n";
    std::cout << "  📊 Logs all metrics\n\n";
    
    std::cout << "Requirements:\n";
    std::cout << "  - Ollama installed and running (ollama serve)\n";
    std::cout << "  - A model pulled (e.g., ollama pull llama3.2)\n";
    std::cout << "  - libcurl library (usually pre-installed)\n\n";
    
    std::string watch_dir = "sample_data";
    int test_interval = 5;
    int save_interval = 25;
    int ollama_interval = 10;
    
    if (argc > 1) watch_dir = argv[1];
    if (argc > 2) test_interval = std::atoi(argv[2]);
    if (argc > 3) ollama_interval = std::atoi(argv[3]);
    if (argc > 4) save_interval = std::atoi(argv[4]);
    
    MelvinOllamaContinuous melvin;
    melvin.run_continuous(watch_dir, test_interval, save_interval, ollama_interval);
    
    return 0;
}

