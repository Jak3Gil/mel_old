/*
 * MELVIN LLM-STYLE TUNING CONFIGURATION
 * 
 * Ready-to-use configuration parameters for tuning the LLM-style upgrades
 */

#ifndef MELVIN_TUNING_CONFIG_H
#define MELVIN_TUNING_CONFIG_H

namespace melvin {
namespace tuning {

// 1) Edge fanout tuning (k=8 → 12 → 16)
struct FanoutConfig {
    int min_fanout = 8;
    int default_fanout = 12;
    int max_fanout = 16;
    float dropout_rate = 0.1f;  // Prevent attention blow-up
};

// 2) Temperature tuning (0.2/0.4/0.7)
struct TemperatureConfig {
    float conservative = 0.2f;  // Low diversity, high determinism
    float balanced = 0.4f;      // Default setting
    float creative = 0.7f;       // High diversity, more creative
    float top_p_min = 0.9f;
    float top_p_max = 0.95f;
};

// 3) Repetition penalty tuning
struct RepetitionConfig {
    float start_penalty = 1.05f;
    float max_penalty = 1.15f;
    int ngram_block_size = 3;   // Block 3-gram repetitions
    float loop_detection_threshold = 0.8f;
};

// 4) Leap threshold tuning (cosine similarity)
struct LeapConfig {
    float conservative_threshold = 0.57f;
    float default_threshold = 0.62f;
    float aggressive_threshold = 0.67f;
    float path_plausibility_weight = 0.5f;
    float min_path_score = 0.5f;
};

// 5) Context decay tuning
struct ContextConfig {
    int conservative_decay = 6;   // Faster forgetting
    int default_decay = 8;        // Balanced
    int persistent_decay = 12;    // Slower forgetting
    float temporal_weight = 0.3f;
    float semantic_weight = 0.7f;
};

// 6) Dual-state evolution tuning
struct EvolutionConfig {
    int consolidation_frequency = 500;  // Every N requests
    int time_based_consolidation = 1200; // 20 minutes in seconds
    float fast_to_slow_rate = 0.1f;     // 10% blend rate
    float drift_threshold = 0.05f;       // Alert if drift > 5%
    bool freeze_persona_anchors = true;
};

// 7) Metric guardrails
struct GuardrailConfig {
    // Attention entropy bounds
    float attn_entropy_min = 0.08f;
    float attn_entropy_max = 0.35f;
    
    // Output diversity bounds
    float diversity_min = 0.45f;
    float diversity_max = 0.85f;
    
    // Decision confidence
    float top2_margin_min = 0.2f;
    
    // Performance limits
    int max_fanout = 16;
    int64_t latency_p95_max_ms = 50;
    
    // Quality thresholds
    float embedding_coherence_min = 0.55f;
    float context_overlap_min = 0.4f;
};

// 8) Embedding configuration
struct EmbeddingConfig {
    int dimension = 128;
    float learning_rate = 0.001f;
    float similarity_threshold = 0.65f;
    int top_neighbors = 5;
    float coherence_target = 0.55f;
    int backfill_batch_size = 100;
};

// 9) Multimodal projection weights
struct MultimodalConfig {
    float audio_weight = 0.3f;
    float image_weight = 0.3f;
    float text_weight = 0.4f;
    float cross_modal_penalty = 0.1f;
    int projection_dimension = 64;
};

// 10) Complete tuning profile
struct TuningProfile {
    FanoutConfig fanout;
    TemperatureConfig temperature;
    RepetitionConfig repetition;
    LeapConfig leap;
    ContextConfig context;
    EvolutionConfig evolution;
    GuardrailConfig guardrails;
    EmbeddingConfig embedding;
    MultimodalConfig multimodal;
    
    // Profile names for easy switching
    static TuningProfile conservative() {
        TuningProfile profile;
        profile.fanout.default_fanout = 8;
        profile.temperature.balanced = 0.2f;
        profile.leap.default_threshold = 0.67f;
        profile.context.default_decay = 6;
        return profile;
    }
    
    static TuningProfile balanced() {
        TuningProfile profile; // Use defaults
        return profile;
    }
    
    static TuningProfile creative() {
        TuningProfile profile;
        profile.fanout.default_fanout = 16;
        profile.temperature.balanced = 0.7f;
        profile.leap.default_threshold = 0.57f;
        profile.context.default_decay = 12;
        profile.repetition.start_penalty = 1.1f;
        return profile;
    }
};

// Utility functions for tuning
class TuningUtils {
public:
    // Apply tuning profile to UCAConfig
    static void apply_profile(UCAConfig& config, const TuningProfile& profile) {
        config.attention_top_k = profile.fanout.default_fanout;
        config.attention_dropout = profile.fanout.dropout_rate;
        config.output_temperature = profile.temperature.balanced;
        config.output_top_p = profile.temperature.top_p_max;
        config.repetition_penalty = profile.repetition.start_penalty;
        config.leap_threshold = profile.leap.default_threshold;
        config.context_decay_half_life = profile.context.default_decay;
        config.consolidation_frequency = profile.evolution.consolidation_frequency;
        config.slow_param_blend_rate = profile.evolution.fast_to_slow_rate;
        config.embedding_dim = profile.embedding.dimension;
        config.embedding_learning_rate = profile.embedding.learning_rate;
    }
    
    // Check if metrics are within guardrails
    static bool check_guardrails(const GuardrailConfig& guardrails,
                                 float attention_entropy,
                                 float output_diversity,
                                 float top2_margin,
                                 int fanout,
                                 int64_t latency_ms) {
        return (attention_entropy >= guardrails.attn_entropy_min &&
                attention_entropy <= guardrails.attn_entropy_max &&
                output_diversity >= guardrails.diversity_min &&
                output_diversity <= guardrails.diversity_max &&
                top2_margin >= guardrails.top2_margin_min &&
                fanout <= guardrails.max_fanout &&
                latency_ms <= guardrails.latency_p95_max_ms);
    }
    
    // Generate tuning report
    static std::string generate_tuning_report(const TuningProfile& profile,
                                              const std::vector<float>& metrics) {
        std::ostringstream report;
        report << "🎛️ TUNING REPORT\n";
        report << "================\n";
        report << "Fanout: " << profile.fanout.default_fanout << "\n";
        report << "Temperature: " << profile.temperature.balanced << "\n";
        report << "Leap threshold: " << profile.leap.default_threshold << "\n";
        report << "Context decay: " << profile.context.default_decay << "\n";
        report << "Repetition penalty: " << profile.repetition.start_penalty << "\n";
        
        if (!metrics.empty()) {
            report << "\n📊 METRICS:\n";
            report << "Attention entropy: " << metrics[0] << "\n";
            report << "Output diversity: " << metrics[1] << "\n";
            report << "Top2 margin: " << metrics[2] << "\n";
            report << "Context overlap: " << metrics[3] << "\n";
            report << "Health score: " << metrics[4] << "\n";
        }
        
        return report.str();
    }
};

} // namespace tuning
} // namespace melvin

#endif // MELVIN_TUNING_CONFIG_H
