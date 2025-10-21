# 2. Tokenization / Node Creation

**Pipeline Stage: Converting raw input into atomic nodes**

This directory contains all components that transform raw sensory data into discrete atomic nodes in Melvin's graph.

## Components

### Core Tokenization
- `tokenizer.cpp/h` - Main tokenization engine
- `binary_ingestor.cpp/h` - Binary data ingestion
- `input_manager.cpp/h` - Input stream management
- `feed_dictionary_verbose.cpp` - Dictionary feeding

### Vision Tokenization
- `vision/visual_tokenization.cpp/h` - Visual feature → node conversion
- `vision/vision_pipeline.cpp/h` - Vision processing pipeline
- `vision/simple_vision_pipeline.h` - Simplified vision pipeline

### Audio Tokenization
- `audio/audio_tokenizer.cpp/h` - Audio → phoneme nodes
- `audio/phoneme_cluster.cpp/h` - Phoneme clustering
- `audio/phoneme_graph.cpp/h` - Phoneme graph construction

## Flow
Raw Input (Stage 1) → Atomic Nodes → Connection Layer (Stage 3)

