/**
 * @file answer_synthesizer.cpp
 * @brief Implementation of answer synthesizer
 */

#include "answer_synthesizer.h"
#include <sstream>
#include <algorithm>
#include <deque>

namespace melvin {
namespace reasoning {

AnswerSynthesizer::AnswerSynthesizer() {}

std::string AnswerSynthesizer::generate_lm_style(
    const std::vector<std::pair<std::string, float>>& top_concepts,
    const std::unordered_map<int, std::string>& id_to_word,
    float confidence
) {
    (void)id_to_word; // Not needed for this variant
    
    if (top_concepts.empty()) {
        return "...";  // Thinking silence
    }
    
    // Build concept tokens with weights
    std::vector<std::pair<std::string, float>> token_pool;
    token_pool.reserve(top_concepts.size());
    for (const auto& [word, score] : top_concepts) {
        token_pool.emplace_back(word, std::max(0.0001f, score));
    }
    
    // Add minimal function words (avoid template-y relation phrases)
    const std::vector<std::pair<const char*, float>> connectors = {
        {"and", 0.25f}, {"also", 0.18f}, {"because", 0.15f},
        {"however", 0.12f}, {"maybe", 0.10f}
    };
    for (auto& c : connectors) token_pool.emplace_back(c.first, c.second);
    
    // Post-echo audit sampling: encourage entropy, stronger repetition penalty
    float temperature = 1.0f;
    float repetition_penalty = 1.3f; // global, cross-turn
    static std::deque<float> recent_conf;
    recent_conf.push_back(confidence);
    if (recent_conf.size() > 5) recent_conf.pop_front();
    float mean_conf = 0.0f; for (float c : recent_conf) mean_conf += c; mean_conf /= std::max<size_t>(1, recent_conf.size());
    if (mean_conf > 0.95f) temperature = 1.2f; // confidence damping -> raise temperature
    
    // Helper to sample from weighted tokens (with global repetition penalty + nucleus top-p)
    std::unordered_map<std::string, int> used;
    static std::deque<std::string> recent_tokens;
    static std::string last_sentence;
    auto recent_penalty = [&](const std::string& t){
        int cnt = 0; for (const auto& s : recent_tokens) if (s == t) cnt++;
        return std::pow(0.8f, (float)cnt); // more repeats => stronger downweight
    };
    auto sample_token = [&]()->std::string{
        // Build softmax over weights/temperature with repetition penalty
        double sum = 0.0;
        std::vector<double> probs(token_pool.size());
        for (size_t i = 0; i < token_pool.size(); ++i) {
            double w = std::max(1e-6, (double)token_pool[i].second);
            w *= (double)recent_penalty(token_pool[i].first);
            if (used.count(token_pool[i].first)) {
                w *= std::pow(repetition_penalty, used[token_pool[i].first]);
            }
            double p = std::pow(w, 1.0 / temperature);
            probs[i] = p;
            sum += p;
        }
        // sort indices by prob desc
        std::vector<size_t> idx(probs.size()); for (size_t i = 0; i < idx.size(); ++i) idx[i] = i;
        std::sort(idx.begin(), idx.end(), [&](size_t a, size_t b){ return probs[a] > probs[b]; });
        // nucleus top-p (0.9)
        double norm_sum = std::max(1e-12, sum);
        double cum = 0.0; std::vector<size_t> nucleus; nucleus.reserve(idx.size());
        for (size_t j = 0; j < idx.size(); ++j) {
            double pj = probs[idx[j]] / norm_sum; cum += pj; nucleus.push_back(idx[j]); if (cum >= 0.9) break;
        }
        size_t pick = nucleus.empty() ? idx.front() : nucleus[(size_t)rand() % nucleus.size()];
        used[token_pool[pick].first]++;
        return token_pool[pick].first;
    };
    
    // Generate 6-12 tokens (organic sentence length), limit function words to 30%
    int target_tokens = 6 + rand() % 7;
    std::ostringstream out;
    int max_function = std::max(2, target_tokens * 3 / 10);
    int fn_used = 0;
    int repeat_count = 0; std::string prev_token;
    for (int i = 0; i < target_tokens; ++i) {
        std::string tok = sample_token();
        if (tok == prev_token) repeat_count++; else repeat_count = 0;
        if (repeat_count >= 2) { used[tok] += 2; tok = sample_token(); repeat_count = (tok == prev_token) ? 1 : 0; }
        bool is_function = (tok == "and" || tok == "also" || tok == "because" || tok == "however" || tok == "maybe");
        if (is_function && fn_used >= max_function) { --i; continue; }
        if (i > 0) out << " ";
        // Capitalize first word
        if (i == 0 && !tok.empty()) {
            tok[0] = std::toupper(tok[0]);
        }
        out << tok;
        prev_token = tok;
        if (is_function) fn_used++;
        // track global recent tokens
        recent_tokens.push_back(tok);
        if (recent_tokens.size() > 50) recent_tokens.pop_front();
    }
    std::string sentence = out.str() + ".";
    // If sentence overlaps heavily with last, increase temperature next call via static effect
    last_sentence = sentence;
    return sentence;
}

std::string AnswerSynthesizer::generate(
    const std::vector<ScoredNode>& scored_nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    melvin::language::ReasoningIntent intent,
    const std::vector<std::string>& query_tokens,
    const std::vector<int>& query_node_ids
) {
    if (scored_nodes.empty()) {
        return "I don't have enough information to answer that question.";
    }
    
    // Organic LM-like freeform generation for all intents
    // We construct a small token pool from top concepts + connective phrases
    // and sample a sentence with temperature and repetition penalty.
    
    // Build concept tokens with weights
    std::vector<std::pair<std::string, float>> token_pool; token_pool.reserve(scored_nodes.size());
    for (const auto& sn : scored_nodes) {
        auto it = id_to_word.find(sn.node_id);
        if (it != id_to_word.end()) {
            token_pool.emplace_back(it->second, std::max(0.0001f, sn.final_score));
        }
    }
    
    // Add connective tokens with moderate weights (acts like learned connectors)
    const std::vector<std::pair<const char*, float>> connectors = {
        {"because", 0.35f}, {"so", 0.25f}, {"therefore", 0.20f}, {"however", 0.22f},
        {"and", 0.50f}, {"also", 0.30f}, {"but", 0.28f}, {"which", 0.18f},
        {"usually", 0.16f}, {"sometimes", 0.16f}, {"in", 0.14f}, {"with", 0.14f}
    };
    for (auto& c : connectors) token_pool.emplace_back(c.first, c.second);
    
    // Temperature and repetition penalty derived from top score spread
    float max_s = scored_nodes.front().final_score;
    float min_s = scored_nodes.back().final_score;
    float spread = std::max(0.001f, max_s - min_s);
    float temperature = std::clamp(0.8f + (0.5f - std::min(spread, 0.5f)), 0.7f, 1.3f);
    float repetition_penalty = 0.85f; // discourage immediate repeats
    
    // Helper to sample from weighted tokens
    auto sample_token = [&](std::unordered_map<std::string,int>& used)->std::string{
        // Build softmax over weights/temperature with repetition penalty
        double sum = 0.0;
        std::vector<double> probs(token_pool.size());
        for (size_t i = 0; i < token_pool.size(); ++i) {
            double w = std::max(1e-6, (double)token_pool[i].second);
            auto it = used.find(token_pool[i].first);
            if (it != used.end()) {
                w *= std::pow(repetition_penalty, std::min(5, it->second));
            }
            w = std::pow(w, 1.0/temperature);
            probs[i] = w; sum += w;
        }
        if (sum <= 0.0) return "";
        for (auto& p : probs) p /= sum;
        double r = (double)rand() / RAND_MAX;
        double acc = 0.0;
        for (size_t i = 0; i < probs.size(); ++i) { acc += probs[i]; if (r <= acc) return token_pool[i].first; }
        return token_pool.back().first;
    };
    
    // Compose sentence 8-20 tokens
    std::unordered_map<std::string,int> used;
    int len = 8 + (rand() % 13);
    std::vector<std::string> words; words.reserve(len);
    
    // Seed with a query token if present
    if (!query_tokens.empty()) {
        words.push_back(query_tokens.front());
        used[words.back()]++;
    }
    while ((int)words.size() < len) {
        std::string t = sample_token(used);
        if (t.empty()) break;
        // Avoid doubling connectors
        if (!words.empty()) {
            const std::string& prev = words.back();
            if ((t == "and" || t == "but" || t == "so" || t == "however") &&
                (prev == "and" || prev == "but" || prev == "so" || prev == "however")) {
                continue;
            }
        }
        words.push_back(t);
        used[t]++;
    }
    
    // Basic cleanup and capitalization
    if (!words.empty()) words[0][0] = std::toupper(words[0][0]);
    std::stringstream out;
    for (size_t i = 0; i < words.size(); ++i) {
        out << words[i];
        if (i + 1 < words.size()) out << (words[i] == "," ? "" : " ");
    }
    out << ".";
    return out.str();
}

std::string AnswerSynthesizer::generate_explanation(
    const ScoredNode& answer_node,
    const std::unordered_map<int, std::string>& id_to_word
) {
    if (answer_node.best_path.size() < 2) {
        return "Direct association.";
    }
    
    std::stringstream ss;
    ss << "Reasoning: ";
    
    for (size_t i = 0; i < std::min(size_t(5), answer_node.best_path.size()); i++) {
        auto it = id_to_word.find(answer_node.best_path[i]);
        if (it != id_to_word.end()) {
            ss << it->second;
            if (i < std::min(size_t(4), answer_node.best_path.size() - 1)) {
                ss << " → ";
            }
        }
    }
    
    if (answer_node.best_path.size() > 5) {
        ss << " ... (" << (answer_node.best_path.size() - 5) << " more steps)";
    }
    
    return ss.str();
}

std::string AnswerSynthesizer::generate_definition(
    const std::vector<ScoredNode>& nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    const std::vector<std::string>& query_tokens
) {
    std::string subject = extract_subject(query_tokens, 
                                         melvin::language::ReasoningIntent::DEFINE);
    
    std::stringstream ss;
    
    if (!subject.empty()) {
        ss << capitalize_first(subject);
    } else {
        ss << "The answer";
    }
    
    ss << " is related to: ";
    
    // Take top 8 non-query nodes
    int count = 0;
    for (const auto& node : nodes) {
        if (count >= 8) break;
        
        auto it = id_to_word.find(node.node_id);
        if (it != id_to_word.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    ss << ".";
    
    return ss.str();
}

std::string AnswerSynthesizer::generate_location(
    const std::vector<ScoredNode>& nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    const std::vector<std::string>& query_tokens
) {
    std::string subject = extract_subject(query_tokens,
                                         melvin::language::ReasoningIntent::LOCATE);
    
    std::stringstream ss;
    
    if (!subject.empty()) {
        ss << capitalize_first(subject) << " is located in ";
    } else {
        ss << "Located in ";
    }
    
    // Take top 3 location nodes
    int count = 0;
    for (const auto& node : nodes) {
        if (count >= 3) break;
        
        auto it = id_to_word.find(node.node_id);
        if (it != id_to_word.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    ss << ".";
    
    return ss.str();
}

std::string AnswerSynthesizer::generate_cause(
    const std::vector<ScoredNode>& nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    const std::vector<std::string>& query_tokens
) {
    std::string subject = extract_subject(query_tokens,
                                         melvin::language::ReasoningIntent::CAUSE);
    
    std::stringstream ss;
    ss << "This occurs because of: ";
    
    // Take top 5 cause nodes
    int count = 0;
    for (const auto& node : nodes) {
        if (count >= 5) break;
        
        auto it = id_to_word.find(node.node_id);
        if (it != id_to_word.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    ss << ".";
    
    return ss.str();
}

std::string AnswerSynthesizer::generate_comparison(
    const std::vector<ScoredNode>& nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    const std::vector<std::string>& query_tokens
) {
    std::stringstream ss;
    ss << "Key differences include: ";
    
    // Take top 6 distinguishing nodes
    int count = 0;
    for (const auto& node : nodes) {
        if (count >= 6) break;
        
        auto it = id_to_word.find(node.node_id);
        if (it != id_to_word.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    ss << ".";
    
    return ss.str();
}

std::string AnswerSynthesizer::generate_generic(
    const std::vector<ScoredNode>& nodes,
    const std::unordered_map<int, std::string>& id_to_word,
    const std::vector<std::string>& query_tokens
) {
    std::stringstream ss;
    ss << "Related concepts: ";
    
    // Take top 10 nodes
    int count = 0;
    for (const auto& node : nodes) {
        if (count >= 10) break;
        
        auto it = id_to_word.find(node.node_id);
        if (it != id_to_word.end()) {
            if (count > 0) ss << ", ";
            ss << it->second;
            count++;
        }
    }
    
    ss << ".";
    
    return ss.str();
}

std::string AnswerSynthesizer::extract_subject(
    const std::vector<std::string>& query_tokens,
    melvin::language::ReasoningIntent intent
) {
    // Simple heuristic: last non-stop-word is often the subject
    std::unordered_set<std::string> stop_words = {
        "what", "where", "why", "how", "when", "is", "are", "the", "a", "an",
        "of", "in", "to", "for", "on", "at", "by", "with", "from"
    };
    
    for (auto it = query_tokens.rbegin(); it != query_tokens.rend(); ++it) {
        if (stop_words.find(*it) == stop_words.end()) {
            return *it;
        }
    }
    
    return "";
}

std::string AnswerSynthesizer::capitalize_first(const std::string& str) {
    if (str.empty()) return str;
    
    std::string result = str;
    result[0] = std::toupper(result[0]);
    return result;
}

bool AnswerSynthesizer::is_query_node(int node_id, const std::vector<int>& query_node_ids) {
    return std::find(query_node_ids.begin(), query_node_ids.end(), node_id) 
           != query_node_ids.end();
}

} // namespace reasoning
} // namespace melvin

