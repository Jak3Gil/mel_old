# Melvin - Graph-Guided Predictive Reasoning System

AI reasoning system that predicts like a language model but reasons like a knowledge graph.

## Quick Start

```bash
# 1. Ingest TinyChat dataset (builds graph + trains predictor)
python3 ingest_tinychat.py --limit 5000
make ingest_tinychat
./melvin_ingest_tinychat

# 2. Run interactive terminal
make interactive
./melvin_interactive

# 3. Compare prediction modes
make demo_predictive
./demo_predictive "how do i feel better"
```

## Features

- **Graph-Guided Prediction** - Markov fluency + cluster-based semantic grounding
- **6,869 Unique Words** - Learned from 5,000 TinyChat conversations
- **260,942 Connections** - Knowledge graph built from natural language
- **EXACT + LEAP Reasoning** - Template-based inference with crowd support
- **Incremental Learning** - Predictor and graph learn from every interaction
- **Runtime Configurable** - Tune graph bias λ without recompiling
- **Multi-Turn Context** - Cached cluster centroids preserve conversation state

## Configuration

```bash
# Graph-Guided Prediction
LAMBDA_GRAPH_BIAS=0.5     # Graph bias strength (0=fluent, 1=grounded)
REASONING_ONLY=0          # Fallback to old system (no prediction)
LOG_PREDICTIONS=1         # Log raw and biased probabilities

# Crowd support
TH_SUPPORT=3.0            # Min examples for LEAP
TH_DIVERSITY=2.0          # Min distinct subjects
TH_MARGIN=1.0             # Best must beat second

# Performance
CACHE_TOPK=128            # Cache size per connector
CACHE_TTL_S=300           # Cache TTL (seconds)

# Debugging
EXPLAIN=1                 # Enable explain tracing
```

## Examples

### Interactive Chat (Predictive Mode)
```bash
./melvin_interactive
# Ask questions, get fluent graph-guided responses
# Shows clusters, bias influence, live learning
```

### Compare Prediction Modes
```bash
./demo_predictive "what makes people happy"
# Tests 5 λ values: 0.0, 0.3, 0.5, 0.8, reasoning-only
# Shows: fluency vs groundedness trade-off
```

### Tune Graph Bias
```bash
LAMBDA_GRAPH_BIAS=0.3 ./melvin_interactive  # More fluent
LAMBDA_GRAPH_BIAS=0.8 ./melvin_interactive  # More grounded
LOG_PREDICTIONS=1 ./melvin_interactive      # See probability logs
```

### Analysis
```bash
./dump_graph --leaps --top 20
cat guardrails_telemetry.jsonl | ./telemetry_rollup
```

## Architecture

```
melvin_predictive.h     # Predictive brain (extends melvin_core.h)
melvin_core.h           # Main brain interface
melvin_storage.h        # Node/edge storage (6,869 nodes, 260,942 connections)
melvin_reasoning.h      # LEAP logic

src/prediction/         # NEW: Graph-guided prediction
  markov_predictor.h    # Bigram/trigram Markov chain
  semantic_clusters.h   # Cluster formation + centroid caching
  graph_biased_generator.h  # Hybrid generation with energy function
  feedback_loop.h       # Incremental learning

src/util/
  config.h              # Configuration (7 new prediction params)
  telemetry.h           # Event logging
  explain.h             # Tracing

demos/
  demo_predictive.cpp   # Compare λ values
  demo_guardrails.cpp   # Validation suite

tools/
  melvin_interactive    # Interactive terminal
  telemetry_rollup      # Analytics
```

## Build Targets

```bash
make ingest_tinychat    # Ingest TinyChat + train predictor
make interactive        # Interactive chat terminal
make demo_predictive    # Compare prediction modes
make demo_guardrails    # Golden test suite
make dump_graph         # Graph visualization
make telemetry_rollup   # Telemetry analyzer
make help               # All targets
```

## Testing

```bash
# Golden tests (6 tests)
./demo_guardrails

# Fuzz testing (500 iterations)
TH_SUPPORT=4 ./fuzz_templates --iters 500

# With custom thresholds
EXPLAIN=1 TH_MARGIN=1.5 ./demo_guardrails
```

## License

MIT
