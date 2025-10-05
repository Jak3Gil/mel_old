#include <chrono>
#include <thread>
#include <deque>
#include <iostream>
#include <cstdio>
#include <cstdlib>

#include "src/uca/uca_types.h"
#include "src/uca/PerceptionEngine.hpp"
#include "src/uca/ReasoningEngine.hpp"
#include "src/uca/LearningEngine.hpp"
#include "src/uca/ReflectionEngine.hpp"
#include "src/uca/OutputManager.hpp"
#include "src/uca/FeedbackBus.hpp"

using namespace std::chrono_literals;
using Clock = std::chrono::steady_clock;

// Environment variable switches for formula ablation
auto env_on = [](const char* k){ const char* v=getenv(k); return v && (*v=='1'); };
bool USE_DEGREE = !getenv("UCA_NO_DEGREE");
bool USE_RELPR  = !getenv("UCA_NO_RELPR");
bool USE_CONTRA = !getenv("UCA_NO_CONTRA");
bool USE_CONT   = !getenv("UCA_NO_CONT");
bool USE_DIV    = !getenv("UCA_NO_DIV");
bool USE_TD     = env_on("UCA_USE_TD");

// CSV metrics logging
static FILE* metrics_log = nullptr;

int main(int argc, char** argv) {
  using namespace uca;

  MelvinParams params{};
  
  // Tunable minimum confidence for emission
  params.min_conf_for_emit = getenv("UCA_MIN_CONF_EMIT") ? 
                             atof(getenv("UCA_MIN_CONF_EMIT")) : 0.15f;
  DynamicGenome genome{
    { {"beam_width", 4.f, 1.f, 16.f},
      {"max_hops",   4.f, 1.f, 16.f},
      {"leap_bias",  0.1f, 0.f, 1.f},
      {"abstr_thresh", 0.6f, 0.f, 1.f} }
  };

  PerceptionEngine perception;
  ReasoningEngine  reasoning;
  LearningEngine   learning;
  ReflectionEngine reflection;
  OutputManager    output;
  FeedbackBus      feedback;

  perception.configure(genome);
  reasoning.configure(genome);
  learning.configure(genome);
  reflection.configure(genome);
  output.configure(genome);

  std::deque<float> conf_hist;   // last N confidences
  std::deque<float> reward_hist; // last N rewards
  constexpr size_t HIST_MAX = 64;
  
  // Gate state for hysteresis
  GateState gate_state;

  std::cout << "[UCA] Starting cognitive loop\n";
  
  // Initialize CSV logging
  metrics_log = fopen("uca_metrics.csv", "a");
  if (metrics_log && ftell(metrics_log) == 0) {
    fprintf(metrics_log, "t,conf,entropy,path_score,len,div_pen,rec_gap,deg_norm,reward,formula_flags,path_log_e,path_log10,s_path,top2_margin,conf_b0,conf_b1_term,conf_b2_term,conf_b3_term,conf_b4_term,conf_logit\n");
  }
  
  // Print ablation status
  std::cout << "[UCA] Formula switches: DEGREE=" << USE_DEGREE 
            << " RELPR=" << USE_RELPR << " CONTRA=" << USE_CONTRA 
            << " CONT=" << USE_CONT << " DIV=" << USE_DIV << " TD=" << USE_TD << "\n";

  // Simple stdin feeder for now; replace with your real InputManager later
  std::string line;
  while (std::getline(std::cin, line)) {
    InputConcept ic;
    ic.modality = "text";
    ic.raw = line;
    ic.t_ns = (uint64_t)std::chrono::duration_cast<std::chrono::nanoseconds>(
                Clock::now().time_since_epoch()).count();

    // 1) Perceive
    Percept p = perception.perceive(ic);

    // 2) Reason
    ReasoningResult rr = reasoning.infer(p);

    // 3) Output (three-mode gate: Emit/Ask/Listen)
    bool emitted = false;
    
    // Detect intent and apply domain-specific thresholds
    Intent intent = detect_intent(p.tokens);
    float domain_threshold = threshold_for_intent(intent);
    
    // Update confidence history for dynamic thresholding
    conf_hist.push_back(rr.confidence);
    if (conf_hist.size() > HIST_MAX) conf_hist.pop_front();
    
    // Apply dynamic thresholding (quantile control)
    float q75 = rolling_quantile(conf_hist, 0.75f);
    float dynamic_threshold = std::max(0.08f, std::min(0.25f, 0.08f + 0.5f * q75));
    
    // Safety hardening checks
    bool safe_to_emit = safety_check(rr);
    
    // Three-mode gate decision
    auto raw_gate = decide_gate(rr.confidence, rr.extra.entropy, rr.extra.top2_margin);
    auto gate = hysteresis_gate(raw_gate, gate_state);
    
    if (gate == Gate::Emit && safe_to_emit) {
      emitted = output.emit(rr);
    } else if (gate == Gate::Ask) {
      // Generate clarifying question
      std::string clarifier = "Could you clarify what you're asking about?";
      if (intent == Intent::Factoid) {
        clarifier = "Are you asking for a definition or specific information about " + 
                   (p.tokens.empty() ? "this topic" : p.tokens[0]) + "?";
      } else if (intent == Intent::ControlRobot) {
        clarifier = "What specific action would you like me to perform?";
      }
      
      ReasoningResult ask_result;
      ask_result.type = OutputType::Text;
      ask_result.text = clarifier;
      ask_result.confidence = std::max(rr.confidence, 0.05f);
      emitted = output.emit(ask_result);
    } else {
      // Listen mode - suppressed emit
      std::cerr << "[UCA] listen (" << rr.confidence << "), suppressed emit\n";
    }

    // 4) Feedback (internal + external)
    Feedback fb = feedback.collect(rr);
    if (emitted) fb.reward = std::max(fb.reward, rr.confidence); // simple positive shaping

    // Log metrics to CSV
    if (metrics_log) {
      float entropy = rr.extra.entropy;
      float path_score = rr.extra.path_score;
      float len = rr.used_paths.empty() ? 0.f : static_cast<float>(rr.used_paths[0].node_ids.size());
      float div_pen = rr.extra.div_pen;
      float rec_gap = rr.extra.temporal_gap;
      float deg_norm = rr.extra.avg_deg_norm;
      
      // Create formula flags string
      std::string flags;
      if (USE_DEGREE) flags += "D";
      if (USE_RELPR) flags += "R";
      if (USE_CONTRA) flags += "C";
      if (USE_CONT) flags += "T";
      if (USE_DIV) flags += "V";
      if (USE_TD) flags += "M";
      
      fprintf(metrics_log, "%llu,%.5f,%.5f,%.5f,%.1f,%.5f,%.5f,%.5f,%.5f,%s,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f,%.5f\n",
              (unsigned long long)ic.t_ns, rr.confidence, entropy, path_score,
              len, div_pen, rec_gap, deg_norm, fb.reward, flags.c_str(),
              rr.extra.path_log_e, rr.extra.path_log10, rr.extra.s_path, rr.extra.top2_margin,
              rr.extra.conf_b0, rr.extra.conf_b1_term, rr.extra.conf_b2_term,
              rr.extra.conf_b3_term, rr.extra.conf_b4_term, rr.extra.conf_logit);
      fflush(metrics_log);
    }

    // 5) Learn (reinforce used paths)
    LearningDelta d = learning.reinforce(rr.used_paths, fb);

    // 6) Reflect (stagnation? micro-evolve?)
    conf_hist.push_back(rr.confidence);
    reward_hist.push_back(fb.reward);
    if (conf_hist.size() > HIST_MAX) conf_hist.pop_front();
    if (reward_hist.size() > HIST_MAX) reward_hist.pop_front();

    ReflectReport rep = reflection.reflect(conf_hist, reward_hist);
    if (rep.stagnating && rep.new_genome.has_value()) {
      genome = *rep.new_genome;         // hot-swap params (micro-evolution)
      // Reconfigure modules with new genome
      perception.configure(genome);
      reasoning.configure(genome);
      learning.configure(genome);
      reflection.configure(genome);
      output.configure(genome);
      std::cout << "[UCA] Applied micro-evolution: " << rep.reason << "\n";
    }

    // Small breathing room to keep logs readable
    std::this_thread::sleep_for(2ms);
  }

  std::cout << "[UCA] Loop terminated\n";
  
  // Close CSV log
  if (metrics_log) {
    fclose(metrics_log);
  }
  
  return 0;
}