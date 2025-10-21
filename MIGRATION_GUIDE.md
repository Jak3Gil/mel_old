# Migration Guide: 8-Stage Pipeline Reorganization

## What Changed

The entire Melvin codebase has been reorganized from a monolithic `melvin/` directory structure into a clear 8-stage cognitive pipeline that mirrors how information flows through the system.

## New Directory Structure

```
Melvin/
├── 1-input-sensors/        # Raw sensory input
├── 2-tokenization/         # Node creation
├── 3-connection-layer/     # Graph connections
├── 4-context-field/        # Working memory
├── 5-learning-chemistry/   # LEAP learning
├── 6-prediction/          # Prediction generation
├── 7-output/              # Actions & speech
├── 8-feedback-loop/       # Error signals
└── 9-other/               # Infrastructure
```

## File Mapping

### Old Location → New Location

#### Vision System
- `melvin/vision/` → `1-input-sensors/vision/`
- `melvin/core/melvin_vision.*` → `9-other/core/`
- `vision_learning.py` → `1-input-sensors/`

#### Audio System
- `melvin/audio/audio_tokenizer.*` → `2-tokenization/audio/`
- `melvin/audio/vocal_engine.*` → `7-output/audio/`
- `melvin/audio/self_feedback.*` → `8-feedback-loop/audio/`

#### Core Graph
- `melvin/core/atomic_graph.*` → `3-connection-layer/`
- `melvin/core/tokenizer.*` → `2-tokenization/`
- `melvin/core/attention_manager.*` → `4-context-field/`
- `melvin/core/fast_learning.*` → `5-learning-chemistry/`
- `melvin/core/*_predictor.*` → `6-prediction/`

#### V2 Components
- `melvin/v2/core/global_workspace.*` → `4-context-field/v2_core/`
- `melvin/v2/memory/` → `4-context-field/v2_memory/`
- `melvin/v2/attention/` → `4-context-field/v2_attention/`
- `melvin/v2/reasoning/` → `7-output/v2_reasoning/`
- `melvin/v2/evolution/` → `9-other/v2_evolution/`

#### Infrastructure
- `melvin/core/types.h` → `9-other/core/`
- `melvin/core/storage.*` → `9-other/core/`
- `melvin/Makefile` → `9-other/Makefile`
- `melvin/include/` → `9-other/include/`
- `melvin/src/` → `9-other/src/`
- `melvin/tests/` → `9-other/tests/`
- `melvin/data/` → `9-other/data/`
- `bin/` → `9-other/bin/`

## Updating Code

### Include Paths

**Old:**
```cpp
#include "core/atomic_graph.h"
#include "core/tokenizer.h"
#include "core/attention_manager.h"
```

**New:**
```cpp
#include "atomic_graph.h"          // From 3-connection-layer
#include "tokenizer.h"             // From 2-tokenization
#include "attention_manager.h"     // From 4-context-field
```

The new Makefile adds all pipeline directories to the include path, so you can use simple includes.

### Python Imports

**Old:**
```python
from melvin.vision import vision_pipeline
from melvin.io import camera_bridge
```

**New:**
```python
import sys
sys.path.append('../1-input-sensors')
from vision import vision_pipeline
from io import camera_bridge
```

## Building

### Quick Build (Recommended)

```bash
# From project root
make status          # Show pipeline structure
make help           # Show help
cd 9-other && make  # Build legacy v1 system
```

### Legacy Builds

The original `melvin/Makefile` is now at `9-other/Makefile` and works the same way:

```bash
cd 9-other
make              # Build test program
make test         # Run tests
make demo-context # Build context demo
make clean        # Clean build artifacts
```

## Working with the New Structure

### Adding New Code

Determine which pipeline stage your code belongs to:

1. **Sensor input?** → `1-input-sensors/`
2. **Creating nodes?** → `2-tokenization/`
3. **Connecting nodes?** → `3-connection-layer/`
4. **Working memory/attention?** → `4-context-field/`
5. **Learning/weight updates?** → `5-learning-chemistry/`
6. **Prediction?** → `6-prediction/`
7. **Output/actions?** → `7-output/`
8. **Feedback/error signals?** → `8-feedback-loop/`
9. **Infrastructure/utilities?** → `9-other/`

### Understanding Data Flow

Follow the pipeline:
```
Raw Input (1) → Nodes (2) → Graph (3) → Context (4) 
                                           ↓
Feedback (8) ← Output (7) ← Predict (6) ← Learn (5)
```

### Documentation

Each directory has a `README.md` explaining:
- What belongs in that stage
- Key components
- How it connects to other stages

## Executables & Scripts

### Executables
- All in `9-other/bin/`
- No path changes needed

### Python Scripts
- Motor control: `1-input-sensors/*motor*.py`
- Vision learning: `1-input-sensors/vision_learning.py`
- V2 tools: `9-other/v2_tools/`

## Data & Logs

- Graph data: `9-other/data/`
- Logs: `9-other/logs/`
- Build artifacts: `9-other/build/`

## Git & Version Control

The reorganization is tracked in git. To see what moved where:

```bash
git log --follow <filename>
```

## Benefits of New Structure

1. **Clarity**: File location matches its role in cognition
2. **Modularity**: Clear boundaries between stages
3. **Teaching**: Structure teaches how Melvin works
4. **Development**: Easier to find and modify components
5. **Documentation**: Self-documenting organization

## Common Issues

### "Cannot find include file"

**Solution**: Update include paths or add directory to CXXFLAGS

### "Module not found" (Python)

**Solution**: Update sys.path or PYTHONPATH

### "Executable not found"

**Solution**: Check `9-other/bin/` or `9-other/build/`

## Getting Help

1. Read `README.md` in project root
2. Read `README.md` in relevant pipeline directory
3. Check `9-other/v2_docs/` for v2 documentation
4. Review `REORGANIZATION_MAP.md` for detailed file mapping

## Philosophy

This reorganization makes Melvin's architecture explicit. Instead of hiding the cognitive pipeline in directory names like `core/` and `v2/`, the structure now mirrors the actual flow of information through the system.

Each stage is a transformation:
- Stage 1-2: Continuous → Discrete (world → nodes)
- Stage 3: Isolated → Connected (nodes → graph)
- Stage 4: Static → Active (graph → working memory)
- Stage 5: Fixed → Adaptive (connections → learning)
- Stage 6: Reactive → Predictive (memory → expectations)
- Stage 7: Internal → External (predictions → actions)
- Stage 8: Open-loop → Closed-loop (actions → feedback)

This is how intelligence works.

