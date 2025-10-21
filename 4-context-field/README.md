# 4. Context Field (Working Mindspace)

**Pipeline Stage: Active working memory and attention**

This directory contains all components that manage the active cognitive workspace where thinking happens.

## Components

### Attention Systems
- `attention_manager.cpp/h` - Attention control
- `melvin_focus.h` - Focus mechanisms
- `v2_attention/` - V2 attention system (saliency, arbitration, top-down)

### Working Memory & Global Workspace
- `v2_core/global_workspace.cpp/h` - Global workspace broadcast
- `v2_memory/working_memory.cpp/h` - Active memory buffer
- `v2_memory/semantic_bridge.cpp/h` - Semantic memory interface

### Energy & Context
- `energy_field.cpp/h` - Activation energy field
- `unified_loop_v2.cpp/h` - V2 cognitive loop orchestrator

### Demos
- `demo_context.cpp`, `test_multimodal_context.cpp` - Context system demos

## Principles
- **Limited capacity** - Only most relevant nodes active
- **Attention-driven** - What gets processed is what gets attended
- **Broadcast architecture** - Information shared across systems

## Flow
Connected Graph (Stage 3) → Active Context → Learning (Stage 5) & Prediction (Stage 6)

