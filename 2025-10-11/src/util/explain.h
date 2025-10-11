/*
 * MELVIN EXPLAIN TRACING
 * 
 * Detailed trace output for LEAP reasoning:
 * - Template gap detection
 * - Crowd table analysis
 * - Decision points with thresholds
 * - Path scoring breakdown
 */

#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstdlib>

namespace melvin_explain {

// ==================== DATA STRUCTURES ====================

struct CrowdRow {
    int y = -1;
    float support = 0.0f;
    int distinct = 0;
    
    CrowdRow() = default;
    CrowdRow(int y_, float support_, int distinct_) 
        : y(y_), support(support_), distinct(distinct_) {}
};

struct PathEdge {
    int src = -1;
    int dst = -1;
    int type = 0;  // 0=EXACT, 1=LEAP
    float weight = 0.0f;
    
    PathEdge() = default;
    PathEdge(int src_, int dst_, int type_, float weight_)
        : src(src_), dst(dst_), type(type_), weight(weight_) {}
};

struct ScoreParts {
    double sum_logw = 0.0;
    int n_exact = 0;
    int n_leap = 0;
    double coherence = 0.0;
    
    ScoreParts() = default;
    ScoreParts(double slw, int ne, int nl, double coh)
        : sum_logw(slw), n_exact(ne), n_leap(nl), coherence(coh) {}
};

// ==================== EXPLAIN SINK ====================

class ExplainSink {
private:
    bool enabled_ = false;
    std::stringstream buffer_;
    int indent_level_ = 0;
    
    void indent() {
        for (int i = 0; i < indent_level_; ++i) {
            buffer_ << "  ";
        }
    }
    
public:
    ExplainSink() {
        // Check environment variable
        const char* env = std::getenv("EXPLAIN");
        if (env && (std::string(env) == "1" || std::string(env) == "true")) {
            enabled_ = true;
        }
    }
    
    void enable(bool enable = true) {
        enabled_ = enable;
    }
    
    bool enabled() const {
        return enabled_;
    }
    
    void clear() {
        buffer_.str("");
        buffer_.clear();
        indent_level_ = 0;
    }
    
    // Template gap detection
    void template_gap(int subj, int connector, const std::string& subj_text, const std::string& conn_text) {
        if (!enabled_) return;
        
        indent();
        buffer_ << "[TEMPLATE GAP] subj=" << subj_text 
                << " (" << subj << "), conn=" << conn_text 
                << " (" << connector << ")\n";
    }
    
    // Crowd table with candidates
    void crowd_table(int connector, 
                     const std::vector<CrowdRow>& rows, 
                     int best_idx, 
                     int second_idx,
                     const std::map<int, std::string>& node_names) {
        if (!enabled_) return;
        
        indent();
        buffer_ << "[CROWD TABLE] connector=" << connector << "\n";
        
        indent_level_++;
        for (size_t i = 0; i < rows.size() && i < 10; ++i) {
            const auto& row = rows[i];
            indent();
            
            std::string marker = "";
            if ((int)i == best_idx) marker = " ← BEST";
            else if ((int)i == second_idx) marker = " ← SECOND";
            
            auto it = node_names.find(row.y);
            std::string y_text = it != node_names.end() ? it->second : std::to_string(row.y);
            
            buffer_ << "Y=" << y_text
                   << "  support=" << std::fixed << std::setprecision(1) << row.support
                   << "  distinct=" << row.distinct
                   << marker << "\n";
        }
        indent_level_--;
    }
    
    // Decision point
    void decision(std::string_view tag, 
                  std::string_view reason, 
                  const std::map<std::string, std::string>& kv) {
        if (!enabled_) return;
        
        indent();
        buffer_ << "[DECISION] " << tag << " reason=\"" << reason << "\"";
        
        for (const auto& [key, value] : kv) {
            buffer_ << " " << key << "=" << value;
        }
        buffer_ << "\n";
    }
    
    // Path with scoring breakdown
    void path(const std::vector<PathEdge>& edges, 
              const ScoreParts& score,
              const std::map<int, std::string>& node_names) {
        if (!enabled_) return;
        
        indent();
        buffer_ << "[PATH] " << edges.size() << " edges\n";
        
        indent_level_++;
        for (const auto& edge : edges) {
            indent();
            
            auto it_src = node_names.find(edge.src);
            auto it_dst = node_names.find(edge.dst);
            std::string src_text = it_src != node_names.end() ? it_src->second : std::to_string(edge.src);
            std::string dst_text = it_dst != node_names.end() ? it_dst->second : std::to_string(edge.dst);
            
            std::string type_str = (edge.type == 0) ? "EXACT" : "LEAP";
            
            buffer_ << src_text << " -" << type_str << "-> " << dst_text
                   << " (w=" << std::fixed << std::setprecision(2) << edge.weight << ")\n";
        }
        
        indent();
        buffer_ << "score: sum_logw=" << std::fixed << std::setprecision(2) << score.sum_logw
               << "  n_exact=" << score.n_exact
               << "  n_leap=" << score.n_leap
               << "  coherence=" << std::fixed << std::setprecision(2) << score.coherence << "\n";
        indent_level_--;
    }
    
    // Print accumulated trace
    void print(std::ostream& os) const {
        if (!enabled_) return;
        os << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
        os << "🔍 EXPLAIN TRACE\n";
        os << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
        os << buffer_.str();
        os << "\n━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    }
    
    // Custom message
    void message(const std::string& msg) {
        if (!enabled_) return;
        indent();
        buffer_ << msg << "\n";
    }
    
    // Section header
    void section(const std::string& title) {
        if (!enabled_) return;
        buffer_ << "\n";
        indent();
        buffer_ << "┌─ " << title << " ─┐\n";
        indent_level_++;
    }
    
    void end_section() {
        if (!enabled_) return;
        indent_level_--;
    }
};

// Global explain sink
static ExplainSink EXPLAIN;

// ==================== CONVENIENCE MACROS ====================

#define EXPLAIN_TEMPLATE_GAP(subj, conn, subj_text, conn_text) \
    melvin_explain::EXPLAIN.template_gap(subj, conn, subj_text, conn_text)

#define EXPLAIN_CROWD_TABLE(connector, rows, best, second, names) \
    melvin_explain::EXPLAIN.crowd_table(connector, rows, best, second, names)

#define EXPLAIN_DECISION(tag, reason, kv) \
    melvin_explain::EXPLAIN.decision(tag, reason, kv)

#define EXPLAIN_PATH(edges, score, names) \
    melvin_explain::EXPLAIN.path(edges, score, names)

#define EXPLAIN_MESSAGE(msg) \
    melvin_explain::EXPLAIN.message(msg)

#define EXPLAIN_SECTION(title) \
    melvin_explain::EXPLAIN.section(title)

#define EXPLAIN_END_SECTION() \
    melvin_explain::EXPLAIN.end_section()

#define EXPLAIN_PRINT(os) \
    melvin_explain::EXPLAIN.print(os)

#define EXPLAIN_CLEAR() \
    melvin_explain::EXPLAIN.clear()

#define EXPLAIN_ENABLE(enable) \
    melvin_explain::EXPLAIN.enable(enable)

#define EXPLAIN_ENABLED() \
    melvin_explain::EXPLAIN.enabled()

} // namespace melvin_explain

