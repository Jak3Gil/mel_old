/*
 * MELVIN TELEMETRY ROLLUP CLI
 * 
 * Analyzes telemetry JSONL logs and provides:
 * - Event counts by tag
 * - Top subjects creating LEAPs
 * - Promotion/decay totals
 * - Average latency by phase
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <cstring>

// Simple JSON parser for key-value extraction
class SimpleJSON {
private:
    std::string data_;
    
public:
    SimpleJSON(const std::string& json) : data_(json) {}
    
    std::string get(const std::string& key) const {
        std::string search = "\"" + key + "\":\"";
        size_t start = data_.find(search);
        if (start == std::string::npos) return "";
        
        start += search.length();
        size_t end = data_.find("\"", start);
        if (end == std::string::npos) return "";
        
        return data_.substr(start, end - start);
    }
    
    double get_number(const std::string& key) const {
        std::string search = "\"" + key + "\":";
        size_t start = data_.find(search);
        if (start == std::string::npos) return 0.0;
        
        start += search.length();
        size_t end = data_.find_first_of(",}", start);
        if (end == std::string::npos) return 0.0;
        
        std::string num_str = data_.substr(start, end - start);
        try {
            return std::stod(num_str);
        } catch (...) {
            return 0.0;
        }
    }
};

// ==================== STATISTICS ====================

struct Stats {
    // Event counts
    std::map<std::string, int> tag_counts;
    
    // Subject tracking
    std::map<std::string, int> subject_leap_creates;
    std::map<std::string, int> subject_leap_rejects;
    
    // Connector tracking
    std::map<std::string, int> connector_usage;
    
    // Timing
    std::map<std::string, std::vector<double>> phase_times;
    
    // Promotion/decay
    int promotions = 0;
    int decays = 0;
    int conflicts = 0;
    int abstentions = 0;
    
    void process_event(const SimpleJSON& json) {
        std::string tag = json.get("tag");
        tag_counts[tag]++;
        
        if (tag == "leap_create") {
            std::string subject = json.get("subject");
            subject_leap_creates[subject]++;
            
            std::string connector = json.get("connector");
            connector_usage[connector]++;
        }
        else if (tag == "leap_reject") {
            std::string subject = json.get("subject");
            subject_leap_rejects[subject]++;
        }
        else if (tag == "leap_promote") {
            promotions++;
        }
        else if (tag == "leap_decay") {
            decays++;
        }
        else if (tag == "conflict_detected") {
            conflicts++;
        }
        else if (tag == "abstain") {
            abstentions++;
        }
        else if (tag == "timer") {
            std::string name = json.get("name");
            double duration_ms = json.get_number("duration_ms");
            phase_times[name].push_back(duration_ms);
        }
    }
    
    void print(std::ostream& os, int top_n = 20) const {
        os << "\n╔══════════════════════════════════════════════════════════════╗\n";
        os << "║  TELEMETRY ROLLUP                                            ║\n";
        os << "╚══════════════════════════════════════════════════════════════╝\n\n";
        
        // Event counts
        os << "📊 Event Counts:\n";
        os << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        
        std::vector<std::pair<std::string, int>> sorted_tags(tag_counts.begin(), tag_counts.end());
        std::sort(sorted_tags.begin(), sorted_tags.end(), 
                 [](const auto& a, const auto& b) { return a.second > b.second; });
        
        for (const auto& [tag, count] : sorted_tags) {
            os << "  " << std::left << std::setw(20) << tag << ": " << count << "\n";
        }
        os << "\n";
        
        // High-level summary
        os << "🎯 Summary:\n";
        os << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        os << "  Promotions:   " << promotions << "\n";
        os << "  Decays:       " << decays << "\n";
        os << "  Conflicts:    " << conflicts << "\n";
        os << "  Abstentions:  " << abstentions << "\n";
        os << "\n";
        
        // Top subjects creating LEAPs
        if (!subject_leap_creates.empty()) {
            os << "🔝 Top Subjects Creating LEAPs (Top " << std::min(top_n, (int)subject_leap_creates.size()) << "):\n";
            os << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            
            std::vector<std::pair<std::string, int>> sorted_subjects(subject_leap_creates.begin(), subject_leap_creates.end());
            std::sort(sorted_subjects.begin(), sorted_subjects.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            int shown = 0;
            for (const auto& [subject, count] : sorted_subjects) {
                if (shown >= top_n) break;
                os << "  " << std::left << std::setw(20) << subject << ": " << count << "\n";
                shown++;
            }
            os << "\n";
        }
        
        // Top connectors
        if (!connector_usage.empty()) {
            os << "🔗 Connector Usage:\n";
            os << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            
            std::vector<std::pair<std::string, int>> sorted_connectors(connector_usage.begin(), connector_usage.end());
            std::sort(sorted_connectors.begin(), sorted_connectors.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            for (const auto& [connector, count] : sorted_connectors) {
                os << "  " << std::left << std::setw(20) << connector << ": " << count << "\n";
            }
            os << "\n";
        }
        
        // Average latencies
        if (!phase_times.empty()) {
            os << "⏱️  Average Latencies:\n";
            os << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            
            for (const auto& [phase, times] : phase_times) {
                if (times.empty()) continue;
                
                double sum = 0.0;
                double min_time = times[0];
                double max_time = times[0];
                
                for (double t : times) {
                    sum += t;
                    min_time = std::min(min_time, t);
                    max_time = std::max(max_time, t);
                }
                
                double avg = sum / times.size();
                
                os << "  " << std::left << std::setw(30) << phase << ": "
                   << std::fixed << std::setprecision(2)
                   << avg << " ms  (min: " << min_time << ", max: " << max_time 
                   << ", n: " << times.size() << ")\n";
            }
            os << "\n";
        }
        
        // Rejection reasons
        if (!subject_leap_rejects.empty()) {
            os << "❌ Top Rejected Subjects (Top " << std::min(top_n, (int)subject_leap_rejects.size()) << "):\n";
            os << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
            
            std::vector<std::pair<std::string, int>> sorted_rejects(subject_leap_rejects.begin(), subject_leap_rejects.end());
            std::sort(sorted_rejects.begin(), sorted_rejects.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            int shown = 0;
            for (const auto& [subject, count] : sorted_rejects) {
                if (shown >= top_n) break;
                os << "  " << std::left << std::setw(20) << subject << ": " << count << "\n";
                shown++;
            }
            os << "\n";
        }
    }
};

// ==================== MAIN ====================

int main(int argc, char* argv[]) {
    std::string input_file;
    int top_n = 20;
    bool from_stdin = true;
    
    // Parse arguments
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--file" && i + 1 < argc) {
            input_file = argv[++i];
            from_stdin = false;
        } else if (arg == "--top" && i + 1 < argc) {
            top_n = std::atoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: telemetry_rollup [OPTIONS]\n\n";
            std::cout << "Options:\n";
            std::cout << "  --file FILE    Read from file instead of stdin\n";
            std::cout << "  --top N        Show top N items (default: 20)\n";
            std::cout << "  --help, -h     Show this help\n\n";
            std::cout << "Examples:\n";
            std::cout << "  cat guardrails_telemetry.jsonl | ./telemetry_rollup --top 20\n";
            std::cout << "  ./telemetry_rollup --file guardrails_telemetry.jsonl\n\n";
            return 0;
        }
    }
    
    Stats stats;
    
    // Process input
    if (from_stdin) {
        std::string line;
        while (std::getline(std::cin, line)) {
            if (line.empty()) continue;
            SimpleJSON json(line);
            stats.process_event(json);
        }
    } else {
        std::ifstream file(input_file);
        if (!file) {
            std::cerr << "Error: Could not open file: " << input_file << "\n";
            return 1;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty()) continue;
            SimpleJSON json(line);
            stats.process_event(json);
        }
    }
    
    // Print results
    stats.print(std::cout, top_n);
    
    return 0;
}

