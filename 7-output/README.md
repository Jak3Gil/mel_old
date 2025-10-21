# 7. Output

**Pipeline Stage: Motor actions and language generation**

This directory contains all components that transform internal predictions into external actions.

## Components

### Audio/Speech Output
- `audio/vocal_engine.cpp/h` - Speech synthesis
- `audio/audio_bridge.cpp/h` - Audio output bridge
- `audio/audio_pipeline.cpp/h` - Audio processing pipeline

### Language Generation
- `v2_reasoning/conversation_engine.cpp/h` - Conversation management
- `v2_reasoning/language_generator.cpp/h` - Graph-grounded language generation

### Motor Output
- `melvin_output.h` - General output interfaces

### Action Systems
- `v2_action/` - V2 action selection and execution

## Principles
- **Graph-grounded output** - All outputs trace back to learned nodes
- **Multi-modal** - Speech, movement, gestures
- **Prediction-driven** - Output is the predicted next action

## Flow
Predictions (Stage 6) → Actions → World → Feedback (Stage 8)

