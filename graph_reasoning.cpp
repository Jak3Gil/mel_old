#include "graph_reasoning.hpp"
#include <fstream>
#include <sstream>
#include <chrono>
#include "storage.h"

namespace melvin {

// GraphReasoningSystem implementation
GraphReasoningSystem::GraphReasoningSystem(const std::string& store_dir, const GraphReasoningConfig& config)
    : store_(open_store(store_dir)),
      scorer_(config.scoring_weights),
      learner_(config.learning_params),
      beam_engine_(config.default_beam_params, config.scoring_weights),
      nlg_(store_.get(), config.nlg_config),
      miner_(store_.get(), config.mining_config),
      config_(config),
      initialized_(false),
      last_maintenance_time_(0) {
    
    initialize_system();
}

void GraphReasoningSystem::initialize_system() {
    if (!store_) {
        throw std::runtime_error("Failed to open store");
    }
    
    apply_config_to_components();
    validate_config();
    
    initialized_ = true;
}

std::string GraphReasoningSystem::reason(const std::string& query) {
    if (!initialized_) {
        return "System not initialized";
    }
    
    try {
        // Preprocess query
        Query processed_query = preprocess_query(query);
        
        // Classify query
        QueryClassification classification = policy_manager_.classify_query(processed_query);
        
        // Find start nodes
        std::vector<NodeID> start_nodes = find_start_nodes(processed_query);
        
        if (start_nodes.empty()) {
            return "GRAPH_REASONING_NO_START_NODES";
        }
        
        // Perform beam search
        std::vector<BeamPath> paths = perform_beam_search(processed_query, start_nodes);
        
        if (paths.empty()) {
            return "I couldn't find a clear path to answer that question.";
        }
        
        // Generate response
        std::string response = generate_response(paths, processed_query);
        
        // Update learning
        if (config_.enable_learning) {
            update_learning(paths, processed_query);
        }
        
        // Check if maintenance is needed
        if (should_run_maintenance()) {
            run_periodic_maintenance();
        }
        
        return response;
        
    } catch (const std::exception& e) {
        return "Error processing query: " + std::string(e.what());
    }
}

std::string GraphReasoningSystem::reason_with_start_nodes(const Query& query) {
    if (!initialized_) {
        return "Graph reasoning system not initialized.";
    }
    
    try {
        // Use the provided start nodes directly
        std::vector<NodeID> start_nodes = query.focus_nodes;
        
        if (start_nodes.empty()) {
            return ""; // Return empty string to indicate no graph reasoning contribution
        }
        
        // Perform beam search
        std::vector<BeamPath> paths = perform_beam_search(query, start_nodes);
        
        if (paths.empty()) {
            return ""; // Return empty string to indicate no graph reasoning contribution
        }
        
        // Generate response
        std::string response = generate_response(paths, query);
        
        // Update learning
        if (config_.enable_learning) {
            update_learning(paths, query);
        }
        
        // Check if maintenance is needed
        if (should_run_maintenance()) {
            run_periodic_maintenance();
        }
        
        return response;
        
    } catch (const std::exception& e) {
        return ""; // Return empty string on error to allow other systems to contribute
    }
}

void GraphReasoningSystem::learn(const std::string& text) {
    if (!initialized_ || !config_.enable_learning) return;
    
    try {
        // Preprocess text into sequence of nodes
        std::vector<NodeID> sequence = preprocess_query(text).focus_nodes;
        
        if (!sequence.empty()) {
            learn_from_sequence(sequence);
        }
        
    } catch (const std::exception& e) {
        // Log error but don't throw
    }
}

void GraphReasoningSystem::learn_from_sequence(const std::vector<NodeID>& sequence) {
    if (!initialized_ || !config_.enable_learning) return;
    
    try {
        learner_.learn_from_sequence(sequence, store_.get());
        
        if (config_.enable_mining) {
            miner_.mine_from_sequence(sequence);
        }
        
    } catch (const std::exception& e) {
        // Log error but don't throw
    }
}

void GraphReasoningSystem::run_maintenance_pass() {
    if (!initialized_) return;
    
    try {
        // Run learner maintenance
        learner_.run_maintenance_pass(store_.get());
        
        // Run miner maintenance
        if (config_.enable_mining) {
            miner_.run_mining_pass();
        }
        
        last_maintenance_time_ = std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();
        
    } catch (const std::exception& e) {
        // Log error but don't throw
    }
}

void GraphReasoningSystem::decay_pass() {
    if (!initialized_) return;
    
    try {
        learner_.get_edge_learner().decay_all_edges(store_.get());
    } catch (const std::exception& e) {
        // Log error but don't throw
    }
}

void GraphReasoningSystem::set_config(const GraphReasoningConfig& config) {
    config_ = config;
    apply_config_to_components();
    validate_config();
}

size_t GraphReasoningSystem::get_node_count() const {
    return store_ ? store_->node_count() : 0;
}

size_t GraphReasoningSystem::get_edge_count() const {
    return store_ ? store_->edge_count() : 0;
}

size_t GraphReasoningSystem::get_path_count() const {
    return store_ ? store_->path_count() : 0;
}

double GraphReasoningSystem::get_health_score() const {
    if (!store_) return 0.0;
    
    // Simple health score based on graph connectivity
    size_t nodes = get_node_count();
    size_t edges = get_edge_count();
    
    if (nodes == 0) return 0.0;
    
    double connectivity = static_cast<double>(edges) / (nodes * nodes);
    return std::min(1.0, connectivity * 100.0); // Scale to [0, 1]
}

QueryClassification GraphReasoningSystem::classify_query(const std::string& query_text) const {
    return policy_manager_.classify_query(query_text);
}

std::vector<BeamPath> GraphReasoningSystem::search_paths(const std::string& query_text) {
    Query processed_query = preprocess_query(query_text);
    std::vector<NodeID> start_nodes = find_start_nodes(processed_query);
    return perform_beam_search(processed_query, start_nodes);
}

Query GraphReasoningSystem::preprocess_query(const std::string& raw_query) {
    Query query;
    query.original_text = raw_query;
    query.timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // Simple tokenization
    std::istringstream iss(raw_query);
    std::string token;
    while (iss >> token) {
        // Remove punctuation and convert to lowercase
        std::string clean_token;
        for (char c : token) {
            if (std::isalnum(c)) {
                clean_token += std::tolower(c);
            }
        }
        if (!clean_token.empty()) {
            query.tokens.push_back(clean_token);
        }
    }
    
    return query;
}

std::vector<NodeID> GraphReasoningSystem::find_start_nodes(const Query& query) {
    if (query.focus_nodes.empty()) {
        // Try to find nodes matching query tokens
        std::vector<NodeID> start_nodes;
        
        if (!store_) {
            return start_nodes;
        }
        
        // Simple node lookup by token matching
        for (const std::string& token : query.tokens) {
            // Try to find a node that matches this token
            NodeID node_id;
            if (find_node_by_token(token, node_id)) {
                start_nodes.push_back(node_id);
            }
        }
        
        return start_nodes;
    }
    
    return query.focus_nodes;
}

bool GraphReasoningSystem::find_node_by_token(const std::string& token, NodeID& node_id) {
    if (!store_) return false;
    
    // Simple approach: iterate through all nodes and find one that matches the token
    // In practice, you'd want a more efficient lookup mechanism
    
    // For now, we'll use a simple hash-based approach
    // Create a deterministic NodeID from the token
    std::hash<std::string> hasher;
    size_t hash_value = hasher(token);
    
    for (int i = 0; i < 32; i++) {
        node_id[i] = static_cast<uint8_t>((hash_value >> (i % 8)) & 0xFF);
    }
    
    // Check if this node exists in the store
    NodeRecHeader node_header;
    std::vector<uint8_t> payload;
    return store_->get_node(node_id, node_header, payload);
}

std::vector<BeamPath> GraphReasoningSystem::perform_beam_search(const Query& query, const std::vector<NodeID>& start_nodes) {
    if (start_nodes.empty()) return {};
    
    std::vector<BeamPath> all_paths;
    
    for (const auto& start_node : start_nodes) {
        std::vector<BeamPath> paths = beam_engine_.search(query, start_node, store_.get());
        all_paths.insert(all_paths.end(), paths.begin(), paths.end());
    }
    
    // Sort by score and return best paths
    std::sort(all_paths.begin(), all_paths.end(),
              [](const BeamPath& a, const BeamPath& b) { return a.score > b.score; });
    
    return all_paths;
}

std::string GraphReasoningSystem::generate_response(const std::vector<BeamPath>& paths, const Query& query) {
    if (paths.empty()) {
        return "I don't have enough information to answer that question.";
    }
    
    // Limit response length
    std::vector<BeamPath> limited_paths = paths;
    if (limited_paths.size() > 5) {
        limited_paths.resize(5);
    }
    
    return nlg_.generate_response(limited_paths, query);
}

void GraphReasoningSystem::update_learning(const std::vector<BeamPath>& used_paths, const Query& query) {
    if (!config_.enable_learning) return;
    
    std::vector<NodeID> query_nodes = query.focus_nodes;
    std::vector<EdgeID> used_edges;
    
    // Extract edges from used paths
    for (const auto& path : used_paths) {
        for (const auto& edge : path.edges) {
            // Convert EdgeRec to EdgeID (simplified - just use first 32 bytes)
            EdgeID edge_id;
            std::copy(edge.src, edge.src + 32, edge_id.begin());
            used_edges.push_back(edge_id);
        }
    }
    
    learner_.learn_from_query(query_nodes, used_edges, store_.get(), true);
}

void GraphReasoningSystem::run_periodic_maintenance() {
    run_maintenance_pass();
}

bool GraphReasoningSystem::should_run_maintenance() const {
    uint64_t current_time = std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    
    // Run maintenance every hour
    uint64_t maintenance_interval = 3600000000000ULL; // 1 hour in nanoseconds
    
    return (current_time - last_maintenance_time_) > maintenance_interval;
}

void GraphReasoningSystem::apply_config_to_components() {
    scorer_.set_weights(config_.scoring_weights);
    learner_.set_params(config_.learning_params);
    beam_engine_.set_params(config_.default_beam_params);
    beam_engine_.set_scoring_weights(config_.scoring_weights);
    nlg_.set_config(config_.nlg_config);
    miner_.set_config(config_.mining_config);
}

void GraphReasoningSystem::validate_config() const {
    if (config_.confidence_threshold_low < 0.0 || config_.confidence_threshold_low > 1.0) {
        throw std::invalid_argument("confidence_threshold_low must be in [0, 1]");
    }
    
    if (config_.confidence_threshold_high < 0.0 || config_.confidence_threshold_high > 1.0) {
        throw std::invalid_argument("confidence_threshold_high must be in [0, 1]");
    }
    
    if (config_.confidence_threshold_low >= config_.confidence_threshold_high) {
        throw std::invalid_argument("confidence_threshold_low must be < confidence_threshold_high");
    }
    
    if (config_.max_response_length == 0) {
        throw std::invalid_argument("max_response_length must be > 0");
    }
}

// ConfigManager implementation
GraphReasoningConfig ConfigManager::load_config(const std::string& config_file) {
    std::ifstream file(config_file);
    if (!file.is_open()) {
        return get_default_config();
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json_str = buffer.str();
    
    return parse_config_json(json_str);
}

void ConfigManager::save_config(const GraphReasoningConfig& config, const std::string& config_file) {
    std::string json_str = config_to_json(config);
    
    std::ofstream file(config_file);
    if (file.is_open()) {
        file << json_str;
    }
}

GraphReasoningConfig ConfigManager::get_default_config() {
    GraphReasoningConfig config;
    
    // Default scoring weights
    config.scoring_weights.beta_text = 1.0;
    config.scoring_weights.beta_freq = 0.3;
    config.scoring_weights.beta_anchor = 2.0;
    config.scoring_weights.gamma_rel = 1.0;
    config.scoring_weights.gamma_sem = 1.5;
    config.scoring_weights.gamma_ctx = 0.8;
    config.scoring_weights.gamma_core = 1.2;
    config.scoring_weights.gamma_len = 0.1;
    config.scoring_weights.delta_start = 1.0;
    config.scoring_weights.delta_support = 0.5;
    config.scoring_weights.delta_redund = 0.3;
    
    // Default learning parameters
    config.learning_params.alpha_core = 1.0;
    config.learning_params.alpha_ctx = 0.5;
    config.learning_params.alpha_infer = 0.2;
    config.learning_params.lambda_core = 0.001;
    config.learning_params.lambda_ctx = 0.01;
    config.learning_params.epsilon_prune = 0.2;
    
    // Default beam parameters
    config.default_beam_params.beam_width = 32;
    config.default_beam_params.max_depth = 5;
    config.default_beam_params.top_k = 8;
    config.default_beam_params.top_p = 0.9;
    config.default_beam_params.stop_threshold = 0.05;
    
    // Default NLG configuration
    config.nlg_config.min_clause_confidence = 0.3;
    config.nlg_config.min_path_confidence = 0.4;
    config.nlg_config.high_confidence_threshold = 0.7;
    config.nlg_config.use_hedges = true;
    config.nlg_config.use_connectors = true;
    config.nlg_config.max_clauses = 3;
    
    // Default mining configuration
    config.mining_config.theta_node = 0.15;
    config.mining_config.theta_pmi = 1.0;
    config.mining_config.theta_pat = 8.0;
    config.mining_config.theta_div = 0.4;
    config.mining_config.max_pattern_length = 4;
    config.mining_config.min_pattern_length = 2;
    
    return config;
}

bool ConfigManager::validate_config(const GraphReasoningConfig& config) {
    try {
        // Validate scoring weights
        if (config.scoring_weights.beta_text < 0.0 || config.scoring_weights.beta_freq < 0.0 ||
            config.scoring_weights.beta_anchor < 0.0) {
            return false;
        }
        
        // Validate learning parameters
        if (config.learning_params.alpha_core < 0.0 || config.learning_params.alpha_ctx < 0.0 ||
            config.learning_params.lambda_core < 0.0 || config.learning_params.lambda_ctx < 0.0) {
            return false;
        }
        
        // Validate beam parameters
        if (config.default_beam_params.beam_width == 0 || config.default_beam_params.max_depth == 0) {
            return false;
        }
        
        // Validate NLG configuration
        if (config.nlg_config.min_clause_confidence < 0.0 || config.nlg_config.min_clause_confidence > 1.0) {
            return false;
        }
        
        // Validate mining configuration
        if (config.mining_config.theta_node < 0.0 || config.mining_config.theta_pat < 0.0) {
            return false;
        }
        
        return true;
        
    } catch (...) {
        return false;
    }
}

GraphReasoningConfig ConfigManager::parse_config_json(const std::string& json_str) {
    // Simplified JSON parsing - in practice you'd want a proper JSON library
    // For now, just return default config
    return get_default_config();
}

std::string ConfigManager::config_to_json(const GraphReasoningConfig& config) {
    // Simplified JSON serialization - in practice you'd want a proper JSON library
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"confidence_threshold_low\": " << config.confidence_threshold_low << ",\n";
    oss << "  \"confidence_threshold_high\": " << config.confidence_threshold_high << ",\n";
    oss << "  \"enable_learning\": " << (config.enable_learning ? "true" : "false") << ",\n";
    oss << "  \"enable_mining\": " << (config.enable_mining ? "true" : "false") << ",\n";
    oss << "  \"max_response_length\": " << config.max_response_length << "\n";
    oss << "}\n";
    
    return oss.str();
}

// MelvinGraphReasoning implementation
MelvinGraphReasoning::MelvinGraphReasoning(const std::string& store_dir, const GraphReasoningConfig& config)
    : reasoning_system_(std::make_unique<GraphReasoningSystem>(store_dir, config)) {}

const char* MelvinGraphReasoning::reason(const char* query) {
    if (!query) return "Invalid query";
    
    std::string result = reasoning_system_->reason(query);
    
    // Store result in a static buffer for C API compatibility
    // In practice, you'd want better memory management
    static std::string last_result;
    last_result = result;
    return last_result.c_str();
}

std::string MelvinGraphReasoning::reason_with_start_nodes(const Query& query) {
    if (!reasoning_system_) return "";
    
    return reasoning_system_->reason_with_start_nodes(query);
}

void MelvinGraphReasoning::learn(const char* text) {
    if (!text) return;
    
    reasoning_system_->learn(text);
}

void MelvinGraphReasoning::decay_pass() {
    reasoning_system_->decay_pass();
}

size_t MelvinGraphReasoning::node_count() const {
    return reasoning_system_->get_node_count();
}

size_t MelvinGraphReasoning::edge_count() const {
    return reasoning_system_->get_edge_count();
}

size_t MelvinGraphReasoning::path_count() const {
    return reasoning_system_->get_path_count();
}

float MelvinGraphReasoning::health_score() const {
    return static_cast<float>(reasoning_system_->get_health_score());
}

void MelvinGraphReasoning::set_config(const GraphReasoningConfig& config) {
    reasoning_system_->set_config(config);
}

const GraphReasoningConfig& MelvinGraphReasoning::get_config() const {
    return reasoning_system_->get_config();
}

} // namespace melvin
