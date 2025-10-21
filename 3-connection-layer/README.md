# 3. Exact Connection Layer

**Pipeline Stage: Creating exact connections between nodes**

This directory contains all components that create and manage exact temporal and spatial connections between atomic nodes.

## Components

### Graph Structure
- `atomic_graph.cpp/h` - Core graph data structure
- `melvin_graph.h` - Graph interface definitions
- `episodic_memory.cpp/h` - Temporal sequence storage

### Connection Management
- `leap_bridge.cpp/h` - Bridge to LEAP learning system

## Principles
- **Exact connections only** - No invented relationships
- **Temporal edges** - What followed what in experience
- **Spatial edges** - What co-occurred in perception

## Flow
Atomic Nodes (Stage 2) → Connected Graph → Context Field (Stage 4)

