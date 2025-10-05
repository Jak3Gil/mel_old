#include "Genome.hpp"
#include <random>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <cmath>

namespace melvin::evolution {

Genome::Genome() {
    genes.resize(GENOME_SIZE, 0.0);
    sigmas.resize(GENOME_SIZE, 0.15); // Default initial step size
}

void Genome::randomize(double gene_std, double sigma_init) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::normal_distribution<double> gene_dist(0.0, gene_std);
    
    for (size_t i = 0; i < GENOME_SIZE; ++i) {
        genes[i] = gene_dist(gen);
        sigmas[i] = sigma_init;
    }
}

std::unordered_map<std::string, double> Genome::toPhenotype() const {
    std::unordered_map<std::string, double> params;
    
    // A) Learning / Memory (8 parameters)
    params["learn_enabled"] = bool_positive(genes[ParameterMap::LEARN_ENABLED - 1]);
    params["canonicalize_enabled"] = bool_positive(genes[ParameterMap::CANONICALIZE_ENABLED - 1]);
    params["alias_store_enabled"] = bool_positive(genes[ParameterMap::ALIAS_STORE_ENABLED - 1]);
    params["max_aliases_per_node"] = int_floor(genes[ParameterMap::MAX_ALIASES_PER_NODE - 1]);
    params["append_batch_size"] = int_floor(genes[ParameterMap::APPEND_BATCH_SIZE - 1]);
    params["flush_interval"] = int_floor(genes[ParameterMap::FLUSH_INTERVAL - 1]);
    params["index_stride"] = int_floor(genes[ParameterMap::INDEX_STRIDE - 1]);
    params["edge_weight_init"] = exp_scale(genes[ParameterMap::EDGE_WEIGHT_INIT - 1], 0);
    
    // B) Graph / Storage (10 parameters)
    params["edge_decay_rate"] = exp_scale(genes[ParameterMap::EDGE_DECAY_RATE - 1], 3);
    params["reinforce_step"] = exp_scale(genes[ParameterMap::REINFORCE_STEP - 1], 1);
    params["temporal_bias"] = sigmoid(genes[ParameterMap::TEMPORAL_BIAS - 1]);
    params["max_edge_fanout"] = int_floor(genes[ParameterMap::MAX_EDGE_FANOUT - 1]);
    params["loop_penalty"] = sigmoid(genes[ParameterMap::LOOP_PENALTY - 1]);
    params["path_length_bonus"] = sigmoid(genes[ParameterMap::PATH_LENGTH_BONUS - 1]);
    params["repetition_penalty"] = sigmoid(genes[ParameterMap::REPETITION_PENALTY - 1]);
    params["thoughtnode_macrohop_weight"] = sigmoid(genes[ParameterMap::THOUGHTNODE_MACROHOP_WEIGHT - 1]);
    params["node_id_size_bytes"] = 32 + 32 * bool_positive(genes[ParameterMap::NODE_ID_SIZE_BYTES - 1]);
    params["mmap_write_coalesce"] = int_floor(genes[ParameterMap::MMAP_WRITE_COALESCE - 1]);
    
    // C) Anchor Selection (8 parameters)
    params["anchor_exact_weight"] = sigmoid(genes[ParameterMap::ANCHOR_EXACT_WEIGHT - 1]);
    params["anchor_lemma_weight"] = sigmoid(genes[ParameterMap::ANCHOR_LEMMA_WEIGHT - 1]);
    params["anchor_alias_weight"] = sigmoid(genes[ParameterMap::ANCHOR_ALIAS_WEIGHT - 1]);
    params["anchor_similarity_tau"] = sigmoid(genes[ParameterMap::ANCHOR_SIMILARITY_TAU - 1]);
    params["max_anchor_candidates"] = 1 + int_floor(genes[ParameterMap::MAX_ANCHOR_CANDIDATES - 1]);
    params["anchor_conf_penalty"] = sigmoid(genes[ParameterMap::ANCHOR_CONF_PENALTY - 1]);
    params["nearest_token_ngram_power"] = exp_scale(genes[ParameterMap::NEAREST_TOKEN_NGRAM_POWER - 1], 0);
    params["multi_anchor_beam_share"] = sigmoid(genes[ParameterMap::MULTI_ANCHOR_BEAM_SHARE - 1]);
    
    // D) Reasoning / Search (12 parameters)
    params["beam_width"] = int_floor(genes[ParameterMap::BEAM_WIDTH - 1]);
    params["max_hops"] = int_floor(genes[ParameterMap::MAX_HOPS - 1]);
    params["iterative_deepening"] = bool_positive(genes[ParameterMap::ITERATIVE_DEEPENING - 1]);
    params["scoring_laplace_k"] = exp_scale(genes[ParameterMap::SCORING_LAPLACE_K - 1], 0);
    params["bigram_repeat_penalty"] = sigmoid(genes[ParameterMap::BIGRAM_REPEAT_PENALTY - 1]);
    params["loop_detect_hardcap"] = int_floor(genes[ParameterMap::LOOP_DETECT_HARDCAP - 1]);
    params["recency_weight"] = sigmoid(genes[ParameterMap::RECENCY_WEIGHT - 1]);
    params["semantic_fit_weight"] = sigmoid(genes[ParameterMap::SEMANTIC_FIT_WEIGHT - 1]);
    params["anchor_boost_weight"] = sigmoid(genes[ParameterMap::ANCHOR_BOOST_WEIGHT - 1]);
    params["thoughtnode_reuse_bias"] = sigmoid(genes[ParameterMap::THOUGHTNODE_REUSE_BIAS - 1]);
    params["path_merge_tolerance"] = sigmoid(genes[ParameterMap::PATH_MERGE_TOLERANCE - 1]);
    params["idbs_depth_increment"] = int_floor(genes[ParameterMap::IDBS_DEPTH_INCREMENT - 1]);
    
    // E) Output Assembly (8 parameters)
    params["retain_function_words"] = bool_positive(genes[ParameterMap::RETAIN_FUNCTION_WORDS - 1]);
    params["grammar_fix_enabled"] = bool_positive(genes[ParameterMap::GRAMMAR_FIX_ENABLED - 1]);
    params["capitalization_enabled"] = bool_positive(genes[ParameterMap::CAPITALIZATION_ENABLED - 1]);
    params["punctuation_enabled"] = bool_positive(genes[ParameterMap::PUNCTUATION_ENABLED - 1]);
    params["max_phrase_length"] = int_floor(genes[ParameterMap::MAX_PHRASE_LENGTH - 1]);
    params["duplicate_word_penalty"] = sigmoid(genes[ParameterMap::DUPLICATE_WORD_PENALTY - 1]);
    params["clause_glue_weight"] = sigmoid(genes[ParameterMap::CLAUSE_GLUE_WEIGHT - 1]);
    params["subject_verb_agreement_weight"] = sigmoid(genes[ParameterMap::SUBJECT_VERB_AGREEMENT_WEIGHT - 1]);
    
    // F) Confidence & Abstain (6 parameters)
    params["conf_threshold_definitional"] = sigmoid(genes[ParameterMap::CONF_THRESHOLD_DEFINITIONAL - 1]);
    params["conf_threshold_factual"] = sigmoid(genes[ParameterMap::CONF_THRESHOLD_FACTUAL - 1]);
    params["conf_threshold_arithmetic"] = sigmoid(genes[ParameterMap::CONF_THRESHOLD_ARITHMETIC - 1]);
    params["conf_threshold_fallback_penalty"] = sigmoid(genes[ParameterMap::CONF_THRESHOLD_FALLBACK_PENALTY - 1]);
    params["topk_agreement_weight"] = sigmoid(genes[ParameterMap::TOPK_AGREEMENT_WEIGHT - 1]);
    params["abstain_bias"] = sigmoid(genes[ParameterMap::ABSTAIN_BIAS - 1]);
    
    // G) Drivers / Meta (3 parameters)
    params["leap_bias"] = sigmoid(genes[ParameterMap::LEAP_BIAS - 1]);
    params["abstraction_thresh"] = sigmoid(genes[ParameterMap::ABSTRACTION_THRESH - 1]);
    params["temporal_weight_inc"] = exp_scale(genes[ParameterMap::TEMPORAL_WEIGHT_INC - 1], 1);
    
    return params;
}

bool Genome::isValid() const {
    if (genes.size() != GENOME_SIZE || sigmas.size() != GENOME_SIZE) {
        return false;
    }
    
    // Check for NaN or infinite values
    for (size_t i = 0; i < GENOME_SIZE; ++i) {
        if (!std::isfinite(genes[i]) || !std::isfinite(sigmas[i]) || sigmas[i] <= 0.0) {
            return false;
        }
    }
    
    return true;
}

std::string Genome::toCSV() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6);
    
    oss << generation << "," << id << "," << fitness << "," 
        << fitness_mini << "," << fitness_agi << "," << correctness << "," 
        << speed << "," << creativity << ",";
    
    // Add genes
    for (size_t i = 0; i < genes.size(); ++i) {
        oss << genes[i];
        if (i < genes.size() - 1) oss << ",";
    }
    
    oss << ",";
    
    // Add sigmas
    for (size_t i = 0; i < sigmas.size(); ++i) {
        oss << sigmas[i];
        if (i < sigmas.size() - 1) oss << ",";
    }
    
    // Add key phenotype values for quick analysis
    auto params = toPhenotype();
    oss << "," << params.at("beam_width") << "," << params.at("max_hops") 
        << "," << params.at("conf_threshold_definitional") << "," 
        << params.at("edge_decay_rate") << "," << params.at("reinforce_step");
    
    return oss.str();
}

std::string Genome::toJSON() const {
    std::ostringstream oss;
    oss << "{\n";
    oss << "  \"generation\": " << generation << ",\n";
    oss << "  \"id\": " << id << ",\n";
    oss << "  \"fitness\": " << std::fixed << std::setprecision(6) << fitness << ",\n";
    oss << "  \"correctness\": " << correctness << ",\n";
    oss << "  \"speed\": " << speed << ",\n";
    oss << "  \"creativity\": " << creativity << ",\n";
    oss << "  \"genes\": [";
    
    for (size_t i = 0; i < genes.size(); ++i) {
        oss << genes[i];
        if (i < genes.size() - 1) oss << ", ";
    }
    
    oss << "],\n";
    oss << "  \"sigmas\": [";
    
    for (size_t i = 0; i < sigmas.size(); ++i) {
        oss << sigmas[i];
        if (i < sigmas.size() - 1) oss << ", ";
    }
    
    oss << "]\n";
    oss << "}";
    
    return oss.str();
}

void Genome::printSummary() const {
    std::cout << "=== Genome Summary ===\n";
    std::cout << "Generation: " << generation << ", ID: " << id << "\n";
    std::cout << "Fitness: " << std::fixed << std::setprecision(4) << fitness 
              << " (Mini: " << fitness_mini << ", AGI: " << fitness_agi << ")\n";
    std::cout << "Components: C=" << correctness << ", S=" << speed << ", K=" << creativity << "\n";
    
    auto params = toPhenotype();
    std::cout << "Key Parameters:\n";
    std::cout << "  Beam Width: " << static_cast<int>(params.at("beam_width")) << "\n";
    std::cout << "  Max Hops: " << static_cast<int>(params.at("max_hops")) << "\n";
    std::cout << "  Conf Threshold: " << params.at("conf_threshold_definitional") << "\n";
    std::cout << "  Edge Decay: " << params.at("edge_decay_rate") << "\n";
    std::cout << "  Reinforce Step: " << params.at("reinforce_step") << "\n";
    std::cout << "=====================\n";
}

} // namespace melvin::evolution
