#pragma once

/*
 * ╔═══════════════════════════════════════════════════════════════════════════╗
 * ║  MELVIN UNIFIED CONFIGURATION                                             ║
 * ║  All runtime parameters in one place                                      ║
 * ╚═══════════════════════════════════════════════════════════════════════════╝
 */

#include <string>

namespace melvin {
namespace config {

// ============================================================================
// STORAGE PATHS
// ============================================================================

constexpr const char* STORAGE_PATH_NODES = "data/nodes.melvin";
constexpr const char* STORAGE_PATH_EDGES = "data/edges.melvin";
constexpr const char* SNAPSHOT_DIR = "data/snapshots/";
constexpr const char* INBOX_DIR = "data/inbox/";

// ============================================================================
// REASONING PARAMETERS
// ============================================================================

// Energy-Based Dynamic Reasoning Depth (replaces fixed MAX_HOPS)
constexpr float ENERGY_DECAY = 0.9f;        // Per-hop decay multiplier
constexpr float ENERGY_EPSILON = 1e-5f;     // Smallest meaningful signal per edge
constexpr float ENERGY_FLOOR = 1e-3f;       // Stop when total energy < this
constexpr float ENERGY_CAP = 10.0f;         // Safety limit for runaway feedback
constexpr int TIME_BUDGET_MS = 100;         // Reasoning time cap (failsafe)
constexpr int MAX_HOPS_SAFETY = 50;         // Hard limit to prevent infinite loops

// Adaptive Local Weighting (scales with graph size)
constexpr float WEIGHT_ALPHA = 1.0f;        // Emphasis on local count
constexpr float WEIGHT_BETA = 0.3f;         // Mild scaling to stabilize growth
constexpr float WEIGHT_DECAY = 0.999f;      // Prevent old data from dominating
constexpr int WEIGHT_UPDATE_INTERVAL = 10;  // Update every N reasoning cycles

// Graph Neural Network Predictor
constexpr int EMBED_DIM = 32;               // Embedding vector dimension
constexpr float GNN_LEARNING_RATE = 0.001f; // Learning rate for embeddings
constexpr float PREDICTION_DECAY = 0.9f;    // Embedding momentum
constexpr float PREDICTION_THRESHOLD = 0.3f;// Create LEAP if error > threshold
constexpr bool FULL_CONNECTIVITY = false;   // Dense attention (transformer-like)
constexpr int GNN_MESSAGE_PASSES = 3;       // Message passing iterations

// Energy Field (Attention & Autonomous Thinking)
constexpr float ENERGY_DECAY_RATE = 0.95f;  // Energy fade per step
constexpr float ENERGY_NOISE_FLOOR = 0.001f;// Minimum baseline activity
constexpr float CURIOSITY_GAIN = 0.05f;     // Prediction error → energy
constexpr float ENERGY_DIFFUSION_RATE = 0.1f; // Energy transfer rate
constexpr float ATTENTION_GAMMA = 2.0f;     // Attention concentration
constexpr float REGENERATION_THRESHOLD = 0.1f; // Min energy to keep thinking

// Multi-hop search (beam-based)
constexpr int BEAM_WIDTH = 8;
constexpr float LENGTH_PENALTY = 0.9f;

// LEAP thresholds
constexpr float LEAP_THRESHOLD = 0.7f;
constexpr float ABSTRACTION_THRESHOLD = 0.78f;
constexpr float LEAP_BIAS = 0.15f;

// Context field dynamics
constexpr float DIFFUSION_RATE = 0.1f;
constexpr float DECAY_RATE = 0.95f;
constexpr int DIFFUSION_STEPS = 10;

// ============================================================================
// LEARNING PARAMETERS
// ============================================================================

constexpr float LEARNING_RATE = 0.01f;
constexpr float EDGE_DECAY_RATE = 0.99f;
constexpr float CONSOLIDATION_RATE = 0.01f;

// SRS (Spaced Repetition System)
constexpr int SRS_INITIAL_INTERVAL_DAYS = 1;
constexpr float SRS_EASE_FACTOR_MIN = 1.3f;
constexpr float SRS_EASE_FACTOR_MAX = 2.5f;

// Emergent dimensions
constexpr float VARIANCE_IMPACT_THRESHOLD = 0.75f;
constexpr float INFLUENCE_WEIGHT_INIT = 0.1f;

// ============================================================================
// SNAPSHOT & MONITORING
// ============================================================================

constexpr int SNAPSHOT_INTERVAL_SECONDS = 300;  // 5 minutes
constexpr bool ENABLE_METRICS = true;
constexpr bool ENABLE_CONSOLE_LOGGING = true;
constexpr bool ENABLE_PATH_LOGGING = true;

// ============================================================================
// MULTIMODAL (OPTIONAL)
// ============================================================================

constexpr const char* AUDIO_MODEL = "whisper-tiny";
constexpr const char* CLIP_MODEL = "ViT-B/32";
constexpr bool ENABLE_AUDIO = false;
constexpr bool ENABLE_IMAGES = false;

// Embedding dimensions
constexpr int EMBEDDING_DIM = 128;
constexpr int LATENT_DIM = 256;

// ============================================================================
// CONTINUOUS LEARNING
// ============================================================================

constexpr bool ENABLE_AUTO_LEARNING = false;
constexpr int WATCH_INTERVAL_SECONDS = 5;

// ============================================================================
// LLM-LEVEL CAPABILITIES (NEW!)
// ============================================================================

// Tokenizer
constexpr int TOKENIZER_VOCAB_SIZE = 50000;
constexpr int MAX_TOKEN_LENGTH = 100;

// Sequence Predictor (Transformer)
constexpr int TRANSFORMER_D_MODEL = 512;
constexpr int TRANSFORMER_N_HEADS = 8;
constexpr int TRANSFORMER_N_LAYERS = 6;
constexpr int TRANSFORMER_CONTEXT_LEN = 2048;
constexpr float TRANSFORMER_DROPOUT = 0.1f;
constexpr float TRANSFORMER_LR = 0.0001f;

// Hybrid Predictor
constexpr float HYBRID_GRAPH_WEIGHT = 0.6f;        // Trust graph more for facts
constexpr float HYBRID_SEQUENCE_WEIGHT = 0.4f;    // Sequence for fluency
constexpr bool HYBRID_GRAPH_GATING = true;         // Graph can veto outputs
constexpr float HYBRID_GATING_THRESHOLD = 0.1f;

// Episodic Memory
constexpr size_t MAX_EPISODES = 10000;
constexpr uint64_t EPISODE_CONSOLIDATION_INTERVAL_MS = 3600000; // 1 hour
constexpr float EPISODE_SIMILARITY_THRESHOLD = 0.7f;

// Autonomous Learning
constexpr int AUTONOMOUS_TRAINING_EPOCHS = 100;
constexpr int AUTONOMOUS_BATCH_SIZE = 32;
constexpr bool ENABLE_SELF_EXPLORATION = true;
constexpr int EXPLORATION_INTERVAL = 50;  // Every N epochs

// Generator
constexpr int GENERATION_MAX_LENGTH = 256;
constexpr float GENERATION_TEMPERATURE = 0.8f;
constexpr float GENERATION_TOP_P = 0.9f;
constexpr bool GENERATION_GRAPH_CONSTRAINED = true;  // Prevent hallucinations!

// ============================================================================
// PERFORMANCE TUNING
// ============================================================================

constexpr int MAX_CONTEXT_SIZE = 32;
constexpr float ATTENTION_TEMPERATURE = 1.0f;
constexpr float OUTPUT_TEMPERATURE = 0.8f;
constexpr float REPETITION_PENALTY = 1.1f;

// ============================================================================
// FEATURE FLAGS
// ============================================================================

constexpr bool USE_SOFT_TRAVERSAL = true;
constexpr bool USE_PROBABILISTIC_OUTPUT = true;
constexpr bool USE_EMBEDDINGS = true;
constexpr bool DUAL_STATE_EVOLUTION = true;
constexpr bool USE_CONTEXT_BUFFER = true;

// ============================================================================
// RUNTIME CONFIGURATION STRUCT
// ============================================================================

/**
 * Runtime configuration that can be loaded from file or env vars
 */
struct RuntimeConfig {
    // Storage
    std::string storage_nodes_path = STORAGE_PATH_NODES;
    std::string storage_edges_path = STORAGE_PATH_EDGES;
    std::string snapshot_dir = SNAPSHOT_DIR;
    std::string inbox_dir = INBOX_DIR;
    
    // Reasoning (energy-based)
    float energy_decay = ENERGY_DECAY;
    float energy_floor = ENERGY_FLOOR;
    float energy_epsilon = ENERGY_EPSILON;
    int max_hops_safety = MAX_HOPS_SAFETY;
    int time_budget_ms = TIME_BUDGET_MS;
    int beam_width = BEAM_WIDTH;
    float leap_threshold = LEAP_THRESHOLD;
    float abstraction_threshold = ABSTRACTION_THRESHOLD;
    
    // Learning
    float learning_rate = LEARNING_RATE;
    float decay_rate = EDGE_DECAY_RATE;
    
    // Monitoring
    bool enable_metrics = ENABLE_METRICS;
    bool enable_console_logging = ENABLE_CONSOLE_LOGGING;
    int snapshot_interval_seconds = SNAPSHOT_INTERVAL_SECONDS;
    
    // Multimodal
    bool enable_audio = ENABLE_AUDIO;
    bool enable_images = ENABLE_IMAGES;
    std::string audio_model = AUDIO_MODEL;
    std::string clip_model = CLIP_MODEL;
    
    // Continuous learning
    bool enable_auto_learning = ENABLE_AUTO_LEARNING;
    int watch_interval_seconds = WATCH_INTERVAL_SECONDS;
    
    /**
     * Load config from environment variables
     */
    void load_from_env();
    
    /**
     * Load config from file (TOML, JSON, etc.)
     */
    bool load_from_file(const std::string& path);
    
    /**
     * Print current configuration
     */
    void print() const;
};

} // namespace config
} // namespace melvin

