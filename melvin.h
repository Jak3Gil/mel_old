#pragma once

#include "melvin_types.h"
#include <cstddef>

#ifdef __cplusplus
extern "C" {
#endif

// C API for Melvin
typedef struct melvin_t melvin_t;

// Core functions
melvin_t* melvin_create(const char* store_dir);
void melvin_destroy(melvin_t* melvin);

// Reasoning and learning
const char* melvin_reason(melvin_t* melvin, const char* query);
void melvin_learn(melvin_t* melvin, const char* text);
void melvin_decay_pass(melvin_t* melvin);

// Nightly consolidation
void melvin_run_nightly_consolidation(melvin_t* melvin);

// Statistics
size_t melvin_node_count(melvin_t* melvin);
size_t melvin_edge_count(melvin_t* melvin);
size_t melvin_path_count(melvin_t* melvin);

// Health monitoring
float melvin_get_health_score(melvin_t* melvin);
void melvin_export_metrics(melvin_t* melvin, const char* filename);

// Test functions
void melvin_setup_test_data(melvin_t* melvin);

#ifdef __cplusplus
}
#endif

// C++ API for Evolutionary Brain Simulation
#ifdef __cplusplus
#include <string>
#include <memory>
#include <vector>
#include <cstdint>

namespace melvin {

// Forward declarations
class Genome;
class EvolutionaryBrain;
class DataStreamGenerator;
class ProbeSystem;
class EvolutionOrchestrator;
class BrainSimulationRunner;

// Main simulation runner class
class BrainSimulationRunner {
public:
    BrainSimulationRunner(const std::string& output_dir, uint64_t master_seed = 1234567890);
    ~BrainSimulationRunner();
    
    void start_simulation();
    void stop_simulation();
    void run_single_generation();
    void export_final_results();
    
private:
    std::string output_dir_;
    std::unique_ptr<EvolutionOrchestrator> orchestrator_;
    bool running_ = false;
};

} // namespace melvin
#endif
