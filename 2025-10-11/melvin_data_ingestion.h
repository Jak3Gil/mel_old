/*
 * MELVIN DATA INGESTION SYSTEM
 * 
 * Continuous data feeding pipeline for real-world learning.
 * Supports text, structured data, and streaming sources.
 */

#pragma once

#include <string>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <atomic>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <chrono>
#include <memory>
#include <functional>
#include <regex>
#include <random>
#include "melvin_guardrails.h"
#include "melvin_telemetry.h"

namespace melvin {

// ==================== DATA TYPES ====================

enum class DataType {
    TEXT_PLAIN,
    TEXT_BOOK,
    TEXT_ARTICLE,
    TEXT_CONVERSATION,
    STRUCTURED_JSON,
    STRUCTURED_CSV,
    STREAMING_API,
    AUDIO_TRANSCRIPT,
    IMAGE_METADATA
};

struct DataChunk {
    std::string content;
    DataType type;
    std::string source;
    uint64_t timestamp;
    std::unordered_map<std::string, std::string> metadata;
    
    DataChunk(const std::string& c, DataType t, const std::string& s) 
        : content(c), type(t), source(s), timestamp(std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count()) {}
};

// ==================== TEXT PROCESSORS ====================

class TextProcessor {
private:
    std::regex sentence_splitter_;
    std::regex word_splitter_;
    std::vector<std::string> stop_words_;
    
public:
    TextProcessor() {
        // Initialize regex patterns
        sentence_splitter_ = std::regex(R"([.!?]+)");
        word_splitter_ = std::regex(R"(\s+)");
        
        // Load common stop words
        stop_words_ = {"the", "a", "an", "and", "or", "but", "in", "on", "at", "to", "for", 
                      "of", "with", "by", "is", "are", "was", "were", "be", "been", "have", 
                      "has", "had", "do", "does", "did", "will", "would", "could", "should"};
    }
    
    // Process text into learning-ready chunks
    std::vector<std::string> process_text(const std::string& text, DataType type) {
        std::vector<std::string> chunks;
        
        switch (type) {
            case DataType::TEXT_BOOK:
                chunks = process_book(text);
                break;
            case DataType::TEXT_ARTICLE:
                chunks = process_article(text);
                break;
            case DataType::TEXT_CONVERSATION:
                chunks = process_conversation(text);
                break;
            default:
                chunks = process_generic(text);
                break;
        }
        
        return chunks;
    }
    
private:
    std::vector<std::string> process_book(const std::string& text) {
        // Split into paragraphs, then sentences
        std::vector<std::string> chunks;
        std::istringstream stream(text);
        std::string paragraph;
        
        while (std::getline(stream, paragraph)) {
            if (paragraph.empty()) continue;
            
            // Split paragraph into sentences
            std::vector<std::string> sentences = split_sentences(paragraph);
            for (const auto& sentence : sentences) {
                if (sentence.length() > 20) {  // Filter very short sentences
                    chunks.push_back(sentence);
                }
            }
        }
        
        return chunks;
    }
    
    std::vector<std::string> process_article(const std::string& text) {
        // Process structured articles (title, paragraphs, etc.)
        std::vector<std::string> chunks;
        std::istringstream stream(text);
        std::string line;
        
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            
            // Clean and process each line
            std::string cleaned = clean_text(line);
            if (cleaned.length() > 10) {
                chunks.push_back(cleaned);
            }
        }
        
        return chunks;
    }
    
    std::vector<std::string> process_conversation(const std::string& text) {
        // Process conversation logs (Q&A pairs, dialogue)
        std::vector<std::string> chunks;
        std::istringstream stream(text);
        std::string line;
        
        while (std::getline(stream, line)) {
            if (line.empty()) continue;
            
            // Extract speaker and content
            size_t colon_pos = line.find(':');
            if (colon_pos != std::string::npos) {
                std::string speaker = line.substr(0, colon_pos);
                std::string content = line.substr(colon_pos + 1);
                
                // Clean content
                std::string cleaned = clean_text(content);
                if (cleaned.length() > 5) {
                    chunks.push_back(speaker + ": " + cleaned);
                }
            }
        }
        
        return chunks;
    }
    
    std::vector<std::string> process_generic(const std::string& text) {
        // Generic text processing
        std::vector<std::string> chunks;
        std::vector<std::string> sentences = split_sentences(text);
        
        for (const auto& sentence : sentences) {
            std::string cleaned = clean_text(sentence);
            if (cleaned.length() > 10) {
                chunks.push_back(cleaned);
            }
        }
        
        return chunks;
    }
    
    std::vector<std::string> split_sentences(const std::string& text) {
        std::vector<std::string> sentences;
        std::sregex_token_iterator iter(text.begin(), text.end(), sentence_splitter_, -1);
        std::sregex_token_iterator end;
        
        for (; iter != end; ++iter) {
            std::string sentence = iter->str();
            if (!sentence.empty()) {
                sentences.push_back(trim(sentence));
            }
        }
        
        return sentences;
    }
    
    std::string clean_text(const std::string& text) {
        std::string cleaned = text;
        
        // Remove extra whitespace
        cleaned = std::regex_replace(cleaned, std::regex(R"(\s+)"), " ");
        
        // Remove special characters (keep basic punctuation)
        cleaned = std::regex_replace(cleaned, std::regex(R"([^\w\s.,!?;:])"), "");
        
        // Trim
        cleaned = trim(cleaned);
        
        return cleaned;
    }
    
    std::string trim(const std::string& str) {
        size_t first = str.find_first_not_of(' ');
        if (first == std::string::npos) return "";
        size_t last = str.find_last_not_of(' ');
        return str.substr(first, (last - first + 1));
    }
};

// ==================== DATA SOURCES ====================

class DataSource {
public:
    virtual ~DataSource() = default;
    virtual std::vector<DataChunk> get_data() = 0;
    virtual bool has_more_data() = 0;
    virtual std::string get_source_name() const = 0;
};

class FileDataSource : public DataSource {
private:
    std::string filepath_;
    DataType data_type_;
    std::ifstream file_;
    bool file_opened_;
    
public:
    FileDataSource(const std::string& filepath, DataType type) 
        : filepath_(filepath), data_type_(type), file_opened_(false) {
        
        file_.open(filepath_);
        file_opened_ = file_.is_open();
        
        if (!file_opened_) {
            std::cerr << "Failed to open file: " << filepath_ << "\n";
        }
    }
    
    std::vector<DataChunk> get_data() override {
        std::vector<DataChunk> chunks;
        
        if (!file_opened_) return chunks;
        
        // Read a chunk of data (e.g., 100 lines)
        std::string content;
        std::string line;
        int line_count = 0;
        const int CHUNK_SIZE = 100;
        
        while (line_count < CHUNK_SIZE && std::getline(file_, line)) {
            content += line + "\n";
            line_count++;
        }
        
        if (!content.empty()) {
            chunks.emplace_back(content, data_type_, filepath_);
        }
        
        return chunks;
    }
    
    bool has_more_data() override {
        return file_opened_ && file_.good();
    }
    
    std::string get_source_name() const override {
        return filepath_;
    }
};

class DirectoryDataSource : public DataSource {
private:
    std::string directory_path_;
    DataType data_type_;
    std::vector<std::string> files_;
    size_t current_file_index_;
    std::unique_ptr<FileDataSource> current_file_source_;
    
public:
    DirectoryDataSource(const std::string& dir_path, DataType type) 
        : directory_path_(dir_path), data_type_(type), current_file_index_(0) {
        
        // Scan directory for files
        scan_directory();
        
        // Open first file if available
        if (!files_.empty()) {
            open_next_file();
        }
    }
    
    std::vector<DataChunk> get_data() override {
        std::vector<DataChunk> chunks;
        
        while (chunks.empty() && has_more_data()) {
            if (current_file_source_) {
                chunks = current_file_source_->get_data();
                
                if (chunks.empty() && !current_file_source_->has_more_data()) {
                    open_next_file();
                }
            }
        }
        
        return chunks;
    }
    
    bool has_more_data() override {
        return current_file_index_ < files_.size() || 
               (current_file_source_ && current_file_source_->has_more_data());
    }
    
    std::string get_source_name() const override {
        return directory_path_;
    }
    
private:
    void scan_directory() {
        try {
            for (const auto& entry : std::filesystem::directory_iterator(directory_path_)) {
                if (entry.is_regular_file()) {
                    std::string extension = entry.path().extension().string();
                    
                    // Filter by file extension
                    if (extension == ".txt" || extension == ".md" || extension == ".json") {
                        files_.push_back(entry.path().string());
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cerr << "Error scanning directory " << directory_path_ << ": " << e.what() << "\n";
        }
        
        std::cout << "📁 Found " << files_.size() << " files in " << directory_path_ << "\n";
    }
    
    void open_next_file() {
        if (current_file_index_ < files_.size()) {
            std::string filepath = files_[current_file_index_];
            current_file_source_ = std::make_unique<FileDataSource>(filepath, data_type_);
            current_file_index_++;
            
            std::cout << "📖 Processing file " << current_file_index_ << "/" << files_.size() 
                      << ": " << std::filesystem::path(filepath).filename().string() << "\n";
        } else {
            current_file_source_.reset();
        }
    }
};

// ==================== DATA INGESTION ENGINE ====================

class DataIngestionEngine {
private:
    std::queue<DataChunk> data_queue_;
    std::mutex queue_mutex_;
    std::vector<std::unique_ptr<DataSource>> data_sources_;
    TextProcessor text_processor_;
    
    // Learning components
    std::unique_ptr<GuardrailsMonitor> guardrails_;
    std::unique_ptr<TelemetryLogger> telemetry_;
    
    // Control
    std::atomic<bool> running_;
    std::atomic<int> processed_chunks_;
    std::atomic<int> total_chunks_;
    std::thread ingestion_thread_;
    std::thread processing_thread_;
    
    // Configuration
    int max_queue_size_;
    int processing_batch_size_;
    std::chrono::milliseconds processing_interval_;
    
public:
    DataIngestionEngine() 
        : running_(false), processed_chunks_(0), total_chunks_(0),
          max_queue_size_(1000), processing_batch_size_(10),
          processing_interval_(std::chrono::milliseconds(100)) {
        
        // Initialize learning components
        guardrails_ = std::make_unique<GuardrailsMonitor>();
        telemetry_ = std::make_unique<TelemetryLogger>("melvin_learning_telemetry.jsonl");
        
        std::cout << "🚀 Melvin Data Ingestion Engine initialized\n";
    }
    
    ~DataIngestionEngine() {
        stop();
    }
    
    // Add data sources
    void add_file_source(const std::string& filepath, DataType type) {
        data_sources_.push_back(std::make_unique<FileDataSource>(filepath, type));
        std::cout << "📄 Added file source: " << filepath << "\n";
    }
    
    void add_directory_source(const std::string& dir_path, DataType type) {
        data_sources_.push_back(std::make_unique<DirectoryDataSource>(dir_path, type));
        std::cout << "📁 Added directory source: " << dir_path << "\n";
    }
    
    // Start continuous learning
    void start_learning() {
        if (running_) {
            std::cout << "⚠️ Learning already running\n";
            return;
        }
        
        running_ = true;
        
        // Start ingestion thread
        ingestion_thread_ = std::thread(&DataIngestionEngine::ingestion_loop, this);
        
        // Start processing thread
        processing_thread_ = std::thread(&DataIngestionEngine::processing_loop, this);
        
        std::cout << "🧠 Starting continuous learning from " << data_sources_.size() << " data sources\n";
    }
    
    void stop() {
        if (!running_) return;
        
        running_ = false;
        
        if (ingestion_thread_.joinable()) {
            ingestion_thread_.join();
        }
        
        if (processing_thread_.joinable()) {
            processing_thread_.join();
        }
        
        std::cout << "🛑 Learning stopped. Processed " << processed_chunks_ << " chunks\n";
    }
    
    // Get learning statistics
    struct LearningStats {
        int processed_chunks = 0;
        int total_chunks = 0;
        int queue_size = 0;
        float processing_rate = 0.0f;
        std::string current_source = "";
    };
    
    LearningStats get_stats() const {
        LearningStats stats;
        stats.processed_chunks = processed_chunks_.load();
        stats.total_chunks = total_chunks_.load();
        
        std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(queue_mutex_));
        stats.queue_size = data_queue_.size();
        
        // Calculate processing rate (chunks per second)
        static auto start_time = std::chrono::steady_clock::now();
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
        if (duration.count() > 0) {
            stats.processing_rate = static_cast<float>(stats.processed_chunks) / duration.count();
        }
        
        return stats;
    }
    
    // Get current guardrails status
    const GuardrailsMonitor& get_guardrails() const {
        return *guardrails_;
    }
    
private:
    void ingestion_loop() {
        std::cout << "📥 Starting data ingestion loop\n";
        
        while (running_) {
            bool data_added = false;
            
            // Process all data sources
            for (auto& source : data_sources_) {
                if (!source->has_more_data()) continue;
                
                std::vector<DataChunk> chunks = source->get_data();
                
                for (auto& chunk : chunks) {
            // Add to queue (with size limit)
            std::lock_guard<std::mutex> lock(queue_mutex_);
            
            if (static_cast<int>(data_queue_.size()) < max_queue_size_) {
                        data_queue_.push(std::move(chunk));
                        total_chunks_++;
                        data_added = true;
                    } else {
                        // Queue full, wait for processing
                        break;
                    }
                }
                
                if (!running_) break;
            }
            
            if (!data_added) {
                // No more data from any source, check if we should wait or stop
                bool any_source_has_data = false;
                for (auto& source : data_sources_) {
                    if (source->has_more_data()) {
                        any_source_has_data = true;
                        break;
                    }
                }
                
                if (!any_source_has_data) {
                    std::cout << "📚 All data sources exhausted\n";
                    break;
                }
                
                // Wait a bit before checking again
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
        
        std::cout << "📥 Data ingestion loop finished\n";
    }
    
    void processing_loop() {
        std::cout << "🧠 Starting data processing loop\n";
        
        while (running_) {
            std::vector<DataChunk> batch;
            
            // Collect batch of data
            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                
                int batch_size = std::min(processing_batch_size_, static_cast<int>(data_queue_.size()));
                for (int i = 0; i < batch_size; ++i) {
                    if (!data_queue_.empty()) {
                        batch.push_back(std::move(data_queue_.front()));
                        data_queue_.pop();
                    }
                }
            }
            
            if (!batch.empty()) {
                // Process batch
                process_batch(batch);
                
                processed_chunks_ += batch.size();
                
                // Log progress
                if (processed_chunks_ % 100 == 0) {
                    auto stats = get_stats();
                    std::cout << "📊 Processed " << stats.processed_chunks << " chunks "
                              << "(" << std::fixed << std::setprecision(1) << stats.processing_rate 
                              << " chunks/sec)\n";
                }
            } else {
                // No data to process, wait
                std::this_thread::sleep_for(processing_interval_);
            }
        }
        
        std::cout << "🧠 Data processing loop finished\n";
    }
    
    void process_batch(const std::vector<DataChunk>& batch) {
        for (const auto& chunk : batch) {
            // Process text into learning chunks
            std::vector<std::string> text_chunks = text_processor_.process_text(chunk.content, chunk.type);
            
            for (const auto& text_chunk : text_chunks) {
                // Simulate learning from text chunk
                simulate_learning_step(text_chunk);
            }
            
            // Log data source progress
            telemetry_->log_event(TelemetryEventType::PARAMETER_ADJUSTMENT, 
                                "data_processed:" + chunk.source);
        }
    }
    
    void simulate_learning_step(const std::string& /* text_chunk */) {
        // Simulate metrics from learning
        static std::random_device rd;
        static std::mt19937 gen(rd());
        std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        
        float entropy = 0.8f + dist(gen) * 0.4f;  // 0.8-1.2
        float top2_margin = 0.15f + dist(gen) * 0.15f;  // 0.15-0.3
        float success_rate = 0.75f + dist(gen) * 0.2f;  // 0.75-0.95
        float drift = 0.1f + dist(gen) * 0.1f;  // 0.1-0.2
        float thought_replay = 0.8f + dist(gen) * 0.15f;  // 0.8-0.95
        
        // Record metrics
        guardrails_->record_metrics(entropy, top2_margin, success_rate, drift, thought_replay);
        
        // Check for evolution triggers
        if (guardrails_->should_trigger_evolution()) {
            telemetry_->log_evolution_triggered("continuous_learning");
            std::cout << "🧬 Evolution triggered during continuous learning!\n";
        }
        
        // Log telemetry every 1000 learning steps
        if (processed_chunks_ % 1000 == 0) {
            auto metrics = guardrails_->get_current_snapshot();
            GenomeSnapshot genome;  // Default genome for now
            telemetry_->log_telemetry(genome, metrics);
        }
    }
};

} // namespace melvin
