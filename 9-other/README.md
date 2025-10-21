# 9. Other

**Infrastructure, utilities, configuration, and orchestration**

This directory contains all support systems that don't fit into the main 8-stage pipeline.

## Components

### Core Infrastructure
- `core/` - Core types, storage, metrics, orchestrators
  - `types.h` - Type definitions
  - `storage.cpp/h`, `optimized_storage.cpp/h` - Persistent storage
  - `metrics.cpp/h` - Performance metrics
  - `melvin.cpp/h`, `unified_mind.cpp/h` - System orchestrators

### V2 Systems
- `v2_core/` - V2 types, neuromodulators
- `v2_evolution/` - Genome and evolution systems
- `v2_safety/` - Safety mechanisms
- `v2_demos/` - V2 demonstration programs
- `v2_tools/` - V2 utility scripts
- `v2_docs/` - V2 documentation

### Build System
- `Makefile` - Build configuration
- `build/` - Build artifacts
- `include/`, `src/` - Additional source files

### Tools & Utilities
- `bin/` - Executable binaries
- `tools/` - Development tools
- `scripts/` - Automation scripts
- `utils/` - Utility functions
- `interfaces/` - External interfaces (Python, HuggingFace)

### Testing & Development
- `tests/` - Test suites
- `demos/` - Demo programs
- `examples/` - Example code

### Data & Logs
- `data/` - Stored graph data
- `logs/`, `pids/` - Runtime logs and process IDs
- `backups/` - Backup data

### Documentation
- `docs/` - Architecture documentation
- `*.md` - Various documentation files
- `*.txt` - Text documentation

### Configuration
- `UCAConfig.h` - System configuration
- Various config files

## Note
This directory is intentionally broad. It contains everything needed to build, run, test, and maintain Melvin, but which doesn't directly participate in the cognitive pipeline.
