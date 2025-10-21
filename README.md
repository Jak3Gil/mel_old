# Melvin - Brain-Inspired AGI

## Architecture: 8-Stage Cognitive Pipeline

Melvin is organized around the natural flow of information through a cognitive system:

```
[ 1. Input / Sensors ]
        ↓
[ 2. Tokenization / Node Creation ]
        ↓
[ 3. Exact Connection Layer ]
        ↓
[ 4. Context Field (Working Mindspace) ]
        ↓
[ 5. Learning Chemistry (LEAP + Weight Updates) ]
        ↓
[ 6. Prediction / Expectation Generation ]
        ↓
[ 7. Output ]
        ↓
[ 8. Feedback Loop → Input ]
```

## Directory Structure

Each directory represents a stage in Melvin's cognitive pipeline:

- **1-input-sensors/** - Raw sensory input (vision, audio, proprioception)
- **2-tokenization/** - Converting raw data into atomic nodes
- **3-connection-layer/** - Creating exact temporal/spatial connections
- **4-context-field/** - Active working memory and attention
- **5-learning-chemistry/** - LEAP learning and weight updates
- **6-prediction/** - Generating expectations about future states
- **7-output/** - Motor actions and language generation
- **8-feedback-loop/** - Error signals and self-monitoring
- **9-other/** - Build system, utilities, tests, documentation

Each directory contains a README.md explaining its role in detail.

## Core Principles

### 1. Graph-Grounded Everything
- All knowledge stored as exact connections between atomic nodes
- No hallucination - can only express what was learned
- Transparent reasoning - every output traces to specific nodes

### 2. Exact Connections Only
- No invented relationships
- Temporal edges: what followed what in experience
- Spatial edges: what co-occurred in perception

### 3. LEAP Learning
- Local Exponential Activation Potentiation
- Hebbian reinforcement with adaptive windows
- Experience-driven weight strengthening

### 4. Prediction-Driven Cognition
- Prediction error drives attention
- Expectation vs reality generates learning signals
- Continuous prediction-correction loop

## Quick Start

```bash
# Build the system
cd 9-other
make

# Run tests
make test

# Run context demo
make demo-context
```

## Key Components

- **AtomicGraph**: Core graph data structure (3-connection-layer)
- **Global Workspace**: Working memory broadcast (4-context-field)
- **LEAP System**: Learning chemistry (5-learning-chemistry)
- **Conversation Engine**: Graph-grounded dialogue (7-output)

## Version History

- **v1**: Initial atomic graph implementation
- **v2**: Global workspace, neuromodulators, genome system
- **v3**: Conversation system with semantic bridge (in progress)

## Documentation

See individual README.md files in each directory for detailed documentation.

For v2 architecture details: `9-other/v2_docs/`

## Building

All build configuration is in `9-other/Makefile`

## Data Storage

Graph data stored in `9-other/data/`

## Philosophy

Melvin implements a naturalistic approach to intelligence:
- Bottom-up learning from raw experience
- No pre-programmed knowledge
- Emergent understanding through pattern formation
- Biologically-inspired learning rules

The 8-stage pipeline mirrors the information flow in biological brains, from sensory input through cognitive processing to motor output, with continuous feedback loops.

