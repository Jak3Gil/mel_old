#pragma once

#include "melvin_types.h"
#include "storage.h"
#include <vector>
#include <memory>
#include <stack>
#include <queue>
#include <random>
#include <cstddef>

namespace melvin {

// Path structure for beam search
struct Path {
    std::vector<EdgeID> edges;
    float score;
    float confidence;
    bool is_durable;
    
    Path() : score(0.0f), confidence(0.0f), is_durable(false) {}
    
    bool operator<(const Path& other) const {
        return score < other.score; // For max-heap
    }
};

// VM execution context
class VMContext {
private:
    Store* store_;
    Drivers drivers_;
    uint64_t seed_;
    std::mt19937_64 rng_;
    
    // Execution state
    std::stack<NodeID> node_stack_;
    std::stack<EdgeID> edge_stack_;
    std::stack<float> float_stack_;
    std::stack<Path> path_stack_;
    
    // Beam search state
    std::priority_queue<Path> beam_;
    size_t beam_size_;
    size_t max_depth_;
    
    // Statistics
    size_t steps_executed_;
    size_t memory_usage_;
    
    // Configuration
    float tau_exp_;      // exploration temperature
    float lambda_mix_;   // ctx/core mix ratio
    float tau_mid_;      // hypothesis threshold
    float tau_high_;     // durable threshold
    float tau_rec_;      // recency factor
    float gamma_c_;      // continuity bonus
    float gamma_v_;      // coverage bonus
    
    // Anti-repeat state
    std::unordered_set<std::string> recent_bigrams_;
    std::unordered_set<std::string> recent_trigrams_;
    size_t anti_repeat_window_;
    
public:
    VMContext(Store* store, uint64_t seed);
    ~VMContext() = default;
    
    // Stack operations
    void push_node(const NodeID& node);
    NodeID pop_node();
    void push_edge(const EdgeID& edge);
    EdgeID pop_edge();
    void push_float(float value);
    float pop_float();
    void push_path(const Path& path);
    Path pop_path();
    
    // Beam search operations
    void init_beam(size_t size, size_t max_depth);
    void add_to_beam(const Path& path);
    Path get_best_path();
    std::vector<Path> get_top_paths(size_t k);
    void clear_beam();
    
    // Path scoring
    float score_edge(const EdgeRec& edge) const;
    float score_path(const Path& path) const;
    float compute_confidence(float path_score) const;
    
    // Traversal operations
    std::vector<Path> beam_search(const NodeID& start, const RelMask& mask, 
                                 size_t depth, size_t beam_size);
    std::vector<Path> explore_paths(const NodeID& start, const RelMask& mask, 
                                   size_t depth, size_t num_samples);
    
    // Learning operations
    void reinforce_edge(const EdgeID& edge, float alpha_ctx, float alpha_core);
    void decay_edge(const EdgeID& edge, float beta_ctx, float beta_core);
    
    // Dynamic connection creation during reasoning
    EdgeID create_inferred_edge(const NodeID& src, const NodeID& dst, Rel relation, float confidence);
    std::vector<EdgeID> infer_missing_connections(const Path& path, float min_confidence);
    bool should_create_connection(const NodeID& src, const NodeID& dst, Rel relation, float confidence) const;
    
    // Output generation
    bool generate_text_output(const NodeID& node, ByteBuf& out);
    bool generate_audio_output(const NodeID& node, ByteBuf& out);
    bool generate_control_output(const NodeID& node, ByteBuf& out);
    
    // Anti-repeat
    bool check_repetition(const std::string& text) const;
    void add_to_recent(const std::string& text);
    
    // Size-relative weight scaling
    float get_size_scaling_factor() const;
    
    // Getters and setters
    Store* get_store() const { return store_; }
    const Drivers& get_drivers() const { return drivers_; }
    void set_drivers(const Drivers& drivers) { drivers_ = drivers; }
    uint64_t get_seed() const { return seed_; }
    void set_seed(uint64_t seed) { seed_ = seed; rng_.seed(seed); }
    
    size_t get_steps_executed() const { return steps_executed_; }
    void increment_steps() { steps_executed_++; }
    size_t get_memory_usage() const { return memory_usage_; }
    float get_tau_high() const { return tau_high_; }
    
    // Configuration
    void set_beam_size(size_t size) { beam_size_ = size; }
    void set_max_depth(size_t depth) { max_depth_ = depth; }
    void set_thresholds(float tau_mid, float tau_high) { 
        tau_mid_ = tau_mid; 
        tau_high_ = tau_high; 
    }
    void set_scoring_params(float tau_rec, float gamma_c, float gamma_v) {
        tau_rec_ = tau_rec;
        gamma_c_ = gamma_c;
        gamma_v_ = gamma_v;
    }
    
    // Random number generation
    float random_float() { return std::uniform_real_distribution<float>(0.0f, 1.0f)(rng_); }
    int random_int(int min, int max) { return std::uniform_int_distribution<int>(min, max)(rng_); }
    
    // Inference rules and confidence thresholds
    float min_inference_confidence_ = 0.3f;  // Minimum confidence to create inferred edges
    float exploration_bonus_ = 0.1f;         // Bonus for exploring new connections
    bool enable_dynamic_inference_ = true;   // Enable dynamic edge creation
};

// VM implementation
class MelvinVM : public VM {
private:
    std::unique_ptr<VMContext> context_;
    
public:
    MelvinVM(Store* store, uint64_t seed);
    ~MelvinVM() override = default;
    
    void exec(const std::vector<uint8_t>& bytecode) override;
    void exec(const uint8_t* bytecode, size_t len) override;
    
    void set_drivers(const Drivers& drivers) override;
    Drivers get_drivers() const override;
    void set_seed(uint64_t seed) override;
    
    bool emit_view(const NodeID& node, ViewKind kind, ByteBuf& out) override;
    
    size_t steps_executed() const override;
    size_t memory_usage() const override;
    
    // Additional VM operations
    VMContext* get_context() { return context_.get(); }
};

// Bytecode instruction decoder
class BytecodeDecoder {
private:
    const uint8_t* code_;
    size_t pos_;
    size_t len_;
    
public:
    BytecodeDecoder(const uint8_t* code, size_t len) : code_(code), pos_(0), len_(len) {}
    
    bool has_more() const { return pos_ < len_; }
    uint8_t read_u8();
    uint16_t read_u16();
    uint32_t read_u32();
    uint64_t read_u64();
    float read_f32();
    NodeID read_node_id();
    EdgeID read_edge_id();
    RelMask read_rel_mask();
    
    void skip(size_t bytes) { pos_ += bytes; }
    size_t position() const { return pos_; }
    void set_position(size_t pos) { pos_ = pos; }
};

// COG_STEP trace recorder
class CogStepRecorder {
private:
    Store* store_;
    std::vector<CogStep> steps_;
    
public:
    CogStepRecorder(Store* store) : store_(store) {}
    
    void record_step(CogOp op, float conf, const Drivers& drivers,
                    const std::vector<NodeID>& inputs,
                    const std::vector<NodeID>& outputs,
                    uint64_t latency_us, ErrorFlags err_flags);
    
    const std::vector<CogStep>& get_steps() const { return steps_; }
    void clear_steps() { steps_.clear(); }
};

} // namespace melvin
