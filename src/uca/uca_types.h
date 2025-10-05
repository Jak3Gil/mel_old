#pragma once
#include <string>
#include <vector>
#include <cstdint>
#include <optional>
#include <memory>

namespace uca {

// ---- Gate Decision System ----
enum class Gate { Emit, Ask, Listen };

struct GateState { 
  Gate last = Gate::Listen; 
  int ticks_since_change = 999; 
};

enum class Intent { 
  Factoid, 
  ControlRobot, 
  General 
};

// ---- Inputs / Percepts ----
struct InputConcept {
  std::string modality;        // "text", "audio", "image", ...
  std::string raw;             // raw text or compact code string
  uint64_t    t_ns = 0;        // monotonic timestamp
};

struct Percept {
  // normalized, tokenized, or embedded form from PerceptionEngine
  std::vector<std::string> tokens;   // for text; or ids for audio/image codes
  std::string modality;
  uint64_t t_ns = 0;
};

// ---- Reasoning ----
enum class OutputType : uint8_t { None=0, Text=1, Action=2, Audio=3 };

struct ReasoningPath {
  // opaque ids/handles into your graph (fill with what your engine produces)
  std::vector<uint64_t> node_ids;
  float confidence = 0.0f;
};

struct ReasoningResult {
  OutputType type = OutputType::Text;
  std::string text;            // primary output (may be empty if Action/Audio)
  std::vector<ReasoningPath> used_paths; // for learning reinforcement
  float confidence = 0.0f;     // overall confidence
  
  // Enhanced metrics for testing and tuning
  struct ExtraMetrics {
    float entropy = -1.0f;           // B2: Path entropy
    float path_score = 0.0f;          // Raw path score before confidence
    float div_pen = 0.0f;            // A6: Diversity penalty
    float temporal_gap = 0.0f;       // A4: Temporal continuity gap
    float avg_deg_norm = 0.0f;      // A1: Average degree normalization
    float max_contradiction = 0.0f;  // A3: Max contradiction score
    float rel_prior_sum = 0.0f;     // A2: Sum of relation priors
    float multi_hop_disc = 0.0f;    // A5: Multi-hop discount
    
    // Log-domain re-centering metrics
    float path_log_e = 0.0f;        // Mean log edge score (natural log)
    float path_log10 = 0.0f;        // Human-friendly orders of magnitude
    float s_path = 0.0f;            // Shifted and scaled path log
    float top2_margin = 0.0f;       // Logit gap between top-2 paths
    
    // Confidence calibration terms
    float conf_b0 = 0.0f;           // Intercept term
    float conf_b1_term = 0.0f;      // Log path score term
    float conf_b2_term = 0.0f;      // Negative length term
    float conf_b3_term = 0.0f;      // Negative similarity term
    float conf_b4_term = 0.0f;      // Negative contradiction term
    float conf_logit = 0.0f;        // Final logit before sigmoid
  } extra;
};

// ---- Feedback / Learning ----
struct Feedback {
  // internal self-feedback + external user feedback (if any)
  float reward = 0.0f;  // [-1, +1] range; 0 = neutral
  std::optional<std::string> note;  // diagnostic
};

struct LearningDelta {
  size_t edges_updated = 0;
  size_t paths_reinforced = 0;
};

// ---- Reflection / Evolution ----
struct GenomeParam { std::string name; float value; float min_v; float max_v; };
struct DynamicGenome { std::vector<GenomeParam> params; };

struct ReflectReport {
  bool stagnating = false;
  std::string reason;     // short explanation
  std::optional<DynamicGenome> new_genome;
};

// ---- Scheduler Glue ----
struct MelvinParams {
  // add knobs that gate inference loop for runtime tuning
  uint32_t max_steps_per_tick = 1;
  float    min_conf_for_emit = 0.15f;
};

} // namespace uca
