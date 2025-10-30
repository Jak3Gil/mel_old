#include "unified_reasoning_engine.h"
#include <iostream>
#include <algorithm>
#include <cmath>
#include <random>

namespace melvin {
namespace reasoning {

UnifiedReasoningEngine::UnifiedReasoningEngine(int embedding_dim)
    : embedding_dim_(embedding_dim)
    , activation_field_()
    , predictor_(embedding_dim)
    , reasoning_session_count_(0)
{
    activation_field_.start_background_loop();
    std::cout << "🧠 Unified Reasoning Engine initialized" << std::endl;
    std::cout << "   Embedding dim: " << embedding_dim << std::endl;
    std::cout << "   Background spreading: ACTIVE" << std::endl;
}

UnifiedReasoningEngine::~UnifiedReasoningEngine() {
    shutdown();
}

void UnifiedReasoningEngine::process_input(
    const std::vector<int>& input_nodes,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    const std::string& modality
) {
    for (int node_id : input_nodes) {
        activation_field_.activate(node_id, 1.0f);
    }
    
    last_activity_ = std::chrono::high_resolution_clock::now();
}

std::vector<PredictionResult> UnifiedReasoningEngine::predict_next(
    const std::vector<int>& context_nodes,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    int top_k,
    Predictor::Mode mode
) {
    last_activity_ = std::chrono::high_resolution_clock::now();
    
    // Tick activation
    activation_field_.tick(graph);
    
    return predictor_.predict_next(context_nodes, activation_field_, graph, embeddings, top_k, mode);
}

std::vector<int> UnifiedReasoningEngine::generate_output(
    const std::vector<int>& prompt_nodes,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    float temperature,
    int max_length
) {
    last_activity_ = std::chrono::high_resolution_clock::now();
    int cycle_start_tick = global_tick_;
    
    // UPDATE CONTEXT with prompt
    update_context_vector(prompt_nodes, embeddings);
    
    // Activate prompt
    for (int node_id : prompt_nodes) {
        activation_field_.activate(node_id, 1.0f);
        add_temporal_event(node_id, global_tick_);
    }
    
    // SELECT THINKING MODE based on task characteristics
    float task_novelty = metrics_.novelty;
    float current_confidence = metrics_.confidence;
    select_thinking_mode(task_novelty, current_confidence);
    
    std::vector<int> output = prompt_nodes;
    int current = prompt_nodes.empty() ? -1 : prompt_nodes.back();
    
    std::random_device rd;
    std::mt19937 gen(rd());
    
    // START INTERNAL DIALOGUE for critical reasoning
    if (meta_reasoning_.current_mode == ThinkingMode::SLOW_ANALYTICAL) {
        start_internal_dialogue(prompt_nodes);
    }
    
    for (int step = 0; step < max_length && current != -1; ++step) {
        // CHECK GOAL: Should we stop inference?
        if (should_stop_inference()) {
            break;
        }
        
        // DECAY WORKING MEMORY each step
        decay_working_memory();
        
        // Get neighbors
        auto it = graph.find(current);
        if (it == graph.end() || it->second.empty()) {
            break;
        }
        
        // Score candidates using MULTIPLE MECHANISMS
        std::vector<std::pair<int, float>> candidates;
        for (const auto& edge : it->second) {
            int neighbor_id = edge.first;
            float edge_weight = edge.second;
            
            // Skip if already in output
            if (std::find(output.begin(), output.end(), neighbor_id) != output.end()) {
                continue;
            }
            
            // Base score: activation + edge weight
            float activation = activation_field_.get_activation(neighbor_id);
            float base_score = edge_weight * (1.0f + activation);
            
            // MECHANISM 1: GOAL RELEVANCE
            float goal_relevance = compute_relevance_to_goal(neighbor_id, embeddings);
            
            // MECHANISM 2: CAUSAL REASONING
            float causal_boost = 0.0f;
            for (const auto& causal_edge : causal_graph_) {
                if (causal_edge.source == current && causal_edge.target == neighbor_id) {
                    if (causal_edge.type == CausalType::CAUSES || causal_edge.type == CausalType::ENABLES) {
                        causal_boost += causal_edge.strength * causal_edge.confidence;
                    }
                }
            }
            
            // MECHANISM 3: CONTEXT RELEVANCE
            float ctx_relevance = context_relevance(neighbor_id, embeddings);
            
            // MECHANISM 4: UNCERTAINTY - prefer nodes we're uncertain about for exploration
            float uncertainty_factor = 1.0f;
            auto variance_it = belief_state_.variance.find(neighbor_id);
            if (variance_it != belief_state_.variance.end() && meta_reasoning_.current_mode == ThinkingMode::SLOW_ANALYTICAL) {
                uncertainty_factor = 1.0f + variance_it->second * 0.5f; // Bonus for uncertain nodes
            }
            
            // MECHANISM 5: SCHEMA MATCHING - bonus if part of learned pattern
            float schema_boost = 0.0f;
            for (const auto& schema : learned_schemas_) {
                if (std::find(schema.pattern_nodes.begin(), schema.pattern_nodes.end(), neighbor_id) 
                    != schema.pattern_nodes.end()) {
                    schema_boost += schema.reliability * 0.3f;
                }
            }
            
            // COMBINE ALL FACTORS based on thinking mode
            float final_score = base_score;
            
            if (meta_reasoning_.current_mode == ThinkingMode::FAST_ASSOCIATIVE) {
                // Fast mode: prioritize activation + schemas (intuition)
                final_score = base_score * 0.7f + schema_boost * 2.0f + ctx_relevance * 0.3f;
            } else if (meta_reasoning_.current_mode == ThinkingMode::SLOW_ANALYTICAL) {
                // Slow mode: prioritize goal relevance + causal reasoning
                final_score = base_score * 0.3f + goal_relevance * 2.0f + causal_boost * 1.5f + 
                              ctx_relevance * 0.5f + uncertainty_factor * 0.3f;
            } else {
                // Hybrid: balanced
                final_score = base_score * 0.5f + goal_relevance * 0.8f + causal_boost * 0.7f + 
                              ctx_relevance * 0.5f + schema_boost * 0.4f;
            }
            
            candidates.emplace_back(neighbor_id, final_score);
        }
        
        if (candidates.empty()) {
            break;
        }
        
        // Apply temperature and sample
        std::vector<float> scores;
        for (const auto& cand : candidates) {
            scores.push_back(cand.second / temperature);
        }
        
        // Softmax
        float max_score = *std::max_element(scores.begin(), scores.end());
        float sum = 0.0f;
        for (float& score : scores) {
            score = std::exp(score - max_score);
            sum += score;
        }
        for (float& score : scores) {
            score /= sum;
        }
        
        // Sample
        std::discrete_distribution<> dist(scores.begin(), scores.end());
        int idx = dist(gen);
        
        current = candidates[idx].first;
        output.push_back(current);
        
        // UPDATE WORKING MEMORY with intermediate result
        update_working_memory("step_" + std::to_string(step), current, scores[idx], 0.8f);
        
        // UPDATE BELIEF STATE
        update_belief_bayesian(current, scores[idx], true);
        
        // ADD TEMPORAL EVENT
        add_temporal_event(current, global_tick_ + step);
        
        // LEARN CAUSAL EDGE (current caused selection of next)
        if (output.size() > 1 && scores[idx] > 0.5f) {
            add_causal_edge(output[output.size()-2], current, CausalType::CAUSES, scores[idx]);
        }
        
        // Stop if activation too low and not in analytical mode
        if (activation_field_.get_activation(current) < 0.05f && 
            meta_reasoning_.current_mode != ThinkingMode::SLOW_ANALYTICAL) {
            break;
        }
    }
    
    // RESOLVE INTERNAL DIALOGUE if started
    if (meta_reasoning_.current_mode == ThinkingMode::SLOW_ANALYTICAL) {
        dialogue_.proponent.hypothesis = output;
        dialogue_.proponent.confidence = metrics_.confidence;
        int winner = resolve_dialogue();
        // Could use winner to adjust output confidence
    }
    
    // LEARN SCHEMA from this reasoning pattern
    if (output.size() >= 3) {
        learn_schema_from_pattern(output);
    }
    
    // DETECT CONTRADICTIONS periodically
    if (global_tick_ % 100 == 0) {
        auto contradictions = detect_contradictions();
        for (const auto& c : contradictions) {
            resolve_contradiction(c);
        }
    }
    
    // RECORD NARRATIVE EPISODE
    std::string summary = "Generated sequence of " + std::to_string(output.size()) + " nodes";
    record_narrative_episode(output, summary);
    
    // EVALUATE THIS REASONING CYCLE
    auto eval = evaluate_reasoning_cycle(cycle_start_tick);
    
    // RECORD EPISODE for reflective learning
    ReasoningEpisode episode;
    episode.goal_stack = {goal_stack_.active_goal_index};
    episode.strategy_used = meta_reasoning_.current_mode;
    episode.outcome_quality = eval.coherence;
    episode.confidence_before = current_confidence;
    episode.confidence_after = metrics_.confidence;
    episode.time_cost = static_cast<float>(eval.reasoning_cycles);
    episode.tick_count = eval.reasoning_cycles;
    record_reasoning_episode(episode);
    
    return output;
}

std::vector<int> UnifiedReasoningEngine::multi_hop_query(
    const std::vector<float>& query_embedding,
    const std::unordered_map<int, std::vector<std::pair<int, float>>>& graph,
    const std::unordered_map<int, std::vector<float>>& embeddings,
    int max_hops
) {
    last_activity_ = std::chrono::high_resolution_clock::now();
    
    std::vector<int> path;
    // Simplified multi-hop: follow highest activations
    
    auto active_nodes = activation_field_.get_active_nodes(0.2f);
    
    for (int hop = 0; hop < max_hops && !active_nodes.empty(); ++hop) {
        // Find highest activation node not yet in path
        int best_node = -1;
        float best_activation = 0.0f;
        
        for (const auto& pair : active_nodes) {
            if (std::find(path.begin(), path.end(), pair.first) == path.end()) {
                if (pair.second > best_activation) {
                    best_activation = pair.second;
                    best_node = pair.first;
                }
            }
        }
        
        if (best_node == -1) {
            break;
        }
        
        path.push_back(best_node);
        
        // Activate neighbors
        auto it = graph.find(best_node);
        if (it != graph.end()) {
            for (const auto& edge : it->second) {
                activation_field_.activate(edge.first, edge.second * 0.3f);
            }
        }
        
        // Refresh active nodes
        active_nodes = activation_field_.get_active_nodes(0.2f);
    }
    
    return path;
}

void UnifiedReasoningEngine::shutdown() {
    activation_field_.stop_background_loop();
    std::cout << "✅ Reasoning engine stopped" << std::endl;
}

// ADAPTIVE INTELLIGENCE: Update reasoning metrics after each reasoning cycle
void UnifiedReasoningEngine::update_metrics(float confidence, float coherence, float novelty) {
    // Store previous values
    metrics_.last_confidence = metrics_.confidence;
    metrics_.last_coherence = metrics_.coherence;
    metrics_.last_novelty = metrics_.novelty;
    
    // Update with exponential moving average (smoothing)
    float alpha = 0.3f; // Learning rate for metrics
    metrics_.confidence = alpha * confidence + (1.0f - alpha) * metrics_.confidence;
    metrics_.coherence = alpha * coherence + (1.0f - alpha) * metrics_.coherence;
    metrics_.novelty = alpha * novelty + (1.0f - alpha) * metrics_.novelty;
    
    // Compute entropy from activation field
    auto active_nodes = activation_field_.get_active_nodes(0.01f);
    float entropy = 0.0f;
    float total_activation = 0.0f;
    
    for (const auto& pair : active_nodes) {
        total_activation += pair.second;
    }
    
    if (total_activation > 1e-6f) {
        for (const auto& pair : active_nodes) {
            float p = pair.second / total_activation;
            if (p > 1e-6f) {
                entropy -= p * std::log(p);
            }
        }
    }
    
    metrics_.entropy = entropy;
    
    // Update success rate based on confidence gain
    float confidence_gain = metrics_.confidence - metrics_.last_confidence;
    bool success = (confidence_gain > 0.0f && metrics_.confidence > 0.5f);
    metrics_.success_rate = 0.9f * metrics_.success_rate + 0.1f * (success ? 1.0f : 0.0f);
}

// ==============================================================================
// HUMAN-LEVEL ADAPTIVE INTELLIGENCE v4.0: World Model
// ==============================================================================

void UnifiedReasoningEngine::predict_future_state(const std::vector<int>& current_context) {
    // Clear previous predictions
    world_model_.predicted_next_nodes.clear();
    world_model_.prediction_confidence.clear();
    
    if (current_context.empty()) return;
    
    // Get currently active nodes
    auto active_nodes = activation_field_.get_active_nodes(0.1f);
    
    // Build prediction from:
    // 1. Causal model (learned action→outcome)
    // 2. Recent activation patterns
    // 3. Temporal expectations
    
    std::unordered_map<int, float> prediction_scores;
    
    // Use causal edges if available
    for (int context_node : current_context) {
        auto causal_it = world_model_.causal_edges.find(context_node);
        if (causal_it != world_model_.causal_edges.end()) {
            for (const auto& [outcome_node, weight] : causal_it->second) {
                prediction_scores[outcome_node] += weight * 0.5f;
            }
        }
    }
    
    // Use current activation (what's "lighting up")
    for (const auto& [node_id, activation] : active_nodes) {
        prediction_scores[node_id] += activation * 0.3f;
    }
    
    // Sort by score and take top K
    std::vector<std::pair<int, float>> scored_predictions;
    for (const auto& [node_id, score] : prediction_scores) {
        scored_predictions.emplace_back(node_id, score);
    }
    
    std::sort(scored_predictions.begin(), scored_predictions.end(),
              [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Store top 10 predictions
    int max_predictions = std::min(10, static_cast<int>(scored_predictions.size()));
    for (int i = 0; i < max_predictions; i++) {
        world_model_.predicted_next_nodes.push_back(scored_predictions[i].first);
        world_model_.prediction_confidence.push_back(scored_predictions[i].second);
    }
}

void UnifiedReasoningEngine::update_world_model(const std::vector<int>& actual_next, float learning_rate) {
    if (world_model_.predicted_next_nodes.empty() || actual_next.empty()) {
        return;
    }
    
    // Compute prediction error
    float total_error = 0.0f;
    int matches = 0;
    
    for (int predicted_node : world_model_.predicted_next_nodes) {
        bool found = false;
        for (int actual_node : actual_next) {
            if (predicted_node == actual_node) {
                found = true;
                matches++;
                break;
            }
        }
        
        if (!found) {
            total_error += 1.0f;
        }
    }
    
    float accuracy = (world_model_.predicted_next_nodes.size() > 0) ?
                     static_cast<float>(matches) / world_model_.predicted_next_nodes.size() : 0.0f;
    
    // Update tracking
    world_model_.recent_prediction_error = total_error / std::max(1.0f, static_cast<float>(world_model_.predicted_next_nodes.size()));
    world_model_.avg_prediction_accuracy = 0.95f * world_model_.avg_prediction_accuracy + 0.05f * accuracy;
    
    // Learn causal connections: what we thought → what actually happened
    // Strengthen edges that were correct, weaken those that weren't
    for (size_t i = 0; i < world_model_.predicted_next_nodes.size(); i++) {
        int predicted_node = world_model_.predicted_next_nodes[i];
        float confidence = world_model_.prediction_confidence[i];
        
        // Was this prediction correct?
        bool correct = std::find(actual_next.begin(), actual_next.end(), predicted_node) != actual_next.end();
        
        // Update causal edges from last context to this outcome
        // This builds a model of "if I see X, I expect Y"
        for (int actual_node : actual_next) {
            auto& edges = world_model_.causal_edges[predicted_node];
            
            bool found = false;
            for (auto& [target, weight] : edges) {
                if (target == actual_node) {
                    // Strengthen if correct, weaken if wrong
                    weight += learning_rate * (correct ? confidence : -confidence * 0.5f);
                    weight = std::max(0.0f, std::min(1.0f, weight));
                    found = true;
                    break;
                }
            }
            
            if (!found && correct) {
                // Add new causal edge
                edges.emplace_back(actual_node, learning_rate * confidence);
            }
        }
    }
}

// ==============================================================================
// HUMAN-LIKE REASONING MECHANISMS: 13 Core Systems
// ==============================================================================

// -----------------------------------------------------------------------------
// 1. GOAL-DRIVEN REASONING
// -----------------------------------------------------------------------------

void UnifiedReasoningEngine::push_goal(const Goal& goal) {
    goal_stack_.goals.push_back(goal);
    goal_stack_.active_goal_index = goal_stack_.goals.size() - 1;
}

void UnifiedReasoningEngine::pop_goal() {
    if (!goal_stack_.goals.empty()) {
        goal_stack_.goals.pop_back();
        goal_stack_.active_goal_index = goal_stack_.goals.empty() ? -1 : goal_stack_.goals.size() - 1;
    }
}

float UnifiedReasoningEngine::compute_relevance_to_goal(
    int node_id, 
    const std::unordered_map<int, std::vector<float>>& embeddings
) {
    if (goal_stack_.active_goal_index < 0 || goal_stack_.active_goal_index >= static_cast<int>(goal_stack_.goals.size())) {
        return 0.5f; // No goal, default relevance
    }
    
    const Goal& active_goal = goal_stack_.goals[goal_stack_.active_goal_index];
    
    // Cosine similarity between node and goal embedding
    auto node_emb_it = embeddings.find(node_id);
    if (node_emb_it == embeddings.end() || active_goal.target_embedding.empty()) {
        return 0.5f;
    }
    
    const auto& node_emb = node_emb_it->second;
    const auto& goal_emb = active_goal.target_embedding;
    
    float dot = 0.0f, norm_node = 0.0f, norm_goal = 0.0f;
    size_t min_dim = std::min(node_emb.size(), goal_emb.size());
    
    for (size_t i = 0; i < min_dim; i++) {
        dot += node_emb[i] * goal_emb[i];
        norm_node += node_emb[i] * node_emb[i];
        norm_goal += goal_emb[i] * goal_emb[i];
    }
    
    norm_node = std::sqrt(norm_node);
    norm_goal = std::sqrt(norm_goal);
    
    if (norm_node < 1e-6f || norm_goal < 1e-6f) {
        return 0.5f;
    }
    
    float similarity = dot / (norm_node * norm_goal);
    return (similarity + 1.0f) / 2.0f; // Map [-1,1] to [0,1]
}

bool UnifiedReasoningEngine::should_stop_inference() {
    if (goal_stack_.active_goal_index < 0) {
        return false; // No goal, continue inference
    }
    
    const Goal& active_goal = goal_stack_.goals[goal_stack_.active_goal_index];
    
    // Check if goal node is highly activated
    float goal_activation = activation_field_.get_activation(active_goal.target_node_id);
    
    if (goal_activation > active_goal.relevance_threshold) {
        return true; // Goal reached
    }
    
    // Check if marginal relevance is too low
    auto active_nodes = activation_field_.get_active_nodes(0.1f);
    float avg_relevance = 0.0f;
    int count = 0;
    
    for (const auto& [node_id, activation] : active_nodes) {
        // Would need embeddings here - simplified version
        avg_relevance += activation;
        count++;
    }
    
    if (count > 0) {
        avg_relevance /= count;
    }
    
    // Stop if average activation is very low (search exhausted)
    return avg_relevance < 0.05f;
}

// -----------------------------------------------------------------------------
// 2. CAUSAL REASONING
// -----------------------------------------------------------------------------

void UnifiedReasoningEngine::add_causal_edge(int source, int target, CausalType type, float strength) {
    CausalEdge edge;
    edge.source = source;
    edge.target = target;
    edge.type = type;
    edge.strength = strength;
    edge.confidence = 0.5f;
    
    causal_graph_.push_back(edge);
}

std::vector<int> UnifiedReasoningEngine::simulate_causal_intervention(int intervention_node, bool enable) {
    std::vector<int> affected_nodes;
    
    // Find all nodes causally dependent on intervention_node
    std::unordered_set<int> visited;
    std::vector<int> to_visit = {intervention_node};
    
    while (!to_visit.empty()) {
        int current = to_visit.back();
        to_visit.pop_back();
        
        if (visited.count(current)) continue;
        visited.insert(current);
        
        // Find all causal edges from current
        for (const auto& edge : causal_graph_) {
            if (edge.source == current) {
                float effect = enable ? edge.strength : -edge.strength;
                
                if (edge.type == CausalType::CAUSES || edge.type == CausalType::ENABLES) {
                    affected_nodes.push_back(edge.target);
                    to_visit.push_back(edge.target);
                } else if (edge.type == CausalType::INHIBITS) {
                    // Inhibition reverses the effect
                    effect = -effect;
                    affected_nodes.push_back(edge.target);
                    to_visit.push_back(edge.target);
                }
            }
        }
    }
    
    return affected_nodes;
}

// -----------------------------------------------------------------------------
// 3. WORKING MEMORY SCRATCHPAD
// -----------------------------------------------------------------------------

void UnifiedReasoningEngine::update_working_memory(
    const std::string& var, 
    int node_id, 
    float value, 
    float confidence
) {
    working_memory_.current_tick++;
    
    // Check if variable already exists
    for (auto& slot : working_memory_.slots) {
        if (slot.variable_name == var) {
            slot.node_id = node_id;
            slot.value = value;
            slot.confidence = confidence;
            slot.last_access_tick = working_memory_.current_tick;
            return;
        }
    }
    
    // Add new slot if space available
    if (static_cast<int>(working_memory_.slots.size()) < working_memory_.max_slots) {
        WorkingMemorySlot slot;
        slot.variable_name = var;
        slot.node_id = node_id;
        slot.value = value;
        slot.confidence = confidence;
        slot.creation_tick = working_memory_.current_tick;
        slot.last_access_tick = working_memory_.current_tick;
        
        working_memory_.slots.push_back(slot);
    } else {
        // Evict least recently used
        auto lru_it = std::min_element(
            working_memory_.slots.begin(),
            working_memory_.slots.end(),
            [](const auto& a, const auto& b) {
                return a.last_access_tick < b.last_access_tick;
            }
        );
        
        if (lru_it != working_memory_.slots.end()) {
            lru_it->variable_name = var;
            lru_it->node_id = node_id;
            lru_it->value = value;
            lru_it->confidence = confidence;
            lru_it->creation_tick = working_memory_.current_tick;
            lru_it->last_access_tick = working_memory_.current_tick;
        }
    }
}

void UnifiedReasoningEngine::decay_working_memory() {
    working_memory_.current_tick++;
    
    // Decay confidence based on time since last access
    for (auto& slot : working_memory_.slots) {
        int ticks_since_access = working_memory_.current_tick - slot.last_access_tick;
        float decay_factor = std::pow(working_memory_.decay_rate, ticks_since_access);
        slot.confidence *= decay_factor;
    }
    
    // Remove slots with very low confidence
    working_memory_.slots.erase(
        std::remove_if(
            working_memory_.slots.begin(),
            working_memory_.slots.end(),
            [](const auto& slot) { return slot.confidence < 0.1f; }
        ),
        working_memory_.slots.end()
    );
}

// -----------------------------------------------------------------------------
// 4. CONTEXT VECTOR
// -----------------------------------------------------------------------------

void UnifiedReasoningEngine::update_context_vector(
    const std::vector<int>& perception,
    const std::unordered_map<int, std::vector<float>>& embeddings
) {
    // Initialize context vector if needed
    if (context_state_.context_vector.empty() && !embeddings.empty()) {
        context_state_.context_vector.resize(embeddings.begin()->second.size(), 0.0f);
    }
    
    // Update recent perception
    context_state_.recent_perception.insert(
        context_state_.recent_perception.end(),
        perception.begin(),
        perception.end()
    );
    
    // Keep only recent items
    if (static_cast<int>(context_state_.recent_perception.size()) > context_state_.window_size) {
        context_state_.recent_perception.erase(
            context_state_.recent_perception.begin(),
            context_state_.recent_perception.begin() + 
            (context_state_.recent_perception.size() - context_state_.window_size)
        );
    }
    
    // Compute context vector as weighted average of recent perceptions + goals
    std::fill(context_state_.context_vector.begin(), context_state_.context_vector.end(), 0.0f);
    
    int count = 0;
    for (int node_id : context_state_.recent_perception) {
        auto it = embeddings.find(node_id);
        if (it != embeddings.end()) {
            for (size_t i = 0; i < std::min(context_state_.context_vector.size(), it->second.size()); i++) {
                context_state_.context_vector[i] += it->second[i];
            }
            count++;
        }
    }
    
    // Add active goals with higher weight
    for (const auto& goal : goal_stack_.goals) {
        for (size_t i = 0; i < std::min(context_state_.context_vector.size(), goal.target_embedding.size()); i++) {
            context_state_.context_vector[i] += goal.target_embedding[i] * 2.0f * goal.priority;
        }
        count += 2; // Goals count double
    }
    
    // Normalize
    if (count > 0) {
        for (float& val : context_state_.context_vector) {
            val /= count;
        }
    }
    
    // Store in history
    context_state_.context_history.push_back(context_state_.context_vector);
    if (static_cast<int>(context_state_.context_history.size()) > context_state_.window_size) {
        context_state_.context_history.pop_front();
    }
}

float UnifiedReasoningEngine::context_relevance(
    int node_id,
    const std::unordered_map<int, std::vector<float>>& embeddings
) {
    if (context_state_.context_vector.empty()) {
        return 0.5f;
    }
    
    auto it = embeddings.find(node_id);
    if (it == embeddings.end()) {
        return 0.5f;
    }
    
    const auto& node_emb = it->second;
    float dot = 0.0f, norm_node = 0.0f, norm_context = 0.0f;
    size_t min_dim = std::min(node_emb.size(), context_state_.context_vector.size());
    
    for (size_t i = 0; i < min_dim; i++) {
        dot += node_emb[i] * context_state_.context_vector[i];
        norm_node += node_emb[i] * node_emb[i];
        norm_context += context_state_.context_vector[i] * context_state_.context_vector[i];
    }
    
    norm_node = std::sqrt(norm_node);
    norm_context = std::sqrt(norm_context);
    
    if (norm_node < 1e-6f || norm_context < 1e-6f) {
        return 0.5f;
    }
    
    float similarity = dot / (norm_node * norm_context);
    return (similarity + 1.0f) / 2.0f; // Map to [0,1]
}

// -----------------------------------------------------------------------------
// 5. UNCERTAINTY TRACKING
// -----------------------------------------------------------------------------

void UnifiedReasoningEngine::update_belief_bayesian(int node_id, float evidence_strength, bool positive) {
    // Get current belief
    float prior_mean = belief_state_.mean_activation[node_id];
    float prior_variance = belief_state_.variance[node_id];
    
    if (prior_variance == 0.0f) {
        prior_variance = 0.25f; // Initial uncertainty
    }
    
    // Bayesian update
    // New evidence: E with strength s, direction d
    float likelihood_mean = positive ? evidence_strength : (1.0f - evidence_strength);
    float likelihood_variance = 0.1f; // Evidence uncertainty
    
    // Update using Bayesian inference
    float posterior_variance = 1.0f / (1.0f / prior_variance + 1.0f / likelihood_variance);
    float posterior_mean = posterior_variance * (prior_mean / prior_variance + likelihood_mean / likelihood_variance);
    
    belief_state_.mean_activation[node_id] = posterior_mean;
    belief_state_.variance[node_id] = posterior_variance;
    belief_state_.confidence[node_id] = 1.0f / (1.0f + posterior_variance); // Higher when variance low
}

// -----------------------------------------------------------------------------
// 6. INTERNAL DIALOGUE
// -----------------------------------------------------------------------------

void UnifiedReasoningEngine::start_internal_dialogue(const std::vector<int>& hypothesis) {
    dialogue_.proponent.hypothesis = hypothesis;
    dialogue_.proponent.confidence = 0.5f;
    dialogue_.proponent.evidence_strength = 0.5f;
    dialogue_.proponent.justifications.clear();
    
    dialogue_.skeptic.hypothesis.clear();
    dialogue_.skeptic.confidence = 0.5f;
    dialogue_.skeptic.evidence_strength = 0.5f;
    dialogue_.skeptic.justifications.clear();
    
    dialogue_.debate_history.clear();
    dialogue_.winner_stream = -1;
}

int UnifiedReasoningEngine::resolve_dialogue() {
    // Compare evidence strength and confidence
    float proponent_score = dialogue_.proponent.confidence * dialogue_.proponent.evidence_strength;
    float skeptic_score = dialogue_.skeptic.confidence * dialogue_.skeptic.evidence_strength;
    
    // Winner is the stream with higher combined score
    if (proponent_score > skeptic_score) {
        dialogue_.winner_stream = 0;
        dialogue_.debate_history.push_back("Proponent wins: stronger evidence");
        return 0;
    } else {
        dialogue_.winner_stream = 1;
        dialogue_.debate_history.push_back("Skeptic wins: insufficient evidence");
        return 1;
    }
}

// -----------------------------------------------------------------------------
// 7. COHERENCE CHECKING
// -----------------------------------------------------------------------------

std::vector<UnifiedReasoningEngine::Contradiction> UnifiedReasoningEngine::detect_contradictions() {
    std::vector<Contradiction> contradictions;
    
    // Check for conflicting causal edges
    for (size_t i = 0; i < causal_graph_.size(); i++) {
        for (size_t j = i + 1; j < causal_graph_.size(); j++) {
            const auto& edge1 = causal_graph_[i];
            const auto& edge2 = causal_graph_[j];
            
            // Same source, same target, opposite types?
            if (edge1.source == edge2.source && edge1.target == edge2.target) {
                bool conflict = false;
                
                if ((edge1.type == CausalType::CAUSES && edge2.type == CausalType::INHIBITS) ||
                    (edge1.type == CausalType::INHIBITS && edge2.type == CausalType::CAUSES)) {
                    conflict = true;
                }
                
                if (conflict) {
                    Contradiction c;
                    c.edge1_source = edge1.source;
                    c.edge1_target = edge1.target;
                    c.edge2_source = edge2.source;
                    c.edge2_target = edge2.target;
                    c.conflict_strength = std::abs(edge1.strength - edge2.strength);
                    c.description = "Causal conflict: same nodes, opposite effects";
                    
                    contradictions.push_back(c);
                }
            }
        }
    }
    
    detected_contradictions_ = contradictions;
    return contradictions;
}

void UnifiedReasoningEngine::resolve_contradiction(const Contradiction& c) {
    // Find the conflicting edges and keep the stronger one
    for (auto it = causal_graph_.begin(); it != causal_graph_.end(); ) {
        if ((it->source == c.edge1_source && it->target == c.edge1_target) ||
            (it->source == c.edge2_source && it->target == c.edge2_target)) {
            
            // Keep edge with higher confidence, remove the other
            float max_confidence = 0.0f;
            for (const auto& edge : causal_graph_) {
                if (edge.source == it->source && edge.target == it->target) {
                    max_confidence = std::max(max_confidence, edge.confidence);
                }
            }
            
            if (it->confidence < max_confidence * 0.8f) {
                it = causal_graph_.erase(it);
            } else {
                ++it;
            }
        } else {
            ++it;
        }
    }
}

// -----------------------------------------------------------------------------
// 8. META-REASONING (System 1 vs System 2)
// -----------------------------------------------------------------------------

void UnifiedReasoningEngine::select_thinking_mode(float task_novelty, float current_confidence) {
    meta_reasoning_.task_novelty = task_novelty;
    meta_reasoning_.cycles_in_mode++;
    
    // High novelty or low confidence → switch to SLOW_ANALYTICAL
    // Low novelty and high confidence → switch to FAST_ASSOCIATIVE
    
    if (task_novelty > 0.7f || current_confidence < 0.3f) {
        if (meta_reasoning_.current_mode != ThinkingMode::SLOW_ANALYTICAL) {
            meta_reasoning_.current_mode = ThinkingMode::SLOW_ANALYTICAL;
            meta_reasoning_.cycles_in_mode = 0;
            meta_reasoning_.confidence_in_mode = 0.6f;
        }
    } else if (task_novelty < 0.3f && current_confidence > 0.7f) {
        if (meta_reasoning_.current_mode != ThinkingMode::FAST_ASSOCIATIVE) {
            meta_reasoning_.current_mode = ThinkingMode::FAST_ASSOCIATIVE;
            meta_reasoning_.cycles_in_mode = 0;
            meta_reasoning_.confidence_in_mode = 0.8f;
        }
    } else {
        // Hybrid mode
        if (meta_reasoning_.current_mode != ThinkingMode::HYBRID) {
            meta_reasoning_.current_mode = ThinkingMode::HYBRID;
            meta_reasoning_.cycles_in_mode = 0;
            meta_reasoning_.confidence_in_mode = 0.7f;
        }
    }
}

// -----------------------------------------------------------------------------
// 9. SCHEMA LEARNING
// -----------------------------------------------------------------------------

void UnifiedReasoningEngine::learn_schema_from_pattern(const std::vector<int>& pattern) {
    if (pattern.size() < 2) return;
    
    // Check if this pattern matches an existing schema
    for (auto& schema : learned_schemas_) {
        int matches = 0;
        for (int node_id : pattern) {
            if (std::find(schema.pattern_nodes.begin(), schema.pattern_nodes.end(), node_id) 
                != schema.pattern_nodes.end()) {
                matches++;
            }
        }
        
        float overlap = static_cast<float>(matches) / pattern.size();
        if (overlap > 0.6f) {
            // Strengthen existing schema
            schema.activation_count += 1.0f;
            schema.reliability = std::min(0.95f, schema.reliability + 0.05f);
            return;
        }
    }
    
    // Create new schema
    Schema new_schema;
    new_schema.schema_node_id = -1; // Would create new node in graph
    new_schema.pattern_nodes = pattern;
    new_schema.activation_count = 1.0f;
    new_schema.reliability = 0.5f;
    
    // Extract edges from pattern
    for (size_t i = 0; i < pattern.size() - 1; i++) {
        new_schema.pattern_edges.emplace_back(pattern[i], pattern[i+1]);
    }
    
    learned_schemas_.push_back(new_schema);
}

// -----------------------------------------------------------------------------
// 10. REFLECTIVE LEARNING
// -----------------------------------------------------------------------------

void UnifiedReasoningEngine::record_reasoning_episode(const ReasoningEpisode& episode) {
    reasoning_history_.push_back(episode);
    
    // Keep only recent history
    if (reasoning_history_.size() > 1000) {
        reasoning_history_.erase(reasoning_history_.begin());
    }
    
    // Analyze patterns in successful vs failed episodes
    float avg_success_time = 0.0f;
    float avg_failure_time = 0.0f;
    int success_count = 0, failure_count = 0;
    
    for (const auto& ep : reasoning_history_) {
        if (ep.outcome_quality > 0.6f) {
            avg_success_time += ep.time_cost;
            success_count++;
        } else {
            avg_failure_time += ep.time_cost;
            failure_count++;
        }
    }
    
    if (success_count > 0) avg_success_time /= success_count;
    if (failure_count > 0) avg_failure_time /= failure_count;
    
    // Learn: successful episodes take less time on average → optimize for that
}

// -----------------------------------------------------------------------------
// 11. TEMPORAL AWARENESS
// -----------------------------------------------------------------------------

void UnifiedReasoningEngine::add_temporal_event(int node_id, int timestamp) {
    TemporalEvent event;
    event.node_id = node_id;
    event.timestamp = timestamp;
    
    temporal_timeline_.push_back(event);
}

void UnifiedReasoningEngine::add_temporal_relation(int event1, int event2, TemporalRelation relation) {
    // Find events in timeline
    for (auto& event : temporal_timeline_) {
        if (event.node_id == event1) {
            event.relations_to_other_events[event2] = relation;
        }
    }
}

// -----------------------------------------------------------------------------
// 12. NARRATIVE MEMORY
// -----------------------------------------------------------------------------

void UnifiedReasoningEngine::record_narrative_episode(
    const std::vector<int>& sequence,
    const std::string& summary
) {
    NarrativeEpisode episode;
    episode.reasoning_sequence = sequence;
    episode.summary = summary;
    episode.start_tick = global_tick_;
    episode.end_tick = global_tick_ + static_cast<int>(sequence.size());
    episode.coherence_score = 0.7f; // Would compute based on sequence coherence
    
    narrative_memory_.push_back(episode);
    
    // Keep only recent narratives
    if (narrative_memory_.size() > 100) {
        narrative_memory_.erase(narrative_memory_.begin());
    }
}

// -----------------------------------------------------------------------------
// 13. REASONING EVALUATION
// -----------------------------------------------------------------------------

UnifiedReasoningEngine::ReasoningEvaluation 
UnifiedReasoningEngine::evaluate_reasoning_cycle(int cycle_start_tick) {
    ReasoningEvaluation eval;
    
    eval.reasoning_cycles = global_tick_ - cycle_start_tick;
    
    // Coherence: how semantically consistent were the activated nodes
    eval.coherence = metrics_.coherence;
    
    // Justification depth: count causal chain length
    int max_causal_depth = 0;
    for (const auto& edge : causal_graph_) {
        // Would do recursive depth search
        max_causal_depth = std::max(max_causal_depth, 1);
    }
    eval.justification_depth = static_cast<float>(max_causal_depth);
    
    // Efficiency: goal progress per compute cycle
    eval.goal_progress = 0.0f;
    if (goal_stack_.active_goal_index >= 0) {
        const auto& goal = goal_stack_.goals[goal_stack_.active_goal_index];
        eval.goal_progress = activation_field_.get_activation(goal.target_node_id);
    }
    
    eval.efficiency = eval.reasoning_cycles > 0 ? eval.goal_progress / eval.reasoning_cycles : 0.0f;
    
    // Introspective accuracy: compare predicted vs actual confidence
    eval.introspective_accuracy = metrics_.confidence;
    
    current_evaluation_ = eval;
    global_tick_++;
    
    return eval;
}

} // namespace reasoning
} // namespace melvin
