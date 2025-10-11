#include "miner.hpp"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <sstream>
#include <random>

namespace melvin {

// PatternMiner implementation
void PatternMiner::mine_sequence(const std::vector<NodeID>& sequence) {
    if (sequence.size() < config_.min_pattern_length) return;
    
    // Update recent sequences window
    update_recent_sequences(sequence);
    
    // Extract n-grams of different lengths
    for (size_t n = config_.min_pattern_length; n <= config_.max_pattern_length && n <= sequence.size(); ++n) {
        std::vector<Pattern> ngrams = extract_ngrams(sequence, n);
        
        for (const auto& pattern : ngrams) {
            std::string context = generate_context_signature(sequence);
            update_ngram_stats(pattern, context);
        }
    }
    
    // Decay old patterns
    decay_old_patterns();
}

void PatternMiner::mine_sequences(const std::vector<std::vector<NodeID>>& sequences) {
    for (const auto& sequence : sequences) {
        mine_sequence(sequence);
    }
}

std::vector<Pattern> PatternMiner::get_frequent_patterns(double min_frequency) const {
    std::vector<Pattern> frequent_patterns;
    
    for (const auto& pattern : discovered_patterns_) {
        if (min_frequency < 0.0 || pattern.frequency >= min_frequency) {
            frequent_patterns.push_back(pattern);
        }
    }
    
    // Sort by frequency (descending)
    std::sort(frequent_patterns.begin(), frequent_patterns.end(),
              [](const Pattern& a, const Pattern& b) { return a.frequency > b.frequency; });
    
    return frequent_patterns;
}

std::vector<Pattern> PatternMiner::get_generalization_candidates() const {
    std::vector<Pattern> candidates;
    
    for (const auto& pattern : discovered_patterns_) {
        if (pattern.frequency >= config_.theta_pat) {
            candidates.push_back(pattern);
        }
    }
    
    // Sort by generalization score (frequency * diversity)
    std::sort(candidates.begin(), candidates.end(),
            [this](const Pattern& a, const Pattern& b) {
                double score_a = a.frequency * compute_pattern_diversity(a);
                double score_b = b.frequency * compute_pattern_diversity(b);
                  return score_a > score_b;
              });
    
    return candidates;
}

void PatternMiner::update_pattern_stats() {
    uint64_t current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Update pattern frequencies and confidences
    for (auto& pattern : discovered_patterns_) {
        std::string pattern_str = pattern_to_string(pattern);
        auto it = ngram_stats_.find(pattern_str);
        
        if (it != ngram_stats_.end()) {
            pattern.frequency = it->second.count;
            pattern.confidence = compute_pattern_confidence(pattern);
            pattern.last_seen = current_time;
        }
    }
    
    // Remove low-frequency patterns
    discovered_patterns_.erase(
        std::remove_if(discovered_patterns_.begin(), discovered_patterns_.end(),
                      [this](const Pattern& pattern) {
                          return pattern.frequency < config_.theta_pat;
                      }),
        discovered_patterns_.end()
    );
}

std::vector<Pattern> PatternMiner::extract_ngrams(const std::vector<NodeID>& sequence, size_t n) const {
    std::vector<Pattern> ngrams;
    
    for (size_t i = 0; i <= sequence.size() - n; ++i) {
        Pattern pattern;
        pattern.sequence.assign(sequence.begin() + i, sequence.begin() + i + n);
        
        // Extract relations between consecutive nodes
        for (size_t j = 0; j < pattern.sequence.size() - 1; ++j) {
            Rel relation = extract_relation_between_nodes(pattern.sequence[j], pattern.sequence[j+1]);
            pattern.relations.push_back(relation);
        }
        
        pattern.first_seen = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        pattern.last_seen = pattern.first_seen;
        
        ngrams.push_back(pattern);
    }
    
    return ngrams;
}

void PatternMiner::update_ngram_stats(const Pattern& pattern, const std::string& context) {
    std::string pattern_str = pattern_to_string(pattern);
    
    auto it = ngram_stats_.find(pattern_str);
    if (it == ngram_stats_.end()) {
        NGramStats stats;
        stats.pattern_string = pattern_str;
        stats.constituent_nodes = pattern.sequence;
        ngram_stats_[pattern_str] = stats;
        it = ngram_stats_.find(pattern_str);
    }
    
    it->second.count++;
    it->second.unique_contexts++;
    it->second.last_updated = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Update diversity score
    it->second.diversity_score = compute_pattern_diversity(pattern);
    
    // Add to discovered patterns if meets criteria
    if (it->second.count >= config_.theta_pat && it->second.diversity_score >= config_.theta_div) {
        bool pattern_exists = false;
        for (auto& existing_pattern : discovered_patterns_) {
            if (patterns_overlap(existing_pattern, pattern)) {
                existing_pattern.frequency = it->second.count;
                existing_pattern.contexts.push_back(context);
                pattern_exists = true;
                break;
            }
        }
        
        if (!pattern_exists) {
            Pattern new_pattern = pattern;
            new_pattern.frequency = it->second.count;
            new_pattern.contexts.push_back(context);
            new_pattern.confidence = compute_pattern_confidence(new_pattern);
            discovered_patterns_.push_back(new_pattern);
        }
    }
}

bool PatternMiner::should_generalize_pattern(const Pattern& pattern) const {
    return pattern.frequency >= config_.theta_pat &&
           compute_pattern_diversity(pattern) >= config_.theta_div &&
           pattern.confidence >= config_.min_generalization_confidence;
}

double PatternMiner::compute_pattern_diversity(const Pattern& pattern) const {
    // Simple diversity based on number of unique contexts
    // In practice, you'd want more sophisticated diversity measures
    
    std::string pattern_str = pattern_to_string(pattern);
    auto it = ngram_stats_.find(pattern_str);
    
    if (it == ngram_stats_.end()) return 0.0;
    
    // Normalize by frequency
    if (it->second.count == 0) return 0.0;
    
    return static_cast<double>(it->second.unique_contexts) / it->second.count;
}

double PatternMiner::compute_pattern_confidence(const Pattern& pattern) const {
    // Simple confidence based on frequency and diversity
    double frequency_score = std::min(1.0, static_cast<double>(pattern.frequency) / 20.0);
    double diversity_score = compute_pattern_diversity(pattern);
    
    return (frequency_score + diversity_score) / 2.0;
}

std::string PatternMiner::generate_context_signature(const std::vector<NodeID>& sequence) const {
    // Simple context signature based on sequence length and first/last nodes
    std::ostringstream oss;
    oss << sequence.size() << "_";
    
    if (!sequence.empty()) {
        // Use first few bytes of first and last node IDs as context
        for (size_t i = 0; i < std::min(static_cast<size_t>(4), sequence[0].size()); ++i) {
            oss << std::hex << static_cast<int>(sequence[0][i]);
        }
        oss << "_";
        
        if (sequence.size() > 1) {
            for (size_t i = 0; i < std::min(static_cast<size_t>(4), sequence.back().size()); ++i) {
                oss << std::hex << static_cast<int>(sequence.back()[i]);
            }
        }
    }
    
    return oss.str();
}

void PatternMiner::update_recent_sequences(const std::vector<NodeID>& sequence) {
    recent_sequences_.push(sequence);
    
    // Maintain window size
    while (recent_sequences_.size() > config_.window_size) {
        recent_sequences_.pop();
    }
}

void PatternMiner::decay_old_patterns() {
    uint64_t current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Decay old n-gram statistics
    for (auto& [pattern_str, stats] : ngram_stats_) {
        if (current_time - stats.last_updated > 86400000000000ULL) { // 1 day in nanoseconds
            stats.count = static_cast<uint32_t>(stats.count * config_.decay_factor);
            stats.last_updated = current_time;
        }
    }
    
    // Remove very old or low-frequency patterns
    auto it = ngram_stats_.begin();
    while (it != ngram_stats_.end()) {
        if (it->second.count < 2) {
            it = ngram_stats_.erase(it);
        } else {
            ++it;
        }
    }
}

std::string PatternMiner::pattern_to_string(const Pattern& pattern) const {
    std::ostringstream oss;
    
    for (size_t i = 0; i < pattern.sequence.size(); ++i) {
        if (i > 0) oss << "->";
        
        // Use first few bytes of node ID for string representation
        for (size_t j = 0; j < std::min(static_cast<size_t>(4), pattern.sequence[i].size()); ++j) {
            oss << std::hex << static_cast<int>(pattern.sequence[i][j]);
        }
    }
    
    return oss.str();
}

bool PatternMiner::patterns_overlap(const Pattern& pattern1, const Pattern& pattern2) const {
    // Simple overlap detection based on sequence similarity
    if (pattern1.sequence.size() != pattern2.sequence.size()) return false;
    
    for (size_t i = 0; i < pattern1.sequence.size(); ++i) {
        if (pattern1.sequence[i] != pattern2.sequence[i]) {
            return false;
        }
    }
    
    return true;
}

Rel PatternMiner::extract_relation_between_nodes(const NodeID& node1, const NodeID& node2) const {
    // This is a simplified implementation
    // In practice, you'd query the store for the actual relation
    
    if (!store_) return Rel::TEMPORAL; // Default relation
    
    // Try to find an edge between the nodes
    RelMask all_relations;
    all_relations.set_all();
    
    AdjView view;
    if (store_->get_out_edges(node1, all_relations, view)) {
        for (size_t i = 0; i < view.count; ++i) {
            NodeID target_node;
            std::copy(view.edges[i].dst, view.edges[i].dst + 32, target_node.begin());
            
            if (target_node == node2) {
                return static_cast<Rel>(view.edges[i].rel);
            }
        }
    }
    
    return Rel::TEMPORAL; // Default if no edge found
}

// GeneralizationCreator implementation
std::vector<NodeID> GeneralizationCreator::create_generalization_nodes(const std::vector<Pattern>& patterns) {
    std::vector<NodeID> created_nodes;
    
    for (const auto& pattern : patterns) {
        if (pattern.frequency >= config_.theta_pat) {
            NodeID gen_node = create_generalization_node(pattern);
            if (!is_zero_id(gen_node)) {
                created_nodes.push_back(gen_node);
                connect_pattern_to_generalization(pattern, gen_node);
            }
        }
    }
    
    // Create hierarchical generalizations if enabled
    if (config_.enable_hierarchical_generalization && created_nodes.size() > 1) {
        std::vector<NodeID> hierarchical_nodes = create_hierarchical_generalizations(created_nodes);
        created_nodes.insert(created_nodes.end(), hierarchical_nodes.begin(), hierarchical_nodes.end());
    }
    
    return created_nodes;
}

NodeID GeneralizationCreator::create_generalization_node(const Pattern& pattern) {
    // Create node header
    NodeRecHeader header;
    header.type = static_cast<uint32_t>(determine_generalization_type(pattern));
    header.flags = static_cast<uint32_t>(GENERALIZED);
    header.ts_created = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    header.ts_updated = header.ts_created;
    header.degree_hint = pattern.sequence.size();
    header.pin_expiry = 0;
    header.confirm_count = pattern.frequency;
    header.anchor_pad = 0;
    
    // Generate label
    std::string label = generate_generalization_label(pattern);
    header.payload_len = label.size();
    
    // Create payload
    std::vector<uint8_t> payload(label.begin(), label.end());
    
    return store_->upsert_node(header, payload);
}

void GeneralizationCreator::connect_pattern_to_generalization(const Pattern& pattern, const NodeID& gen_node) {
    create_generalization_edges(gen_node, pattern.sequence);
}

std::vector<NodeID> GeneralizationCreator::create_hierarchical_generalizations(const std::vector<NodeID>& existing_generalizations) {
    std::vector<NodeID> hierarchical_nodes;
    
    if (!should_create_hierarchy(existing_generalizations)) {
        return hierarchical_nodes;
    }
    
    std::vector<NodeID> candidates = find_hierarchy_candidates(existing_generalizations);
    
    // Create higher-level generalizations for candidates
    for (size_t i = 0; i < candidates.size(); i += 2) {
        if (i + 1 < candidates.size()) {
            // Create a pattern representing the two generalizations
            Pattern hierarchy_pattern;
            hierarchy_pattern.sequence = {candidates[i], candidates[i+1]};
            hierarchy_pattern.frequency = 1;
            hierarchy_pattern.confidence = 0.8;
            
            NodeID hierarchy_node = create_generalization_node(hierarchy_pattern);
            if (!is_zero_id(hierarchy_node)) {
                hierarchical_nodes.push_back(hierarchy_node);
                create_generalization_edges(hierarchy_node, {candidates[i], candidates[i+1]});
            }
        }
    }
    
    return hierarchical_nodes;
}

std::string GeneralizationCreator::generate_generalization_label(const Pattern& pattern) const {
    std::ostringstream oss;
    oss << "GEN_" << pattern.sequence.size() << "_" << pattern.frequency;
    
    // Add constituent node information
    for (size_t i = 0; i < std::min(static_cast<size_t>(3), pattern.sequence.size()); ++i) {
        oss << "_";
        for (size_t j = 0; j < std::min(static_cast<size_t>(2), pattern.sequence[i].size()); ++j) {
            oss << std::hex << static_cast<int>(pattern.sequence[i][j]);
        }
    }
    
    return oss.str();
}

NodeType GeneralizationCreator::determine_generalization_type(const Pattern& pattern) const {
    // Determine the best node type for this generalization
    if (pattern.sequence.size() <= 2) {
        return NodeType::PHRASE;
    } else if (pattern.sequence.size() <= 4) {
        return NodeType::CONCEPT;
    } else {
        return NodeType::ABSTRACTION;
    }
}

void GeneralizationCreator::create_generalization_edges(const NodeID& gen_node, const std::vector<NodeID>& constituents) {
    for (const auto& constituent : constituents) {
        create_generalization_edge(constituent, gen_node);
    }
}

EdgeID GeneralizationCreator::create_generalization_edge(const NodeID& constituent, const NodeID& generalization) {
    EdgeRec edge;
    std::copy(constituent.begin(), constituent.end(), edge.src);
    std::copy(generalization.begin(), generalization.end(), edge.dst);
    edge.rel = static_cast<uint32_t>(Rel::GENERALIZATION);
    edge.layer = 2; // Generalized layer
    edge.w = 0.8f;
    edge.w_core = 0.8f;
    edge.w_ctx = 0.0f;
    edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    edge.count = 1;
    edge.flags = static_cast<uint32_t>(EDGE_GENERALIZED);
    edge.pad = 0;
    
    return store_->upsert_edge(edge);
}

bool GeneralizationCreator::should_create_hierarchy(const std::vector<NodeID>& generalizations) const {
    return generalizations.size() >= 4 && config_.enable_hierarchical_generalization;
}

std::vector<NodeID> GeneralizationCreator::find_hierarchy_candidates(const std::vector<NodeID>& generalizations) const {
    // Simple heuristic: pair up generalizations
    std::vector<NodeID> candidates = generalizations;
    
    // Shuffle to create random pairings
    std::random_device rd;
    std::mt19937 g(rd());
    std::shuffle(candidates.begin(), candidates.end(), g);
    
    return candidates;
}

// PMIMiner implementation
void PMIMiner::update_pmi_stats(const std::vector<NodeID>& sequence) {
    if (sequence.size() < 2) return;
    
    total_observations_ += sequence.size();
    
    update_node_counts(sequence);
    update_pair_counts(sequence);
    update_cooccurrence_contexts(sequence);
    
    // Decay old statistics
    decay_old_statistics();
}

std::vector<std::pair<NodeID, NodeID>> PMIMiner::get_leap_candidates() const {
    std::vector<std::pair<NodeID, NodeID>> candidates;
    
    for (const auto& [pair, count] : pair_counts_) {
        if (should_create_leap_edge(pair.first, pair.second)) {
            candidates.push_back(pair);
        }
    }
    
    return candidates;
}

std::vector<EdgeID> PMIMiner::create_leap_edges(const std::vector<std::pair<NodeID, NodeID>>& candidates) {
    std::vector<EdgeID> created_edges;
    
    for (const auto& [src, dst] : candidates) {
        double pmi = compute_pmi(src, dst);
        EdgeID edge_id = create_leap_edge(src, dst, pmi);
        
        if (!is_zero_id(edge_id)) {
            created_edges.push_back(edge_id);
        }
    }
    
    return created_edges;
}

double PMIMiner::compute_pmi(const NodeID& node_a, const NodeID& node_b) const {
    double prob_a = compute_probability(node_a);
    double prob_b = compute_probability(node_b);
    double prob_ab = compute_joint_probability(node_a, node_b);
    
    if (prob_a <= 0.0 || prob_b <= 0.0 || prob_ab <= 0.0) return 0.0;
    
    return std::log(prob_ab / (prob_a * prob_b));
}

bool PMIMiner::should_create_leap_edge(const NodeID& node_a, const NodeID& node_b) const {
    if (!are_temporally_distant(node_a, node_b)) return false;
    
    double pmi = compute_pmi(node_a, node_b);
    return pmi >= config_.theta_pmi;
}

double PMIMiner::compute_probability(const NodeID& node) const {
    auto it = node_counts_.find(node);
    if (it == node_counts_.end() || total_observations_ == 0) return 0.0;
    
    return static_cast<double>(it->second) / total_observations_;
}

double PMIMiner::compute_joint_probability(const NodeID& node_a, const NodeID& node_b) const {
    auto it = pair_counts_.find({node_a, node_b});
    if (it == pair_counts_.end()) {
        it = pair_counts_.find({node_b, node_a});
        if (it == pair_counts_.end()) return 0.0;
    }
    
    return static_cast<double>(it->second) / total_observations_;
}

bool PMIMiner::are_temporally_distant(const NodeID& node_a, const NodeID& node_b) const {
    // Check if nodes are temporally distant based on cooccurrence contexts
    auto it = cooccurrence_contexts_.find(node_a);
    if (it == cooccurrence_contexts_.end()) return false;
    
    for (const auto& [other_node, distance] : it->second) {
        if (other_node == node_b && distance > config_.max_temporal_distance) {
            return true;
        }
    }
    
    return false;
}

void PMIMiner::update_node_counts(const std::vector<NodeID>& sequence) {
    for (const auto& node : sequence) {
        node_counts_[node]++;
    }
}

void PMIMiner::update_pair_counts(const std::vector<NodeID>& sequence) {
    for (size_t i = 0; i < sequence.size() - 1; ++i) {
        for (size_t j = i + 1; j < sequence.size(); ++j) {
            if (j - i <= config_.max_temporal_distance) continue; // Skip temporally adjacent pairs
            
            NodeID node_a = sequence[i];
            NodeID node_b = sequence[j];
            
            if (node_a < node_b) {
                pair_counts_[{node_a, node_b}]++;
            } else {
                pair_counts_[{node_b, node_a}]++;
            }
        }
    }
}

void PMIMiner::update_cooccurrence_contexts(const std::vector<NodeID>& sequence) {
    for (size_t i = 0; i < sequence.size(); ++i) {
        for (size_t j = i + 1; j < sequence.size(); ++j) {
            double distance = static_cast<double>(j - i);
            
            cooccurrence_contexts_[sequence[i]].emplace_back(sequence[j], distance);
            cooccurrence_contexts_[sequence[j]].emplace_back(sequence[i], distance);
        }
    }
}

void PMIMiner::decay_old_statistics() {
    // Simple decay - reduce counts over time
    uint64_t current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // This is a placeholder - in practice you'd track timestamps and decay appropriately
}

EdgeID PMIMiner::create_leap_edge(const NodeID& src, const NodeID& dst, double pmi_score) {
    EdgeRec edge;
    std::copy(src.begin(), src.end(), edge.src);
    std::copy(dst.begin(), dst.end(), edge.dst);
    edge.rel = static_cast<uint32_t>(Rel::LEAP);
    edge.layer = 1; // Inferred layer
    edge.w = compute_edge_weight_from_pmi(pmi_score);
    edge.w_core = edge.w * 0.7f;
    edge.w_ctx = edge.w * 0.3f;
    edge.ts_last = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    edge.count = 1;
    edge.flags = static_cast<uint32_t>(EDGE_INFERRED);
    edge.pad = 0;
    
    return store_->upsert_edge(edge);
}

double PMIMiner::compute_edge_weight_from_pmi(double pmi) const {
    // Convert PMI score to edge weight [0, 1]
    return std::min(1.0, std::max(0.0, (pmi + 2.0) / 4.0));
}

// CausalMiner implementation
void CausalMiner::mine_causal_relationships(const std::vector<NodeID>& sequence) {
    if (sequence.size() < 2) return;
    
    update_lead_lag_statistics(sequence);
    update_temporal_distances(sequence);
}

std::vector<std::pair<NodeID, NodeID>> CausalMiner::get_causal_relationships(double min_strength) const {
    std::vector<std::pair<NodeID, NodeID>> relationships;
    
    for (const auto& [pair, strength] : causal_strengths_) {
        if (strength >= min_strength) {
            relationships.push_back(pair);
        }
    }
    
    // Sort by causal strength
    std::sort(relationships.begin(), relationships.end(),
              [this](const std::pair<NodeID, NodeID>& a, const std::pair<NodeID, NodeID>& b) {
                  return get_causal_strength(a.first, a.second) > get_causal_strength(b.first, b.second);
              });
    
    return relationships;
}

double CausalMiner::get_causal_strength(const NodeID& cause, const NodeID& effect) const {
    auto it = causal_strengths_.find({cause, effect});
    if (it != causal_strengths_.end()) {
        return it->second;
    }
    
    // Compute from lead-lag counts if not cached
    auto count_it = lead_lag_counts_.find({cause, effect});
    if (count_it != lead_lag_counts_.end()) {
        auto reverse_it = lead_lag_counts_.find({effect, cause});
        uint32_t lag_count = (reverse_it != lead_lag_counts_.end()) ? reverse_it->second : 0;
        return compute_causal_strength_from_counts(count_it->second, lag_count);
    }
    
    return 0.0;
}

bool CausalMiner::is_causal(const NodeID& cause, const NodeID& effect) const {
    return get_causal_strength(cause, effect) >= 0.6; // Default threshold
}

void CausalMiner::update_causal_annotations() {
    // Update edge flags to indicate causal relationships
    // This would require iterating through edges and updating their flags
    // Implementation depends on the specific edge annotation system
}

void CausalMiner::update_lead_lag_statistics(const std::vector<NodeID>& sequence) {
    for (size_t i = 0; i < sequence.size() - 1; ++i) {
        for (size_t j = i + 1; j < sequence.size(); ++j) {
            lead_lag_counts_[{sequence[i], sequence[j]}]++;
        }
    }
    
    // Update causal strengths
    for (const auto& [pair, lead_count] : lead_lag_counts_) {
        auto reverse_it = lead_lag_counts_.find({pair.second, pair.first});
        uint32_t lag_count = (reverse_it != lead_lag_counts_.end()) ? reverse_it->second : 0;
        
        causal_strengths_[pair] = compute_causal_strength_from_counts(lead_count, lag_count);
    }
}

void CausalMiner::update_temporal_distances(const std::vector<NodeID>& sequence) {
    for (size_t i = 0; i < sequence.size(); ++i) {
        for (size_t j = i + 1; j < sequence.size(); ++j) {
            double distance = static_cast<double>(j - i);
            temporal_distances_[{sequence[i], sequence[j]}].push_back(distance);
            temporal_distances_[{sequence[j], sequence[i]}].push_back(-distance);
        }
    }
}

double CausalMiner::compute_causal_strength_from_counts(uint32_t lead_count, uint32_t lag_count) const {
    if (lead_count + lag_count == 0) return 0.0;
    
    return static_cast<double>(lead_count) / (lead_count + lag_count);
}

bool CausalMiner::passes_causal_test(const std::pair<NodeID, NodeID>& pair) const {
    double strength = get_causal_strength(pair.first, pair.second);
    return strength >= 0.6 && strength <= 0.9; // Avoid perfect causality (likely spurious)
}

double CausalMiner::compute_granger_causality_score(const std::pair<NodeID, NodeID>& pair) const {
    // Simplified Granger causality test
    // In practice, you'd implement proper statistical tests
    
    auto it = temporal_distances_.find(pair);
    if (it == temporal_distances_.end()) return 0.0;
    
    const auto& distances = it->second;
    if (distances.empty()) return 0.0;
    
    // Count positive vs negative distances
    int positive_count = 0;
    int negative_count = 0;
    
    for (double distance : distances) {
        if (distance > 0) positive_count++;
        else if (distance < 0) negative_count++;
    }
    
    int total = positive_count + negative_count;
    if (total == 0) return 0.0;
    
    return static_cast<double>(positive_count) / total;
}

void CausalMiner::annotate_edge_with_causal_info(const EdgeID& edge, bool is_causal, double strength) {
    // This would update edge flags to indicate causal relationships
    // Implementation depends on the specific edge annotation system
}

// GraphMiner implementation
void GraphMiner::mine_from_sequence(const std::vector<NodeID>& sequence) {
    coordinate_mining_components(sequence);
    sequences_processed_++;
    update_mining_statistics();
}

void GraphMiner::mine_from_sequences(const std::vector<std::vector<NodeID>>& sequences) {
    for (const auto& sequence : sequences) {
        mine_from_sequence(sequence);
    }
}

void GraphMiner::run_mining_pass() {
    last_mining_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Update pattern statistics
    pattern_miner_.update_pattern_stats();
    
    // Create generalizations
    std::vector<Pattern> generalization_candidates = pattern_miner_.get_generalization_candidates();
    if (!generalization_candidates.empty()) {
        std::vector<Pattern> selected_candidates;
        size_t max_candidates = std::min(config_.max_generalizations_per_batch, generalization_candidates.size());
        
        for (size_t i = 0; i < max_candidates; ++i) {
            selected_candidates.push_back(generalization_candidates[i]);
        }
        
        generalization_creator_.create_generalization_nodes(selected_candidates);
    }
    
    // Create LEAP edges
    std::vector<std::pair<NodeID, NodeID>> leap_candidates = pmi_miner_.get_leap_candidates();
    if (!leap_candidates.empty()) {
        pmi_miner_.create_leap_edges(leap_candidates);
    }
    
    // Update causal annotations
    causal_miner_.update_causal_annotations();
    
    consolidate_mining_results();
}

std::vector<Pattern> GraphMiner::get_discovered_patterns() const {
    return pattern_miner_.get_patterns();
}

std::vector<NodeID> GraphMiner::get_generalization_nodes() const {
    // This would return created generalization nodes
    // Implementation depends on tracking created nodes
    return {};
}

std::vector<EdgeID> GraphMiner::get_leap_edges() const {
    // This would return created LEAP edges
    // Implementation depends on tracking created edges
    return {};
}

std::vector<std::pair<NodeID, NodeID>> GraphMiner::get_causal_relationships() const {
    return causal_miner_.get_causal_relationships();
}

void GraphMiner::set_config(const MiningConfig& config) {
    config_ = config;
    pattern_miner_.set_config(config);
    generalization_creator_.set_config(config);
    pmi_miner_.set_config(config);
    causal_miner_.set_config(config);
}

void GraphMiner::coordinate_mining_components(const std::vector<NodeID>& sequence) {
    // Mine patterns
    pattern_miner_.mine_sequence(sequence);
    
    // Update PMI statistics
    pmi_miner_.update_pmi_stats(sequence);
    
    // Mine causal relationships
    causal_miner_.mine_causal_relationships(sequence);
}

void GraphMiner::update_mining_statistics() {
    // Update internal statistics
    // This could include tracking mining performance, resource usage, etc.
}

void GraphMiner::consolidate_mining_results() {
    // Consolidate results from all mining components
    // This could include deduplication, validation, etc.
}

void GraphMiner::validate_mining_results() {
    // Validate mining results for consistency and quality
    // This could include checking for contradictions, low-quality patterns, etc.
}

// MiningUtils implementation
namespace MiningUtils {
    bool is_pattern_significant(const Pattern& pattern, const MiningConfig& config) {
        return pattern.frequency >= config.theta_pat &&
               pattern.confidence >= config.theta_div &&
               pattern.confidence >= config.min_generalization_confidence;
    }
    
    double compute_pattern_novelty(const Pattern& pattern, Store* store) {
        // Simple novelty computation based on pattern uniqueness
        // In practice, you'd want more sophisticated novelty measures
        
        if (pattern.sequence.empty()) return 0.0;
        
        // Check if constituent nodes are frequent
        bool all_frequent = true;
        for (const auto& node : pattern.sequence) {
            if (!node_is_frequent(node, store)) {
                all_frequent = false;
                break;
            }
        }
        
        // Novelty is higher if pattern combines frequent nodes in new ways
        return all_frequent ? 0.3 : 0.8;
    }
    
    bool patterns_are_similar(const Pattern& pattern1, const Pattern& pattern2) {
        if (pattern1.sequence.size() != pattern2.sequence.size()) return false;
        
        for (size_t i = 0; i < pattern1.sequence.size(); ++i) {
            if (pattern1.sequence[i] != pattern2.sequence[i]) {
                return false;
            }
        }
        
        return true;
    }
    
    bool node_is_frequent(const NodeID& node, Store* store, double threshold) {
        // Check node degree as proxy for frequency
        RelMask all_relations;
        all_relations.set_all();
        
        AdjView view;
        if (store->get_adj(node, all_relations, view)) {
            return view.count >= static_cast<size_t>(threshold * 100); // Arbitrary threshold
        }
        
        return false;
    }
    
    std::vector<NodeID> find_frequent_nodes(Store* store, double threshold) {
        // This would implement sophisticated node frequency analysis
        // For now, return empty vector
        return {};
    }
    
    bool edge_should_be_created(const NodeID& src, const NodeID& dst, Rel relation, double confidence) {
        return confidence >= 0.3 && !is_zero_id(src) && !is_zero_id(dst);
    }
    
    double compute_edge_creation_confidence(const NodeID& src, const NodeID& dst, Rel relation) {
        // Simple confidence computation
        // In practice, you'd want more sophisticated confidence measures
        
        switch (relation) {
            case Rel::EXACT:
                return 0.9; // High confidence for exact relations
            case Rel::TEMPORAL:
                return 0.7; // Medium confidence for temporal relations
            case Rel::LEAP:
                return 0.5; // Lower confidence for leap relations
            case Rel::GENERALIZATION:
                return 0.8; // High confidence for generalizations
            default:
                return 0.5;
        }
    }
    
    bool is_temporal_sequence(const std::vector<NodeID>& sequence, Store* store) {
        // Check if sequence has temporal characteristics
        // This is a simplified implementation
        
        if (sequence.size() < 2) return false;
        
        // Check for temporal edges between consecutive nodes
        for (size_t i = 0; i < sequence.size() - 1; ++i) {
            RelMask temporal_mask;
            temporal_mask.set(Rel::TEMPORAL);
            
            AdjView view;
            if (store->get_out_edges(sequence[i], temporal_mask, view)) {
                for (size_t j = 0; j < view.count; ++j) {
                    NodeID target_node;
                    std::copy(view.edges[j].dst, view.edges[j].dst + 32, target_node.begin());
                    
                    if (target_node == sequence[i + 1]) {
                        return true;
                    }
                }
            }
        }
        
        return false;
    }
    
    std::vector<double> compute_temporal_distances(const std::vector<NodeID>& sequence, Store* store) {
        std::vector<double> distances;
        
        for (size_t i = 0; i < sequence.size() - 1; ++i) {
            distances.push_back(1.0); // Simple unit distance
        }
        
        return distances;
    }
    
    bool should_create_generalization(const Pattern& pattern, const MiningConfig& config) {
        return pattern.frequency >= config.theta_pat &&
               pattern.confidence >= config.theta_div &&
               pattern.confidence >= config.min_generalization_confidence;
    }
    
    NodeType determine_best_generalization_type(const Pattern& pattern) {
        if (pattern.sequence.size() <= 2) {
            return NodeType::PHRASE;
        } else if (pattern.sequence.size() <= 4) {
            return NodeType::CONCEPT;
        } else {
            return NodeType::ABSTRACTION;
        }
    }
    
    double compute_entropy(const std::vector<double>& probabilities) {
        double entropy = 0.0;
        
        for (double prob : probabilities) {
            if (prob > 0.0) {
                entropy -= prob * std::log2(prob);
            }
        }
        
        return entropy;
    }
    
    double compute_mutual_information(const std::vector<std::vector<double>>& joint_probs) {
        // Simplified mutual information computation
        // In practice, you'd want more sophisticated implementation
        
        double mi = 0.0;
        
        for (size_t i = 0; i < joint_probs.size(); ++i) {
            for (size_t j = 0; j < joint_probs[i].size(); ++j) {
                double joint_prob = joint_probs[i][j];
                if (joint_prob > 0.0) {
                    // Simplified computation - in practice you'd need marginal probabilities
                    mi += joint_prob * std::log2(joint_prob + 1.0);
                }
            }
        }
        
        return mi;
    }
    
    double compute_chi_square_statistic(const std::vector<std::vector<uint32_t>>& observed) {
        // Simplified chi-square computation
        // In practice, you'd want complete implementation with expected values
        
        double chi_square = 0.0;
        
        for (const auto& row : observed) {
            for (uint32_t obs : row) {
                // Simplified - would need expected values for proper computation
                chi_square += obs * obs;
            }
        }
        
        return chi_square;
    }
    
    std::string nodes_to_string(const std::vector<NodeID>& nodes, Store* store) {
        std::ostringstream oss;
        
        for (size_t i = 0; i < nodes.size(); ++i) {
            if (i > 0) oss << " ";
            
            NodeRecHeader header;
            std::vector<uint8_t> payload;
            if (store->get_node(nodes[i], header, payload)) {
                std::string content(reinterpret_cast<const char*>(payload.data()), payload.size());
                oss << content;
            } else {
                oss << "<unknown>";
            }
        }
        
        return oss.str();
    }
    
    std::vector<std::string> extract_keywords(const std::string& text) {
        std::vector<std::string> keywords;
        std::istringstream iss(text);
        std::string word;
        
        while (iss >> word) {
            // Simple keyword extraction - remove punctuation
            word.erase(std::remove_if(word.begin(), word.end(), ::ispunct), word.end());
            if (!word.empty()) {
                keywords.push_back(word);
            }
        }
        
        return keywords;
    }
    
    std::string generate_pattern_label(const Pattern& pattern, Store* store) {
        return nodes_to_string(pattern.sequence, store);
    }
    
    double compute_pattern_diversity(const Pattern& pattern) {
        // Simple diversity computation based on context variety
        if (pattern.contexts.empty()) return 0.0;
        
        std::unordered_set<std::string> unique_contexts(pattern.contexts.begin(), pattern.contexts.end());
        return static_cast<double>(unique_contexts.size()) / pattern.contexts.size();
    }
}

} // namespace melvin
