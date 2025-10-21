# 8. Feedback Loop

**Pipeline Stage: Self-monitoring and error correction**

This directory contains all components that compare predictions with reality and generate error signals.

## Components

### Self-Feedback
- `audio/self_feedback.cpp/h` - Audio-based self-monitoring
- `melvin_reflect.h` - Reflection and metacognition

## Principles
- **Prediction error** - Compare expected vs actual
- **Error-driven attention** - Surprises capture focus
- **Meta-learning** - Learning about the learning process

## Flow
Output (Stage 7) → World Response → Error Signal → Back to Input (Stage 1) & Context (Stage 4) & Learning (Stage 5)

