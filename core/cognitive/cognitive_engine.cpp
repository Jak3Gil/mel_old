/**
 * cognitive_engine.cpp
 * 
 * Implementation of MELVIN's Unified Cognitive Architecture
 */

#include "cognitive_engine.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <cctype>

namespace melvin {
namespace cognitive {

CognitiveEngine::CognitiveEngine(GraphStorage& g, reasoning::UnifiedReasoningEngine& e, evolution::Genome* genome)
    : graph(g), engine(e), genome_(genome), gen(std::random_device{}()) {
    
    // Initialize with reasonable defaults
    state.current_goal = Goal::UNDERSTAND;
    state.goal_duration = 10;
    state.exploration_rate = 0.3f;
    state.quality_threshold = 0.3f;
    state.boredom_threshold = 3;
    
    // If genome is provided, load parameters from it
    if (genome_) {
        sync_from_genome();
        std::cout << "🧬 Cognitive engine connected to genome!" << std::endl;
    }
}

// ============================================================================
// INPUT PROCESSING
// ============================================================================

bool CognitiveEngine::process_input(const std::vector<int>& nodes, const std::string& modality) {
    if (nodes.empty()) return false;
    
    state.total_processed++;
    
    // MECHANISM 1: Evaluate quality
    auto quality = evaluate_quality(nodes);
    
    // Reject truly terrible input (but threshold is low!)
    if (quality.overall() < state.quality_threshold) {
        state.rejected_count++;
        return false;
    }
    
    state.high_quality_count++;
    
    // Process through reasoning engine
    auto embeddings = graph.get_embeddings();
    engine.process_input(nodes, embeddings, modality);
    
    // Let activation spread
    for (int i = 0; i < 3; i++) {
        engine.activation_field().tick(graph.edges);
    }
    
    // MECHANISM 5: Update salience
    update_salience(nodes, quality.overall());
    
    // Track for consolidation
    recent_episodes.push_back(nodes);
    
    // Consolidate periodically
    if (recent_episodes.size() >= 5) {
        consolidate();
    }
    
    return true;
}

CognitiveEngine::QualityScore CognitiveEngine::evaluate_quality(const std::vector<int>& nodes) {
    QualityScore score;
    
    if (nodes.empty()) {
        return score;  // Returns 0.5 defaults
    }
    
    // Coherence: token quality + diversity
    float avg_token_quality = 0.0f;
    for (int node_id : nodes) {
        auto token_it = graph.id_to_token.find(node_id);
        if (token_it != graph.id_to_token.end()) {
            avg_token_quality += score_token_quality(token_it->second);
        }
    }
    avg_token_quality /= nodes.size();
    
    std::unordered_set<int> unique(nodes.begin(), nodes.end());
    float diversity = (float)unique.size() / nodes.size();
    
    score.coherence = (avg_token_quality * 0.7f + diversity * 0.3f);
    
    // Relevance: how connected are these nodes? (proxy for meaningfulness)
    float avg_connectivity = 0.0f;
    for (int node_id : nodes) {
        auto edge_it = graph.edges.find(node_id);
        if (edge_it != graph.edges.end()) {
            avg_connectivity += std::min(1.0f, (float)edge_it->second.size() / 100.0f);
        }
    }
    avg_connectivity /= nodes.size();
    score.relevance = avg_connectivity;
    
    // Novelty: how many of these have we seen recently?
    size_t h = hash_sequence(nodes);
    int seen_count = thought_history[h];
    score.novelty = 1.0f - std::min(1.0f, seen_count / 3.0f);
    
    return score;
}

float CognitiveEngine::score_token_quality(const std::string& token) {
    if (token.empty()) return 0.0f;
    
    float quality = 0.5f;  // Baseline
    
    // Reasonable length
    if (token.length() >= 3 && token.length() <= 15) quality += 0.2f;
    
    // Character analysis
    int alpha_count = 0;
    int digit_count = 0;
    int special_count = 0;
    
    for (char c : token) {
        if (std::isalpha(c)) alpha_count++;
        else if (std::isdigit(c)) digit_count++;
        else special_count++;
    }
    
    // Prefer mostly letters
    float alpha_ratio = (float)alpha_count / token.length();
    if (alpha_ratio > 0.7f) quality += 0.3f;
    else if (alpha_ratio > 0.4f) quality += 0.1f;
    
    // Penalize noise
    if (digit_count > 3 || special_count > 3) quality -= 0.3f;
    
    // Penalize HTML artifacts
    if (token.find("quot") != std::string::npos ||
        token.find("align") != std::string::npos ||
        token.find("nbsp") != std::string::npos ||
        token.find("bgcolor") != std::string::npos) {
        quality -= 0.5f;
    }
    
    return std::max(0.0f, std::min(1.0f, quality));
}

// ============================================================================
// THINKING & GENERATION
// ============================================================================

std::vector<int> CognitiveEngine::think() {
    auto embeddings = graph.get_embeddings();
    
    // MECHANISM 3 & 6: Exploration + stochastic temperature
    float temperature = get_temperature();
    if (should_explore()) {
        temperature += 0.4f;
    }
    
    // GENOME-CONTROLLED OUTPUT LENGTH!
    int length_min = 20;  // Default
    int length_max = 100; // Default
    int length_variance = 30;
    
    if (genome_) {
        length_min = (int)genome_->get("output_length_min");
        length_max = (int)genome_->get("output_length_max");
        length_variance = (int)genome_->get("output_length_variance");
    }
    
    // Generate variable length within range
    std::uniform_int_distribution<int> length_dist(0, length_variance);
    int length = length_min + length_dist(gen);
    length = std::min(length, length_max);
    
    // Get current active nodes as context
    auto active = engine.activation_field().get_active_nodes(0.1f);
    std::vector<int> context;
    for (const auto& [node_id, _] : active) {
        context.push_back(node_id);
        if (context.size() >= 5) break;
    }
    
    // If no context, select quality nodes
    if (context.empty()) {
        context = select_quality_nodes(3);
    }
    
    // Generate
    auto generated = engine.generate_output(
        context,
        graph.edges,
        embeddings,
        temperature,
        length
    );
    
    // MECHANISM 2: Check for boredom
    if (is_bored_of(generated)) {
        // Try again with more exploration
        state.bored_count++;
        state.exploration_rate = std::min(0.8f, state.exploration_rate + 0.2f);
        context = select_quality_nodes(3);  // Fresh start
        generated = engine.generate_output(context, graph.edges, embeddings, temperature + 0.5f, length);
    }
    
    // Evaluate and adapt
    auto quality = evaluate_quality(generated);
    adapt_parameters(quality.overall());
    
    // ADAPTIVE INTELLIGENCE: Evaluate adaptation quality
    evaluate_adaptation();
    
    // HUMAN-LEVEL v4.0: Integrate all advanced systems
    update_self_model();
    compute_intrinsic_motivation();
    broadcast_global_workspace();
    update_multi_timescale_memory(generated);
    
    // Update world model with what we generated
    if (!context.empty()) {
        engine.predict_future_state(context);
        engine.update_world_model(generated, 0.1f);
    }
    
    // Apply affective neuromodulation
    if (genome_) {
        const auto& reasoning_metrics = engine.get_metrics();
        genome_->apply_affective_modulation(
            reasoning_metrics.success_rate,
            reasoning_metrics.coherence,
            reasoning_metrics.novelty
        );
    }
    
    // Sleep cycle
    cycles_since_sleep_++;
    if (cycles_since_sleep_ >= sleep_interval_ && self_model_.energy_budget < 0.3f) {
        sleep_and_replay();
    }
    
    // Sync back to genome if learning is enabled
    if (genome_ && state.total_processed % 100 == 0) {
        sync_to_genome();
    }
    
    return generated;
}

std::vector<int> CognitiveEngine::generate_response(int max_length) {
    // Similar to think() but with goal bias
    update_goal();  // MECHANISM 4
    
    return think();  // For now, same as think - can add goal-specific logic later
}

std::vector<reasoning::PredictionResult> CognitiveEngine::predict_next(
    const std::vector<int>& context,
    int top_k
) {
    auto embeddings = graph.get_embeddings();
    return engine.predict_next(
        context,
        graph.edges,
        embeddings,
        top_k,
        reasoning::Predictor::Mode::HYBRID
    );
}

// ============================================================================
// LEARNING & CONSOLIDATION
// ============================================================================

void CognitiveEngine::consolidate() {
    if (recent_episodes.empty()) return;
    
    // Get activation history
    auto activation_history = engine.activation_field().get_active_nodes(0.0f);
    
    // Consolidate with Hebbian learning
    consolidator.consolidate(graph.edges, activation_history, recent_episodes);
    
    recent_episodes.clear();
}

void CognitiveEngine::update_salience(const std::vector<int>& nodes, float quality) {
    // MECHANISM 5: High quality experiences increase salience
    for (int node_id : nodes) {
        concept_salience[node_id] += quality * 0.1f;
        
        // Cap at 1.0
        if (concept_salience[node_id] > 1.0f) {
            concept_salience[node_id] = 1.0f;
        }
    }
}

// ============================================================================
// COGNITIVE MECHANISMS
// ============================================================================

bool CognitiveEngine::is_bored_of(const std::vector<int>& thought) {
    // MECHANISM 2: Boredom/habituation
    size_t h = hash_sequence(thought);
    thought_history[h]++;
    return thought_history[h] >= state.boredom_threshold;
}

bool CognitiveEngine::should_explore() {
    // MECHANISM 3: Exploration vs exploitation
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    return dist(gen) < state.exploration_rate;
}

void CognitiveEngine::update_goal() {
    // MECHANISM 4: Goal-directed processing
    state.goal_duration--;
    
    if (state.goal_duration <= 0) {
        // Pick new goal
        static const std::vector<Goal> goals = {
            Goal::UNDERSTAND, Goal::CREATE, Goal::LEARN,
            Goal::EXPLORE, Goal::CONNECT, Goal::REASON
        };
        
        std::uniform_int_distribution<> dist(0, goals.size() - 1);
        state.current_goal = goals[dist(gen)];
        state.goal_duration = 5 + (std::uniform_int_distribution<int>(0, 10)(gen));
    }
}

void CognitiveEngine::set_goal(Goal g, int duration) {
    state.current_goal = g;
    state.goal_duration = duration;
}

float CognitiveEngine::get_temperature() {
    // MECHANISM 6: Stochastic processing
    std::uniform_real_distribution<float> dist(0.6f, 1.4f);
    return dist(gen);
}

// ============================================================================
// NODE SELECTION
// ============================================================================

std::vector<int> CognitiveEngine::select_quality_nodes(int count) {
    std::vector<int> candidate_nodes;
    std::vector<float> weights;
    
    bool exploring = should_explore();
    
    // Score all nodes
    for (const auto& [node_id, edge_list] : graph.edges) {
        if (edge_list.size() < 5) continue;  // Must have some connections
        
        float score = score_node(node_id, exploring);
        
        candidate_nodes.push_back(node_id);
        weights.push_back(score);
    }
    
    if (candidate_nodes.empty()) {
        return {};
    }
    
    // Weighted random selection
    std::discrete_distribution<> dist(weights.begin(), weights.end());
    
    std::vector<int> selected;
    for (int i = 0; i < count && !candidate_nodes.empty(); i++) {
        int idx = dist(gen);
        selected.push_back(candidate_nodes[idx]);
    }
    
    return selected;
}

float CognitiveEngine::score_node(int node_id, bool exploring) {
    float score = 1.0f;
    
    // Connectivity
    auto edge_it = graph.edges.find(node_id);
    if (edge_it != graph.edges.end()) {
        score += std::min(1.0f, (float)edge_it->second.size() / 100.0f);
    }
    
    // Token quality
    auto token_it = graph.id_to_token.find(node_id);
    if (token_it != graph.id_to_token.end()) {
        score += score_token_quality(token_it->second);
    }
    
    // Salience (learned importance)
    if (concept_salience.find(node_id) != concept_salience.end()) {
        score += concept_salience[node_id];
    }
    
    // Exploration boost
    if (exploring) {
        std::uniform_real_distribution<float> boost(0.0f, 2.0f);
        score += boost(gen);
    }
    
    return score;
}

// ============================================================================
// ADAPTATION
// ============================================================================

void CognitiveEngine::adapt_parameters(float quality) {
    // Adapt exploration rate based on quality
    if (quality > 0.7f) {
        // Good thoughts = exploit more
        state.exploration_rate = std::max(0.1f, state.exploration_rate - 0.02f);
    } else {
        // Mediocre thoughts = explore more
        state.exploration_rate = std::min(0.5f, state.exploration_rate + 0.02f);
    }
}

// ============================================================================
// STATE & STATISTICS
// ============================================================================

void CognitiveEngine::print_statistics() const {
    std::cout << "\n📊 Cognitive System Statistics:" << std::endl;
    std::cout << "   Total processed: " << state.total_processed << std::endl;
    std::cout << "   High quality: " << state.high_quality_count 
              << " (" << (100.0f * state.high_quality_count / std::max(1, state.total_processed)) << "%)" << std::endl;
    std::cout << "   Rejected: " << state.rejected_count << std::endl;
    std::cout << "   Bored (repetitive): " << state.bored_count << std::endl;
    std::cout << "   Current goal: " << goal_to_string(state.current_goal) << std::endl;
    std::cout << "   Exploration rate: " << state.exploration_rate << std::endl;
    std::cout << "   Concepts tracked: " << concept_salience.size() << std::endl;
    
    // ADAPTIVE INTELLIGENCE: Show adaptation metrics
    std::cout << "\n🧠 Adaptive Intelligence Metrics:" << std::endl;
    std::cout << "   Total thinking cycles: " << total_thinking_cycles_ << std::endl;
    std::cout << "   Adaptation Quality Score (AQS): " << adaptation_metrics_.aqs << std::endl;
    std::cout << "   Coherence gain: " << adaptation_metrics_.coherence_gain << std::endl;
    std::cout << "   Novelty gain: " << adaptation_metrics_.novelty_gain << std::endl;
    std::cout << "   Goal success: " << adaptation_metrics_.goal_success << std::endl;
    std::cout << "   Energy cost: " << adaptation_metrics_.energy_cost << std::endl;
    std::cout << "   Avg success rate: " << adaptation_metrics_.avg_success_rate << std::endl;
    std::cout << "   Avg surprise rate: " << adaptation_metrics_.avg_surprise_rate << std::endl;
    
    // Show genome phase if connected
    if (genome_) {
        std::cout << "\n🧬 Genome Phase: " << genome_->get_phase_name() << std::endl;
        std::cout << "   Base learning rate: " << genome_->get("base_learning_rate") << std::endl;
        std::cout << "   Temperature range: [" << genome_->get("temperature_min") 
                  << ", " << genome_->get("temperature_max") << "]" << std::endl;
    }
    
    // Show reasoning engine metrics
    const auto& reasoning_metrics = engine.get_metrics();
    std::cout << "\n⚡ Reasoning Engine Metrics:" << std::endl;
    std::cout << "   Confidence: " << reasoning_metrics.confidence << std::endl;
    std::cout << "   Coherence: " << reasoning_metrics.coherence << std::endl;
    std::cout << "   Novelty: " << reasoning_metrics.novelty << std::endl;
    std::cout << "   Entropy: " << reasoning_metrics.entropy << std::endl;
    std::cout << "   Success rate: " << reasoning_metrics.success_rate << std::endl;
    
    // HUMAN-LEVEL v4.0: Show advanced intelligence metrics
    std::cout << "\n🌟 Self-Model (Self-Awareness):" << std::endl;
    std::cout << "   Confidence: " << self_model_.confidence << std::endl;
    std::cout << "   Emotion valence: " << self_model_.emotion_valence << std::endl;
    std::cout << "   Curiosity level: " << self_model_.curiosity_level << std::endl;
    std::cout << "   Energy budget: " << self_model_.energy_budget << std::endl;
    std::cout << "   Arousal: " << self_model_.arousal << std::endl;
    std::cout << "   Focus: " << self_model_.focus << std::endl;
    
    std::cout << "\n💎 Intrinsic Motivation:" << std::endl;
    std::cout << "   Curiosity drive: " << intrinsic_motivation_.curiosity << std::endl;
    std::cout << "   Stability drive: " << intrinsic_motivation_.stability << std::endl;
    std::cout << "   Empathy drive: " << intrinsic_motivation_.empathy << std::endl;
    std::cout << "   Intrinsic reward: " << intrinsic_motivation_.intrinsic_reward << std::endl;
    std::cout << "   Meta-value (V): " << intrinsic_motivation_.meta_value << std::endl;
    
    std::cout << "\n🌍 World Model:" << std::endl;
    const auto& world_model = engine.get_world_model();
    std::cout << "   Prediction error: " << world_model.recent_prediction_error << std::endl;
    std::cout << "   Prediction accuracy: " << world_model.avg_prediction_accuracy << std::endl;
    std::cout << "   Causal edges learned: " << world_model.causal_edges.size() << std::endl;
    
    std::cout << "\n🧬 Neuromodulators:" << std::endl;
    if (genome_) {
        const auto& neuro = genome_->get_neuromodulators();
        std::cout << "   Dopamine: " << neuro.dopamine << std::endl;
        std::cout << "   Serotonin: " << neuro.serotonin << std::endl;
        std::cout << "   Noradrenaline: " << neuro.noradrenaline << std::endl;
        std::cout << "   Acetylcholine: " << neuro.acetylcholine << std::endl;
    }
    
    std::cout << "\n💾 Multi-Timescale Memory:" << std::endl;
    std::cout << "   Short-term episodes: " << multi_timescale_memory_.short_term_episodes.size() << std::endl;
    std::cout << "   Mid-term episodes: " << multi_timescale_memory_.mid_term_episodes.size() << std::endl;
    std::cout << "   Cycles since sleep: " << cycles_since_sleep_ << std::endl;
    
    std::cout << "\n🔷 Symbolic Concepts:" << std::endl;
    std::cout << "   Formed concepts: " << symbolic_concepts_.size() << std::endl;
    
    std::cout << "\n👁️  Global Workspace:" << std::endl;
    std::cout << "   Focus nodes: " << global_workspace_.focus_nodes.size() << std::endl;
    std::cout << "   Active: " << (global_workspace_.is_active ? "YES" : "NO") << std::endl;
}

// ============================================================================
// UTILITIES
// ============================================================================

std::string CognitiveEngine::nodes_to_string(const std::vector<int>& nodes, int max_len) const {
    std::string result;
    int len = 0;
    
    for (size_t i = 0; i < nodes.size(); i++) {
        auto token_it = graph.id_to_token.find(nodes[i]);
        if (token_it != graph.id_to_token.end()) {
            result += token_it->second;
            len += token_it->second.length();
            
            if (i < nodes.size() - 1) {
                result += " → ";
                len += 3;
            }
            
            if (len > max_len) {
                result += "...";
                break;
            }
        }
    }
    
    return result;
}

std::string CognitiveEngine::goal_to_string(Goal g) const {
    switch (g) {
        case Goal::UNDERSTAND: return "understand";
        case Goal::CREATE: return "create";
        case Goal::LEARN: return "learn";
        case Goal::EXPLORE: return "explore";
        case Goal::CONNECT: return "connect";
        case Goal::REASON: return "reason";
        case Goal::LISTEN: return "listen";
        case Goal::RECOGNIZE: return "recognize";
        default: return "unknown";
    }
}

size_t CognitiveEngine::hash_sequence(const std::vector<int>& seq) {
    size_t h = 0;
    for (int id : seq) {
        h ^= std::hash<int>{}(id) + 0x9e3779b9 + (h << 6) + (h >> 2);
    }
    return h;
}

// ============================================================================
// GENOME SYNCHRONIZATION
// ============================================================================

void CognitiveEngine::sync_from_genome() {
    if (!genome_) return;
    
    // Load all cognitive parameters from genome
    state.quality_threshold = genome_->get("quality_threshold");
    state.boredom_threshold = (int)genome_->get("boredom_threshold");
    state.exploration_rate = genome_->get("exploration_rate");
    
    // Note: Other parameters like output_length are read directly in think()
    // This allows for dynamic adjustment during runtime
}

void CognitiveEngine::sync_to_genome() {
    if (!genome_) return;
    
    // Save learned cognitive parameters back to genome
    // This allows the system to evolve better thinking strategies!
    genome_->set("quality_threshold", state.quality_threshold);
    genome_->set("boredom_threshold", (float)state.boredom_threshold);
    genome_->set("exploration_rate", state.exploration_rate);
    
    // The genome can then mutate and evolve these values
}

// ==============================================================================
// ADAPTIVE INTELLIGENCE: Adaptation Quality Score & Evaluation
// ==============================================================================

float CognitiveEngine::compute_aqs() {
    // AQS = (0.5 * coherence_gain + 0.3 * novelty_gain + 0.2 * goal_success)
    //       / (energy_cost + 1e-3)
    // 
    // This measures HOW EFFICIENTLY we're adapting:
    // - High AQS = intelligent adaptation (learning efficiently)
    // - Low AQS = poor adaptation (slow/unstable learning)
    
    float numerator = (0.5f * adaptation_metrics_.coherence_gain +
                      0.3f * adaptation_metrics_.novelty_gain +
                      0.2f * adaptation_metrics_.goal_success);
    
    float denominator = adaptation_metrics_.energy_cost + 1e-3f;
    
    adaptation_metrics_.aqs = numerator / denominator;
    
    return adaptation_metrics_.aqs;
}

void CognitiveEngine::evaluate_adaptation() {
    total_thinking_cycles_++;
    adaptation_metrics_.cycles_since_update++;
    
    // Update every 10 cycles
    if (adaptation_metrics_.cycles_since_update < 10) {
        return;
    }
    
    // Get current reasoning metrics from engine
    const auto& reasoning_metrics = engine.get_metrics();
    
    // Compute gains
    adaptation_metrics_.coherence_gain = reasoning_metrics.coherence - 
                                          adaptation_metrics_.last_coherence;
    adaptation_metrics_.novelty_gain = reasoning_metrics.novelty - 
                                        adaptation_metrics_.last_novelty;
    
    // Estimate goal success (high if confidence is high and improving)
    bool confidence_improving = (reasoning_metrics.confidence > 
                                 reasoning_metrics.last_confidence);
    adaptation_metrics_.goal_success = confidence_improving ? 0.8f : 0.3f;
    
    // Estimate energy cost (from activation field entropy)
    // High entropy = high energy cost
    adaptation_metrics_.energy_cost = reasoning_metrics.entropy + 0.1f;
    
    // Compute AQS
    float aqs = compute_aqs();
    
    // ========================================================================
    // ADAPTIVE RESPONSE: Adjust parameters based on AQS
    // ========================================================================
    
    if (aqs < 0.1f) {
        // POOR ADAPTATION: Too slow or unstable
        std::cout << "⚠️  Poor adaptation detected (AQS: " << aqs << ")" << std::endl;
        
        // Stabilize: reduce temperature, lower learning rate
        if (genome_) {
            float current_temp_max = genome_->get("temperature_max");
            genome_->set("temperature_max", current_temp_max * 0.9f);
            
            float current_learning = genome_->get("base_learning_rate");
            genome_->set("base_learning_rate", current_learning * 0.8f);
        }
        
        // Increase quality threshold (be more selective)
        state.quality_threshold = std::min(0.7f, state.quality_threshold + 0.05f);
        
    } else if (aqs > 0.3f) {
        // INTELLIGENT ADAPTATION: Learning efficiently!
        std::cout << "✨ Intelligent adaptation (AQS: " << aqs << ")" << std::endl;
        
        // Encourage: slightly increase learning rate
        if (genome_) {
            float current_learning = genome_->get("base_learning_rate");
            genome_->set("base_learning_rate", std::min(0.5f, current_learning * 1.05f));
        }
        
        // Can afford to be more exploratory
        state.exploration_rate = std::min(0.8f, state.exploration_rate + 0.02f);
    }
    
    // ========================================================================
    // Update running averages for genome phase control
    // ========================================================================
    
    float success = (reasoning_metrics.confidence > 0.6f) ? 1.0f : 0.0f;
    float surprise = reasoning_metrics.novelty;
    
    adaptation_metrics_.avg_success_rate = 0.9f * adaptation_metrics_.avg_success_rate + 
                                           0.1f * success;
    adaptation_metrics_.avg_surprise_rate = 0.9f * adaptation_metrics_.avg_surprise_rate + 
                                            0.1f * surprise;
    
    // ========================================================================
    // Update genome phase (EXPLORE/REFINE/EXPLOIT) every 100 cycles
    // ========================================================================
    
    if (genome_ && total_thinking_cycles_ % 100 == 0) {
        genome_->update_phase(adaptation_metrics_.avg_success_rate,
                             adaptation_metrics_.avg_surprise_rate);
        
        // Sync parameters from genome after phase change
        sync_from_genome();
    }
    
    // ========================================================================
    // Update reasoning engine metrics
    // ========================================================================
    
    engine.update_metrics(reasoning_metrics.confidence,
                         reasoning_metrics.coherence,
                         reasoning_metrics.novelty);
    
    // Store for next iteration
    adaptation_metrics_.last_coherence = reasoning_metrics.coherence;
    adaptation_metrics_.last_novelty = reasoning_metrics.novelty;
    adaptation_metrics_.cycles_since_update = 0;
}

// ==============================================================================
// HUMAN-LEVEL ADAPTIVE INTELLIGENCE v4.0: Complete Implementation
// ==============================================================================

void CognitiveEngine::update_self_model() {
    // Update self-awareness based on recent performance
    const auto& reasoning_metrics = engine.get_metrics();
    
    // Confidence tracks how well we're performing
    self_model_.confidence = 0.9f * self_model_.confidence + 0.1f * reasoning_metrics.confidence;
    
    // Emotion valence: positive when succeeding, negative when failing
    float success_signal = (reasoning_metrics.success_rate > 0.5f) ? 1.0f : -1.0f;
    self_model_.emotion_valence = 0.9f * self_model_.emotion_valence + 0.1f * success_signal;
    
    // Curiosity driven by prediction error and novelty
    float prediction_error = engine.get_world_model().recent_prediction_error;
    self_model_.curiosity_level = 0.8f * reasoning_metrics.novelty + 0.2f * prediction_error;
    
    // Energy budget depletes with thinking, replenishes during rest
    self_model_.energy_budget *= 0.999f;  // Slow depletion
    if (is_sleeping_) {
        self_model_.energy_budget = std::min(1.0f, self_model_.energy_budget + 0.1f);
    }
    
    // Arousal tracks activation level
    auto active_nodes = engine.activation_field().get_active_nodes(0.05f);
    self_model_.arousal = std::min(1.0f, static_cast<float>(active_nodes.size()) / 100.0f);
    
    // Focus is inverse of entropy (narrow when low entropy)
    self_model_.focus = 1.0f - std::min(1.0f, reasoning_metrics.entropy);
}

void CognitiveEngine::compute_intrinsic_motivation() {
    // Intrinsic rewards drive behavior even without external rewards
    const auto& reasoning_metrics = engine.get_metrics();
    
    // Curiosity reward: high when prediction error is high
    float prediction_error = engine.get_world_model().recent_prediction_error;
    intrinsic_motivation_.curiosity = prediction_error * reasoning_metrics.novelty;
    
    // Stability reward: high when coherent and low entropy
    intrinsic_motivation_.stability = reasoning_metrics.coherence * (1.0f - reasoning_metrics.entropy / 2.0f);
    
    // Empathy reward: alignment with expected communication patterns
    // (Proxy: quality of generated thoughts)
    float quality = evaluate_quality(engine.activation_field().get_high_energy_nodes(0.2f)).overall();
    intrinsic_motivation_.empathy = quality * self_model_.social_alignment;
    
    // Combined intrinsic reward
    intrinsic_motivation_.intrinsic_reward = 
        0.4f * intrinsic_motivation_.curiosity +
        0.4f * intrinsic_motivation_.stability +
        0.2f * intrinsic_motivation_.empathy;
    
    // Meta-value combines intrinsic and task rewards
    intrinsic_motivation_.meta_value = 
        intrinsic_motivation_.intrinsic_reward + 
        intrinsic_motivation_.task_reward -
        (1.0f - self_model_.energy_budget);  // Cost of depletion
}

float CognitiveEngine::compute_value_function() {
    // Differentiable value function for gradient-based optimization
    const auto& reasoning_metrics = engine.get_metrics();
    
    float alpha = 0.3f;  // Coherence weight
    float beta = 0.2f;   // Novelty weight
    float gamma = 0.3f;  // Goal success weight
    float delta = 0.2f;  // Energy cost weight
    
    float V = alpha * reasoning_metrics.coherence +
              beta * reasoning_metrics.novelty +
              gamma * adaptation_metrics_.goal_success -
              delta * (1.0f - self_model_.energy_budget);
    
    return V;
}

void CognitiveEngine::broadcast_global_workspace() {
    // Global workspace broadcasts salient information to all subsystems
    total_thinking_cycles_++;
    global_workspace_.last_broadcast_time = total_thinking_cycles_;
    
    if (!global_workspace_.is_active) return;
    
    // Select top salient nodes for broadcast
    auto active_nodes = engine.activation_field().get_active_nodes(0.1f);
    
    // Update salience map
    for (const auto& [node_id, activation] : active_nodes) {
        float salience = activation * concept_salience[node_id];
        global_workspace_.salience_map[node_id] = salience;
    }
    
    // Sort by salience and take top 10 for focus
    std::vector<std::pair<int, float>> salience_vec(
        global_workspace_.salience_map.begin(),
        global_workspace_.salience_map.end()
    );
    std::sort(salience_vec.begin(), salience_vec.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    global_workspace_.focus_nodes.clear();
    int max_focus = std::min(10, static_cast<int>(salience_vec.size()));
    for (int i = 0; i < max_focus; i++) {
        global_workspace_.focus_nodes.push_back(salience_vec[i].first);
    }
    
    // Broadcast affects: update prediction, consolidation, etc.
    if (!global_workspace_.focus_nodes.empty()) {
        engine.predict_future_state(global_workspace_.focus_nodes);
    }
}

void CognitiveEngine::update_multi_timescale_memory(const std::vector<int>& episode) {
    // Short-term memory (fast, volatile)
    multi_timescale_memory_.short_term_episodes.push_back(episode);
    if (multi_timescale_memory_.short_term_episodes.size() > 
        static_cast<size_t>(multi_timescale_memory_.short_term_capacity)) {
        multi_timescale_memory_.short_term_episodes.erase(
            multi_timescale_memory_.short_term_episodes.begin()
        );
    }
    
    // Periodically promote to mid-term
    multi_timescale_memory_.consolidation_counter++;
    if (multi_timescale_memory_.consolidation_counter % 10 == 0) {
        consolidate_timescale_memories();
    }
}

void CognitiveEngine::consolidate_timescale_memories() {
    // Promote significant short-term episodes to mid-term
    for (const auto& episode : multi_timescale_memory_.short_term_episodes) {
        float quality = evaluate_quality(episode).overall();
        
        if (quality > 0.5f) {  // Only consolidate high-quality episodes
            multi_timescale_memory_.mid_term_episodes.push_back(episode);
        }
    }
    
    // Trim mid-term if too large
    if (multi_timescale_memory_.mid_term_episodes.size() > 
        static_cast<size_t>(multi_timescale_memory_.mid_term_capacity)) {
        // Keep most recent
        multi_timescale_memory_.mid_term_episodes.erase(
            multi_timescale_memory_.mid_term_episodes.begin()
        );
    }
    
    // Long-term consolidation happens during sleep
}

void CognitiveEngine::sleep_and_replay() {
    // Offline consolidation through replay
    is_sleeping_ = true;
    
    std::cout << "💤 Entering sleep mode for consolidation..." << std::endl;
    
    // Replay mid-term memories to strengthen important patterns
    for (const auto& episode : multi_timescale_memory_.mid_term_episodes) {
        // Reactivate the episode
        for (int node_id : episode) {
            engine.activation_field().activate(node_id, 0.5f);
        }
        
        // Let activation spread
        engine.activation_field().tick(graph.edges);
    }
    
    // Consolidate all replayed memories
    auto activation_history = engine.activation_field().get_active_nodes(0.01f);
    std::deque<reasoning::Experience> experiences;  // Empty for now (can be populated if needed)
    auto embeddings = graph.get_embeddings();
    consolidator.consolidate_full(
        graph.edges,
        embeddings,
        activation_history,
        experiences,
        multi_timescale_memory_.long_term_learning_rate
    );
    
    // Form abstractions during sleep
    form_symbolic_abstractions();
    
    // Renormalize and prune
    // (Consolidator already handles this)
    
    is_sleeping_ = false;
    cycles_since_sleep_ = 0;
    
    std::cout << "☀️  Waking from sleep - memories consolidated" << std::endl;
}

void CognitiveEngine::form_symbolic_abstractions() {
    // Create higher-level concepts from stable clusters
    auto active_nodes = engine.activation_field().get_active_nodes(0.3f);
    
    if (active_nodes.size() < 3) return;  // Need at least 3 nodes
    
    // Group highly co-active nodes
    std::vector<int> cluster_nodes;
    for (const auto& [node_id, activation] : active_nodes) {
        if (activation > 0.5f) {
            cluster_nodes.push_back(node_id);
        }
    }
    
    if (cluster_nodes.size() >= 3) {
        // Check cluster stability (has it appeared before?)
        size_t cluster_hash = hash_sequence(cluster_nodes);
        
        if (thought_history[cluster_hash] > 3) {  // Seen at least 3 times
            // Form symbolic concept
            std::string concept_name = "concept_" + std::to_string(cluster_hash);
            
            if (symbolic_concepts_.find(concept_name) == symbolic_concepts_.end()) {
                // Create new concept node (use hash as ID)
                int concept_node_id = static_cast<int>(cluster_hash % 1000000);
                symbolic_concepts_[concept_name] = concept_node_id;
                concept_members_[concept_node_id] = cluster_nodes;
                
                std::cout << "🔷 Formed symbolic concept: " << concept_name 
                          << " from " << cluster_nodes.size() << " nodes" << std::endl;
            }
        }
    }
}

std::vector<int> CognitiveEngine::creative_compress(const std::vector<std::vector<int>>& patterns) {
    // Find minimal predictor for multiple patterns (creative synthesis)
    std::unordered_map<int, int> node_frequency;
    
    // Count node appearances across patterns
    for (const auto& pattern : patterns) {
        for (int node_id : pattern) {
            node_frequency[node_id]++;
        }
    }
    
    // Select nodes that appear in multiple patterns
    std::vector<std::pair<int, int>> common_nodes;
    for (const auto& [node_id, count] : node_frequency) {
        if (count >= 2) {  // Appears in at least 2 patterns
            common_nodes.emplace_back(node_id, count);
        }
    }
    
    // Sort by frequency
    std::sort(common_nodes.begin(), common_nodes.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Return minimal compressed representation
    std::vector<int> compressed;
    int max_compression = std::min(5, static_cast<int>(common_nodes.size()));
    for (int i = 0; i < max_compression; i++) {
        compressed.push_back(common_nodes[i].first);
    }
    
    return compressed;
}

void CognitiveEngine::import_social_trace(const std::vector<int>& trace, float trust_score) {
    // Social learning: import knowledge from external sources
    if (trace.empty() || trust_score < 0.3f) return;  // Minimum trust required
    
    std::string trace_hash = std::to_string(hash_sequence(trace));
    social_trust_[trace_hash] = trust_score;
    
    // Add to mid-term memory with trust-weighted consolidation
    multi_timescale_memory_.mid_term_episodes.push_back(trace);
    
    // Strengthen connections proportional to trust
    for (size_t i = 0; i < trace.size() - 1; i++) {
        int node_a = trace[i];
        int node_b = trace[i + 1];
        
        // Add/strengthen edge
        auto& edges_from_a = graph.edges[node_a];
        bool found = false;
        for (auto& [target, weight] : edges_from_a) {
            if (target == node_b) {
                weight += 0.1f * trust_score;
                weight = std::min(1.0f, weight);
                found = true;
                break;
            }
        }
        
        if (!found) {
            edges_from_a.emplace_back(node_b, 0.1f * trust_score);
        }
    }
    
    std::cout << "🤝 Imported social trace (trust: " << trust_score << ")" << std::endl;
}

void CognitiveEngine::learn_action_consequence(int action_node, const std::vector<int>& outcomes) {
    // Embodiment: learn action → consequence mappings
    // Update world model's causal edges
    engine.update_world_model(outcomes, 0.2f);
    
    // Also strengthen direct edges from action to outcomes
    auto& action_edges = graph.edges[action_node];
    for (int outcome_node : outcomes) {
        bool found = false;
        for (auto& [target, weight] : action_edges) {
            if (target == outcome_node) {
                weight += 0.15f;  // Stronger learning for embodied actions
                weight = std::min(1.0f, weight);
                found = true;
                break;
            }
        }
        
        if (!found) {
            action_edges.emplace_back(outcome_node, 0.15f);
        }
    }
}

} // namespace cognitive
} // namespace melvin

