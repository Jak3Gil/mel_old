#pragma once
#include <string>
#include <vector>
#include <unordered_set>
#include <memory>
#include <unordered_map>
#include "melvin_types.h"

// Reasoning trace structure
struct ReasoningTrace {
  int paths_found = 0;
  double best_confidence = 0.0;
  double best_top2_margin = 0.0;
  std::string best_path_text;
  std::vector<std::string> visited_nodes;
};

// Enhanced token sampling context with minimum length enforcement
struct NextTokenContext {
  std::vector<melvin::NodeID> context_nodes;
  std::string current_text;
  std::vector<std::string> tokens;
  int  min_len = 6;          // NEW: force at least N tokens
  bool force_period = true;  // ensure sentence end if none chosen
  double temperature = 0.8;
  int max_tokens = 50;
};

// Token sampling result
struct TokenSamplingResult {
  std::string text;
  std::vector<std::string> tokens;
  double confidence = 0.0;
  bool truncated = false;
  std::string fallback_reason;
};

// Graph-conditioned token sampler that guarantees output
class GraphTokenSampler {
private:
  // Cached unigram tokens for fallback
  std::vector<std::string> top_unigrams_;
  bool unigrams_loaded_ = false;
  
  // N-gram cache for smoothing
  std::unordered_map<std::string, int> bigram_counts_;
  std::unordered_map<std::string, int> trigram_counts_;
  
public:
  GraphTokenSampler();
  ~GraphTokenSampler() = default;
  
  // Main sampling function - guaranteed to return text
  TokenSamplingResult sample_tokens(const NextTokenContext& ctx);
  
  // Fallback token sources
  std::vector<std::string> context_anchor_tokens(const std::vector<melvin::NodeID>& nodes, int max_tokens = 64);
  std::vector<std::string> top_unigram_tokens(int max_tokens = 128);
  
  // Token generation helpers
  std::string generate_response_graph_conditioned(const ReasoningTrace& rt, 
                                                 const std::vector<melvin::NodeID>& context_nodes, 
                                                 int min_len = 6);
  
private:
  void load_unigrams();
  std::string token_to_text(const std::string& token);
  std::string text_to_token(const std::string& text);
  bool is_stop_token(const std::string& token);
  void update_ngram_cache(const std::vector<std::string>& tokens);
  double get_ngram_probability(const std::string& token, const std::vector<std::string>& context);
};
