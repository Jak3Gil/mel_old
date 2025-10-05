#pragma once

#include <vector>
#include <string>
#include <unordered_map>

namespace melvin::evolution {

/**
 * Melvin's Evolution System - Genome Representation
 * 
 * 55-parameter genome vector with self-adaptive evolution
 * Each gene has its own mutation step size (sigma)
 */

struct Genome {
    static constexpr size_t GENOME_SIZE = 55;
    
    // Core genome data
    std::vector<double> genes;      // g1..g55 (unbounded real values)
    std::vector<double> sigmas;     // σ1..σ55 (mutation step sizes)
    
    // Fitness metrics
    double fitness = 0.0;           // F (final fitness)
    double correctness = 0.0;       // C
    double speed = 0.0;             // S  
    double creativity = 0.0;        // K (novelty + entropy)
    
    // Evaluation scores
    double fitness_mini = 0.0;      // From mini eval harness
    double fitness_agi = 0.0;       // From AGI test (every 5 generations)
    
    // Evolution metadata
    int generation = 0;
    int id = 0;
    bool evaluated = false;
    
    Genome();
    Genome(const Genome& other) = default;
    Genome& operator=(const Genome& other) = default;
    
    // Initialize with random values
    void randomize(double gene_std = 1.0, double sigma_init = 0.15);
    
    // Phenotype mapping functions
    double sigmoid(double x) const { return 1.0 / (1.0 + std::exp(-x)); }
    double exp_scale(double x, int k) const { return std::exp(x) * std::pow(10.0, -k); }
    int int_floor(double x) const { return static_cast<int>(std::floor(std::abs(x))) + 1; }
    bool bool_positive(double x) const { return x > 0.0; }
    
    // Map genome to brain parameters
    std::unordered_map<std::string, double> toPhenotype() const;
    
    // Validation
    bool isValid() const;
    
    // Serialization
    std::string toCSV() const;
    std::string toJSON() const;
    
    // Pretty printing
    void printSummary() const;
};

/**
 * Phenotype parameter names and their genome indices
 * Organized by functional categories
 */
struct ParameterMap {
    // A) Learning / Memory (8 parameters)
    static constexpr int LEARN_ENABLED = 1;
    static constexpr int CANONICALIZE_ENABLED = 2;
    static constexpr int ALIAS_STORE_ENABLED = 3;
    static constexpr int MAX_ALIASES_PER_NODE = 4;
    static constexpr int APPEND_BATCH_SIZE = 5;
    static constexpr int FLUSH_INTERVAL = 6;
    static constexpr int INDEX_STRIDE = 7;
    static constexpr int EDGE_WEIGHT_INIT = 8;
    
    // B) Graph / Storage (10 parameters)
    static constexpr int EDGE_DECAY_RATE = 9;
    static constexpr int REINFORCE_STEP = 10;
    static constexpr int TEMPORAL_BIAS = 11;
    static constexpr int MAX_EDGE_FANOUT = 12;
    static constexpr int LOOP_PENALTY = 13;
    static constexpr int PATH_LENGTH_BONUS = 14;
    static constexpr int REPETITION_PENALTY = 15;
    static constexpr int THOUGHTNODE_MACROHOP_WEIGHT = 16;
    static constexpr int NODE_ID_SIZE_BYTES = 17;
    static constexpr int MMAP_WRITE_COALESCE = 18;
    
    // C) Anchor Selection (8 parameters)
    static constexpr int ANCHOR_EXACT_WEIGHT = 19;
    static constexpr int ANCHOR_LEMMA_WEIGHT = 20;
    static constexpr int ANCHOR_ALIAS_WEIGHT = 21;
    static constexpr int ANCHOR_SIMILARITY_TAU = 22;
    static constexpr int MAX_ANCHOR_CANDIDATES = 23;
    static constexpr int ANCHOR_CONF_PENALTY = 24;
    static constexpr int NEAREST_TOKEN_NGRAM_POWER = 25;
    static constexpr int MULTI_ANCHOR_BEAM_SHARE = 26;
    
    // D) Reasoning / Search (12 parameters)
    static constexpr int BEAM_WIDTH = 27;
    static constexpr int MAX_HOPS = 28;
    static constexpr int ITERATIVE_DEEPENING = 29;
    static constexpr int SCORING_LAPLACE_K = 30;
    static constexpr int BIGRAM_REPEAT_PENALTY = 31;
    static constexpr int LOOP_DETECT_HARDCAP = 32;
    static constexpr int RECENCY_WEIGHT = 33;
    static constexpr int SEMANTIC_FIT_WEIGHT = 34;
    static constexpr int ANCHOR_BOOST_WEIGHT = 35;
    static constexpr int THOUGHTNODE_REUSE_BIAS = 36;
    static constexpr int PATH_MERGE_TOLERANCE = 37;
    static constexpr int IDBS_DEPTH_INCREMENT = 38;
    
    // E) Output Assembly (8 parameters)
    static constexpr int RETAIN_FUNCTION_WORDS = 39;
    static constexpr int GRAMMAR_FIX_ENABLED = 40;
    static constexpr int CAPITALIZATION_ENABLED = 41;
    static constexpr int PUNCTUATION_ENABLED = 42;
    static constexpr int MAX_PHRASE_LENGTH = 43;
    static constexpr int DUPLICATE_WORD_PENALTY = 44;
    static constexpr int CLAUSE_GLUE_WEIGHT = 45;
    static constexpr int SUBJECT_VERB_AGREEMENT_WEIGHT = 46;
    
    // F) Confidence & Abstain (6 parameters)
    static constexpr int CONF_THRESHOLD_DEFINITIONAL = 47;
    static constexpr int CONF_THRESHOLD_FACTUAL = 48;
    static constexpr int CONF_THRESHOLD_ARITHMETIC = 49;
    static constexpr int CONF_THRESHOLD_FALLBACK_PENALTY = 50;
    static constexpr int TOPK_AGREEMENT_WEIGHT = 51;
    static constexpr int ABSTAIN_BIAS = 52;
    
    // G) Drivers / Meta (3 parameters)
    static constexpr int LEAP_BIAS = 53;
    static constexpr int ABSTRACTION_THRESH = 54;
    static constexpr int TEMPORAL_WEIGHT_INC = 55;
};

} // namespace melvin::evolution
