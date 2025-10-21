# 6. Prediction / Expectation Generation

**Pipeline Stage: Generating predictions about what comes next**

This directory contains all components that use the learned graph to predict future states and expected inputs.

## Components

### Prediction Engines
- `gnn_predictor.cpp/h` - Graph neural network predictor
- `hybrid_predictor.cpp/h` - Hybrid prediction system
- `sequence_predictor.cpp/h` - Temporal sequence prediction
- `hopfield_diffusion.cpp/h` - Hopfield-style pattern completion

### Generation & Reasoning
- `generator.cpp/h` - Output generation
- `reasoning.cpp/h` - Logical reasoning over graph
- `melvin_reasoning.h` - Reasoning interfaces
- `pattern_detector.h` - Pattern recognition

## Principles
- **Graph-grounded predictions** - All predictions based on actual learned connections
- **Multi-scale prediction** - Immediate next-token to long-term goals
- **Prediction error** - Drives attention and learning (feedback to Stages 4 & 5)

## Flow
Active Context + Learned Weights → Predictions → Output (Stage 7) & Error Signals (Stage 8)

