# 🧬 Melvin Evolution System

A comprehensive self-adaptive evolution system for optimizing Melvin's 55 brain parameters using genetic algorithms with advanced mutation strategies.

## 🎯 Overview

The Melvin Evolution System implements a sophisticated parameter optimization framework that treats Melvin's brain configuration as a 55-parameter genome. It uses self-adaptive evolution with multiple mutation strategies, fitness-based selection, and comprehensive logging.

## 🏗️ Architecture

### Core Components

- **`Genome.hpp/cpp`** - 55-parameter genome representation with phenotype mapping
- **`Evolution.hpp/cpp`** - Self-adaptive evolution engine with mutation and crossover
- **`Fitness.hpp/cpp`** - Multi-objective fitness evaluation (correctness, speed, creativity)
- **`Logging.hpp`** - Comprehensive logging and analysis system
- **`Integration.hpp`** - Integration utilities for connecting to Melvin
- **`main.cpp`** - Command-line interface and main evolution loop

### Key Features

- **55-Parameter Genome**: Complete representation of Melvin's brain configuration
- **Self-Adaptive Mutation**: Per-gene step sizes with 1/5 success rule
- **Multiple Mutation Strategies**: Gaussian, Cauchy heavy-tail, two-scale moves
- **Multi-Objective Fitness**: Correctness (60%), Speed (25%), Creativity (15%)
- **Comprehensive Logging**: CSV/JSON export, parameter trajectories, analysis
- **Runtime Evolution**: Optional continuous optimization during operation
- **Safety Checks**: Parameter validation and performance constraints

## 🚀 Quick Start

### Build the System

```bash
cd evolution/
make
```

### Run Basic Evolution

```bash
# Default run (40 population, 50 generations)
./melvin_evolution

# Fast run for testing
./melvin_evolution --fast --population 20 --generations 20

# Verbose output
./melvin_evolution --verbose
```

### Command Line Options

```bash
./melvin_evolution [options]

Options:
  --population SIZE     Population size (default: 40)
  --generations NUM     Max generations (default: 50)
  --output DIR          Output directory (default: evolution_logs/)
  --fast               Use mini eval only (faster)
  --verbose            Verbose output
  --seed NUM           Random seed
  --help               Show help
```

## 🧬 The 55-Parameter Genome

### Parameter Categories

**A) Learning / Memory (8 parameters)**
- `learn_enabled`, `canonicalize_enabled`, `alias_store_enabled`
- `max_aliases_per_node`, `append_batch_size`, `flush_interval`
- `index_stride`, `edge_weight_init`

**B) Graph / Storage (10 parameters)**
- `edge_decay_rate`, `reinforce_step`, `temporal_bias`
- `max_edge_fanout`, `loop_penalty`, `path_length_bonus`
- `repetition_penalty`, `thoughtnode_macrohop_weight`
- `node_id_size_bytes`, `mmap_write_coalesce`

**C) Anchor Selection (8 parameters)**
- `anchor_exact_weight`, `anchor_lemma_weight`, `anchor_alias_weight`
- `anchor_similarity_tau`, `max_anchor_candidates`
- `anchor_conf_penalty`, `nearest_token_ngram_power`
- `multi_anchor_beam_share`

**D) Reasoning / Search (12 parameters)**
- `beam_width`, `max_hops`, `iterative_deepening`
- `scoring_laplace_k`, `bigram_repeat_penalty`, `loop_detect_hardcap`
- `recency_weight`, `semantic_fit_weight`, `anchor_boost_weight`
- `thoughtnode_reuse_bias`, `path_merge_tolerance`, `idbs_depth_increment`

**E) Output Assembly (8 parameters)**
- `retain_function_words`, `grammar_fix_enabled`, `capitalization_enabled`
- `punctuation_enabled`, `max_phrase_length`, `duplicate_word_penalty`
- `clause_glue_weight`, `subject_verb_agreement_weight`

**F) Confidence & Abstain (6 parameters)**
- `conf_threshold_definitional`, `conf_threshold_factual`, `conf_threshold_arithmetic`
- `conf_threshold_fallback_penalty`, `topk_agreement_weight`, `abstain_bias`

**G) Drivers / Meta (3 parameters)**
- `leap_bias`, `abstraction_thresh`, `temporal_weight_inc`

### Phenotype Mapping

Each genome value is mapped to usable parameters via transforms:

- **Nonnegative continuous**: `param = e^(gene) * 10^(-k)`
- **Probabilities/thresholds**: `param = 1/(1+e^(-gene))` (sigmoid)
- **Discrete counts**: `param = floor(|gene|) + 1`
- **Binary flags**: `param = (gene > 0 ? 1 : 0)`

## 🔬 Evolution Algorithm

### Self-Adaptive Mutation

Each gene has its own mutation step size (σ):

```
σ_i' = σ_i * exp(τ' * N(0,1) + τ * N_i(0,1))
```

Where:
- `τ' = 1/√(2n)`, `τ = 1/√(2√n)` (n=55)
- `N(0,1)` = shared Gaussian noise
- `N_i(0,1)` = per-gene noise

### Gene Mutation

Mixture of Gaussian and heavy-tail Cauchy:

```
Δg_i ~ {
    Cauchy(0, κ*η*σ_i') with probability ρ=0.1
    N(0, (η*σ_i')²)      with probability 1-ρ
}
```

### 1/5 Success Rule

Global step size control based on success rate:

```
η' = {
    η * 1.2  if success_rate > 0.2
    η / 1.2  if success_rate < 0.2
    η        otherwise
}
```

### Crossover

Linear blend between parents:

```
g_child = β * g_parent1 + (1-β) * g_parent2
β ~ Uniform(0,1)
```

## 📊 Fitness Function

### Multi-Objective Evaluation

**F = 0.6*C + 0.25*S + 0.15*K**

Where:

- **C (Correctness)**: `#correct_answers / #total_tests`
- **S (Speed)**: `1 / (1 + avg_response_time)`
- **K (Creativity)**: `0.5*uniqueness + 0.5*entropy`

### Evaluation Modes

- **Mini Eval**: Fast test suite (every generation)
- **AGI Eval**: Full test suite (every 5 generations)
- **Safety Checks**: Parameter validation and performance constraints

## 📈 Logging and Analysis

### Output Files

- `population.csv` - Complete population data per generation
- `generations.csv` - Generation-level statistics
- `best_genome.json` - Best genome details
- `evolution_summary.txt` - Human-readable summary
- `parameter_trajectories.csv` - Parameter evolution over time

### Key Metrics Tracked

- Fitness evolution (best, average, worst)
- Success rate and global step size (η)
- Population diversity and convergence
- Parameter trajectories and correlations
- Safety check results

## 🔧 Integration with Melvin

### One-Time Optimization

```cpp
#include "evolution/Integration.hpp"

// Run parameter optimization
MELVIN_EVOLVE_PARAMETERS();
```

### Runtime Evolution

```cpp
// Setup continuous optimization
MELVIN_SETUP_RUNTIME_EVOLUTION();

// Apply evolved parameters
Genome best_genome = getBestEvolvedGenome();
MELVIN_APPLY_EVOLVED_PARAMETERS(best_genome);
```

### Custom Evolution

```cpp
MelvinEvolutionIntegration integration;
integration.initializeEvolutionSystem();

// Custom configuration
EvolutionEngine::Config config;
config.population_size = 60;
config.max_generations = 100;

Genome best = integration.evolveParameters();
```

## 🎛️ Configuration

### Evolution Parameters

```cpp
EvolutionEngine::Config config;
config.population_size = 40;        // Population size
config.max_generations = 50;        // Max generations
config.selection_rate = 0.25;       // Top 25% selection
config.elitism_count = 2;           // Elite preservation
config.initial_sigma = 0.15;        // Initial step size
config.global_eta = 1.0;            // Global step multiplier
config.heavy_tail_prob = 0.1;       // Cauchy jump probability
```

### Fitness Evaluation

```cpp
FitnessEvaluator::Config fitness_config;
fitness_config.correctness_weight = 0.6;    // Primary weight
fitness_config.speed_weight = 0.25;         // Speed importance
fitness_config.creativity_weight = 0.15;    // Creativity weight
fitness_config.use_mini_eval = true;        // Fast evaluation
fitness_config.agi_eval_frequency = 5;      // Full eval every 5 gens
```

## 📊 Expected Results

### Performance Improvements

- **Baseline**: 3.1% pass rate, 0.037 average score
- **Target**: 15-25% pass rate, 0.15-0.25 average score
- **Optimized**: 30%+ pass rate, 0.30+ average score

### Key Optimized Parameters

Typical evolved values:
- `beam_width`: 3-8 (vs default 4)
- `max_hops`: 6-12 (vs default 8)
- `conf_threshold_definitional`: 0.02-0.08 (vs default 0.01)
- `edge_decay_rate`: 0.001-0.01 (vs default 0.001)
- `reinforce_step`: 0.05-0.2 (vs default 0.1)

## 🔍 Troubleshooting

### Common Issues

**Evolution Stagnates**
- Increase population diversity
- Adjust mutation rates
- Check for parameter bounds

**Fitness Doesn't Improve**
- Verify test cases are appropriate
- Check safety constraints
- Ensure parameter ranges are valid

**Slow Evaluation**
- Use mini eval mode (`--fast`)
- Reduce population size
- Decrease test case count

### Debug Mode

```bash
make debug
./melvin_evolution --verbose --population 10 --generations 5
```

## 📚 Further Reading

- **Evolution Strategies**: Self-adaptive parameter control
- **Multi-Objective Optimization**: Pareto-optimal solutions
- **Genetic Algorithms**: Selection, crossover, mutation strategies
- **Parameter Tuning**: Hyperparameter optimization techniques

## 🤝 Contributing

The evolution system is designed to be extensible:

- Add new parameter categories in `Genome.hpp`
- Implement custom fitness functions in `Fitness.cpp`
- Add new mutation strategies in `Evolution.cpp`
- Extend logging capabilities in `Logging.hpp`

## 📄 License

Part of the Melvin AGI system. See main project license.

---

**🧬 Ready to evolve Melvin's brain? Start with `make && ./melvin_evolution --fast`!**
