#include "GraphTokenSampler.h"
#include "melvin_types.h"
#include <random>
#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

GraphTokenSampler::GraphTokenSampler() {
  load_unigrams();
}

void GraphTokenSampler::load_unigrams() {
  if (unigrams_loaded_) return;
  
  // Common English unigrams for fallback
  top_unigrams_ = {
    "the", "a", "an", "and", "or", "but", "in", "on", "at", "to", "for", "of", "with", "by",
    "is", "are", "was", "were", "be", "been", "being", "have", "has", "had", "do", "does", "did",
    "will", "would", "could", "should", "may", "might", "can", "this", "that", "these", "those",
    "i", "you", "he", "she", "it", "we", "they", "me", "him", "her", "us", "them",
    "my", "your", "his", "her", "its", "our", "their", "mine", "yours", "ours", "theirs",
    "what", "who", "when", "where", "why", "how", "which", "whom", "whose",
    "here", "there", "now", "then", "today", "tomorrow", "yesterday", "always", "never", "sometimes",
    "good", "bad", "big", "small", "new", "old", "first", "last", "next", "previous",
    "think", "know", "see", "look", "hear", "feel", "want", "need", "like", "love",
    "go", "come", "get", "give", "take", "make", "find", "use", "work", "play",
    "say", "tell", "ask", "answer", "help", "try", "start", "stop", "continue", "finish"
  };
  
  unigrams_loaded_ = true;
}

TokenSamplingResult GraphTokenSampler::sample_tokens(const NextTokenContext& ctx) {
  TokenSamplingResult result;
  result.tokens = ctx.tokens;
  
  std::vector<std::string> candidates;
  
  // 1) Try to get candidates from graph context
  if (!ctx.context_nodes.empty()) {
    auto ctx_tokens = context_anchor_tokens(ctx.context_nodes, 64);
    candidates.insert(candidates.end(), ctx_tokens.begin(), ctx_tokens.end());
  }
  
  // 2) If still empty, pull from top unigrams
  if (candidates.empty()) {
    auto priors = top_unigram_tokens(128);
    candidates.insert(candidates.end(), priors.begin(), priors.end());
  }
  
  // 3) Still empty? Last resort single token to bootstrap
  if (candidates.empty()) {
    candidates.push_back("the");
    result.fallback_reason = "bootstrap_token";
  }
  
  // Generate tokens with minimum length enforcement
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, candidates.size() - 1);
  
  while ((int)result.tokens.size() < ctx.min_len || 
         (ctx.force_period && result.tokens.empty())) {
    
    // Sample next token
    int idx = dis(gen);
    std::string next_token = candidates[idx];
    
    // Check if this is a stop token before min_len
    bool is_stop = is_stop_token(next_token);
    bool before_min = ((int)result.tokens.size() < ctx.min_len);
    
    if (is_stop && before_min) {
      continue; // Skip stop tokens before minimum length
    }
    
    result.tokens.push_back(next_token);
    
    // Stop if we hit a stop token after min_len
    if (is_stop && !before_min) {
      break;
    }
    
    // Safety check for maximum length
    if ((int)result.tokens.size() >= ctx.max_tokens) {
      result.truncated = true;
      break;
    }
  }
  
  // Ensure we end with punctuation if force_period is set
  if (ctx.force_period && !result.tokens.empty()) {
    std::string last_token = result.tokens.back();
    if (last_token != "." && last_token != "?" && last_token != "!") {
      result.tokens.push_back(".");
    }
  }
  
  // Convert tokens to text
  for (size_t i = 0; i < result.tokens.size(); ++i) {
    if (i > 0 && result.tokens[i] != "." && result.tokens[i] != "," && 
        result.tokens[i] != "!" && result.tokens[i] != "?") {
      result.text += " ";
    }
    result.text += result.tokens[i];
  }
  
  // Capitalize first letter
  if (!result.text.empty() && std::islower(result.text[0])) {
    result.text[0] = std::toupper(result.text[0]);
  }
  
  result.confidence = 0.5; // Default confidence for fallback generation
  
  return result;
}

std::vector<std::string> GraphTokenSampler::context_anchor_tokens(const std::vector<melvin::NodeID>& nodes, int max_tokens) {
  std::vector<std::string> tokens;
  
  // For now, return some generic tokens based on context
  // In a real implementation, this would extract actual token labels from the nodes
  tokens.push_back("concept");
  tokens.push_back("relates");
  tokens.push_back("to");
  tokens.push_back("knowledge");
  tokens.push_back("information");
  tokens.push_back("understanding");
  tokens.push_back("learning");
  
  if ((int)tokens.size() > max_tokens) {
    tokens.resize(max_tokens);
  }
  
  return tokens;
}

std::vector<std::string> GraphTokenSampler::top_unigram_tokens(int max_tokens) {
  if ((int)top_unigrams_.size() > max_tokens) {
    return std::vector<std::string>(top_unigrams_.begin(), top_unigrams_.begin() + max_tokens);
  }
  return top_unigrams_;
}

std::string GraphTokenSampler::generate_response_graph_conditioned(const ReasoningTrace& rt, 
                                                                 const std::vector<melvin::NodeID>& context_nodes, 
                                                                 int min_len) {
  NextTokenContext ctx;
  ctx.context_nodes = context_nodes;
  ctx.min_len = min_len;
  ctx.force_period = true;
  ctx.max_tokens = 50;
  
  auto result = sample_tokens(ctx);
  
  // If we have reasoning trace information, try to incorporate it
  if (!rt.visited_nodes.empty()) {
    std::string trace_text = rt.visited_nodes[0] + " relates to other concepts";
    if (rt.visited_nodes.size() > 1) {
      trace_text = rt.visited_nodes[0] + " relates to " + rt.visited_nodes[1];
    }
    return trace_text;
  }
  
  return result.text.empty() ? "I'm learning about this concept." : result.text;
}

std::string GraphTokenSampler::token_to_text(const std::string& token) {
  return token; // Simple identity mapping for now
}

std::string GraphTokenSampler::text_to_token(const std::string& text) {
  return text; // Simple identity mapping for now
}

bool GraphTokenSampler::is_stop_token(const std::string& token) {
  return token == "." || token == "?" || token == "!";
}

void GraphTokenSampler::update_ngram_cache(const std::vector<std::string>& tokens) {
  // Update bigram cache
  for (size_t i = 0; i < tokens.size() - 1; ++i) {
    std::string bigram = tokens[i] + " " + tokens[i + 1];
    bigram_counts_[bigram]++;
  }
  
  // Update trigram cache
  for (size_t i = 0; i < tokens.size() - 2; ++i) {
    std::string trigram = tokens[i] + " " + tokens[i + 1] + " " + tokens[i + 2];
    trigram_counts_[trigram]++;
  }
}

double GraphTokenSampler::get_ngram_probability(const std::string& token, const std::vector<std::string>& context) {
  // Simple Kneser-Ney smoothing approximation
  if (context.empty()) {
    return 1.0 / top_unigrams_.size(); // Uniform fallback
  }
  
  // Check bigram probability
  std::string bigram = context.back() + " " + token;
  auto bigram_it = bigram_counts_.find(bigram);
  if (bigram_it != bigram_counts_.end() && bigram_it->second > 0) {
    return 0.7; // High probability for seen bigrams
  }
  
  // Fall back to unigram probability
  return 0.3;
}
