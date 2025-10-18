# 🧠 Melvin UCA v1 - Unified Cognitive Architecture

**A complete brain-inspired perception-reasoning-action system**

[![Build Status](https://img.shields.io/badge/build-passing-brightgreen)]()
[![Tests](https://img.shields.io/badge/tests-6%2F6-success)]()
[![C++](https://img.shields.io/badge/C%2B%2B-20-blue)]()
[![License](https://img.shields.io/badge/license-MIT-blue)]()

---

## Quick Start (2 minutes)

```bash
# Build
cd melvin
make

# Run tests
make run

# Expected output:
# ✅ ALL TESTS COMPLETE
# 502 nodes, 1995 edges created
# 20 Hz cognitive cycles
```

---

## What Is This?

Melvin UCA implements the **complete biological cognitive loop**:

```
INPUT → PERCEPTION → ATTENTION → REASONING → REFLECTION → OUTPUT
  ↑                                                          ↓
  └────────────────────── FEEDBACK ─────────────────────────┘
```

### Key Features

✅ **Biological Timing**: ~20 Hz cycles (like the brain)
✅ **Graph Memory**: Hippocampus + cortical networks
✅ **Predictive Coding**: Error-driven learning
✅ **Attention Formula**: F = 0.45·S + 0.35·G + 0.20·C
✅ **Synaptic Plasticity**: Hebbian reinforcement + decay
✅ **LEAP Formation**: Automatic shortcut discovery
✅ **Real-Time**: <1ms per cycle

---

## Architecture

### 7 Core Modules

1. **melvin_graph** - Memory (Hippocampus + Cortex)
   - Compact 24-byte nodes/edges
   - O(1) lookups, fast queries
   - Reinforce, decay, LEAP formation

2. **melvin_vision** - Vision (V1 → V4 → IT)
   - Frame tokenization
   - Gestalt grouping
   - Saliency computation

3. **melvin_focus** - Attention (FEF + SC)
   - Focus selection
   - Inhibition of return
   - Inertia & switching

4. **melvin_reasoning** - PFC
   - Multi-hop graph reasoning
   - Thought generation
   - Active concepts (global workspace)

5. **melvin_reflect** - Predictive Coding
   - Forward predictions
   - Error detection
   - Learning from mistakes

6. **melvin_output** - Motor/Speech
   - Focus application
   - Speech generation
   - Event logging

7. **unified_mind** - Complete Loop
   - 6-stage orchestration
   - 20 Hz timing
   - Feedback routing

---

## Test Suite

**6 comprehensive demos (all passing):**

1. ✅ Tokenize & Link - Frame/patch creation, temporal edges
2. ✅ Gestalt Group - Spatial grouping, object formation
3. ✅ Saliency vs Goal - Attention formula, top-down biasing
4. ✅ Reasoning Hop - Multi-hop graph inference
5. ✅ Predictive Error - Prediction → learning → LEAPs
6. ✅ Closed Loop - 100 complete cognitive cycles

---

## Usage

### Basic Example

```cpp
#include "unified_mind.h"

int main() {
    melvin::UnifiedMind mind;
    
    // Process frame
    melvin::FrameMeta fm;
    fm.w = 128; fm.h = 128;
    
    std::vector<uint8_t> image = get_camera_frame();
    mind.tick(fm, image);
    
    // Query knowledge
    auto& graph = mind.get_graph();
    std::cout << "Nodes: " << graph.node_count() << "\n";
    std::cout << "Edges: " << graph.edge_count() << "\n";
    
    return 0;
}
```

### Run Examples

```bash
# Run test suite
./build/test_uca_system

# Run main application
make main
./build/melvin_uca 100

# Clean and rebuild
make clean && make
```

---

## Documentation

- **README.md** - This file (quick start)
- **docs/UCA_ARCHITECTURE.md** - Complete architecture (biological mappings)
- **docs/UCA_IMPLEMENTATION_SUMMARY.md** - Implementation details

---

## Performance

- **Cycle time**: 0.18 - 0.51 ms
- **Throughput**: 20 Hz sustained
- **Memory**: 24 bytes/node, 24 bytes/edge
- **Scalability**: 502 nodes, 1995 edges (no slowdown)

---

## Key Formulas

### Attention
```
F = α·Saliency + β·Goal + γ·Curiosity
  = 0.45·S + 0.35·G + 0.20·C
```

### Learning
```
Reinforcement: weight += 0.10
Decay: weight *= exp(-0.0025 · dt)
LEAP threshold: error > 0.12
```

---

## Project Structure

```
melvin/
├── include/          ← Headers (8 files)
├── src/              ← Implementations (8 files)
├── tests/            ← Test suite (6 demos)
├── docs/             ← Documentation
├── build/            ← Build artifacts
├── Makefile          ← Build system
└── README.md         ← This file
```

---

## Requirements

- C++20 compiler (g++/clang++)
- Standard library only
- No external dependencies!

---

## Test Results

```
TEST 1: Tokenize & Link                  ✓ PASS
TEST 2: Gestalt Group                    ✓ PASS
TEST 3: Saliency vs Goal                 ✓ PASS
TEST 4: Reasoning Hop                    ✓ PASS
TEST 5: Predictive Error                 ✓ PASS
TEST 6: Closed Loop (100 ticks)          ✓ PASS

Final: 502 nodes, 1995 edges, 20 Hz
```

---

## Biological Mappings

| Brain Region | Melvin Module |
|--------------|---------------|
| V1-V4-IT | melvin_vision |
| FEF + SC | melvin_focus |
| PFC | melvin_reasoning |
| Hippocampus | melvin_graph (episodic) |
| Cortex | melvin_graph (semantic) |
| Cerebellum | melvin_reflect (predictions) |
| ACC | melvin_reflect (errors) |
| M1 + Broca's | melvin_output |

---

## What Makes This Special

### vs Traditional AI
- ❌ Feedforward only
- ✅ **Closed feedback loop**

### vs Other Cognitive Architectures  
- ❌ Abstract symbolic
- ✅ **Biologically grounded**

### vs LLMs
- ❌ Black box
- ✅ **Explainable graph paths**

---

## Next Steps

### Immediate
- Integrate real camera (OpenCV)
- Richer verbalization
- Visualization dashboard

### Short Term
- Audio pipeline
- Motor control (RobStride)
- Transformer integration

### Long Term
- Hierarchical planning
- Multi-agent communication
- Full humanoid embodiment

---

## Contributing

Key areas for contribution:
- Real camera/sensor integration
- Enhanced reasoning patterns
- Visualization tools
- Benchmarking suite

---

## License

MIT License - See LICENSE file

---

## Citation

```bibtex
@software{melvin_uca_2024,
  title={Melvin UCA: Unified Cognitive Architecture},
  author={Your Name},
  year={2024},
  description={Brain-inspired perception-reasoning-action system}
}
```

---

## Contact

Questions? Issues? Suggestions?
- Open an issue on GitHub
- Check the documentation in `docs/`

---

**Built with neuroscience. Powered by graphs. Ready for robots.** 🧠✨

*Melvin UCA v1 - Complete brain-inspired AI in ~1,661 lines of C++20*
