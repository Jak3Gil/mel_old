# Melvin Pipeline Reorganization Map

## 8-Stage Pipeline Structure

### 1-input-sensors/
**Purpose: All sensor inputs - vision, audio, motor feedback**
- melvin/vision/ (camera input components)
- melvin/io/ (input/output handlers)
- melvin_sees.py, test_camera.py, visualize_camera.py, visualize_melvin.py
- vision_learning.py
- v2/perception/
- v2/tools/*camera*.py, *vision*.py, *detector*.py
- Motor sensor feedback files

### 2-tokenization/
**Purpose: Converting raw input into atomic nodes**
- melvin/vision/visual_tokenization.cpp/h
- melvin/vision/vision_pipeline.cpp/h
- melvin/audio/audio_tokenizer.cpp/h
- melvin/audio/phoneme_cluster.cpp/h, phoneme_graph.cpp/h
- melvin/core/tokenizer.cpp/h
- melvin/core/binary_ingestor.cpp/h
- melvin/core/input_manager.cpp/h
- feed_dictionary_verbose.cpp
- melvin/demos/test_tokenization.cpp, test_vision_pipeline.cpp

### 3-connection-layer/
**Purpose: Creating exact connections between nodes**
- melvin/core/atomic_graph.cpp/h
- melvin/core/melvin_graph.h
- melvin/interfaces/leap_bridge.cpp/h
- melvin/core/episodic_memory.cpp/h

### 4-context-field/
**Purpose: Working mindspace, attention, global workspace**
- demo_context.cpp, test_multimodal_context.cpp
- melvin/core/attention_manager.cpp/h
- melvin/core/melvin_focus.h
- melvin/core/energy_field.cpp/h
- melvin/v2/core/global_workspace.cpp/h
- melvin/v2/memory/working_memory.cpp/h, semantic_bridge.cpp/h
- melvin/v2/attention/ (all attention files)
- melvin/v2/unified_loop_v2.cpp/h
- Docs: CONTEXT_EXPLAINED.md, ATTENTION_*.md, ANTI_STARING_EXPLAINED.md

### 5-learning-chemistry/
**Purpose: LEAP system, weight updates, learning**
- melvin/core/fast_learning.cpp/h
- melvin/core/learning.cpp/h
- melvin/core/leap_inference.cpp/h
- melvin/core/leap_synthesis.cpp/h
- melvin/core/adaptive_weighting.cpp/h
- melvin/core/adaptive_window.h, adaptive_window_config.h
- melvin/core/autonomous_learner.cpp/h
- melvin/core/dataset_loader.cpp/h
- melvin/demos/test_adaptive_*.cpp
- Docs: EXACT_LEAP_SYSTEM.md, ADAPTIVE_WINDOW_*.md

### 6-prediction/
**Purpose: Generating predictions and expectations**
- melvin/core/gnn_predictor.cpp/h
- melvin/core/hybrid_predictor.cpp/h
- melvin/core/sequence_predictor.cpp/h
- melvin/core/hopfield_diffusion.cpp/h
- melvin/core/generator.cpp/h
- melvin/core/pattern_detector.h
- melvin/core/melvin_reasoning.h
- melvin/core/reasoning.cpp/h

### 7-output/
**Purpose: Motor control, speech, language generation**
- melvin/audio/vocal_engine.cpp/h
- melvin/audio/audio_bridge.cpp/h (output side)
- melvin/audio/audio_pipeline.cpp/h (output side)
- melvin/core/melvin_output.h
- melvin/v2/reasoning/language_generator.cpp/h
- melvin/v2/reasoning/conversation_engine.cpp/h
- melvin/v2/action/
- Motor control output: *.py files for motor control
- Docs: ROBSTRIDE_MOTOR_CONTROL.md, MOTOR_TROUBLESHOOTING.md

### 8-feedback-loop/
**Purpose: Self-feedback, reflection, error signals**
- melvin/audio/self_feedback.cpp/h
- melvin/core/melvin_reflect.h
- melvin/demos/prove_adaptation.cpp

### 9-other/
**Purpose: Infrastructure, utilities, configuration**
- Makefile, build system files
- melvin/core/types.h, melvin/v2/core/types_v2.h
- melvin/core/storage.cpp/h, optimized_storage.cpp/h
- melvin/core/metrics.cpp/h
- melvin/core/melvin.cpp/h, unified_mind.cpp/h (orchestrators)
- melvin/v2/evolution/ (genome system)
- melvin/v2/safety/
- melvin/include/
- melvin/src/ (if orchestration)
- melvin/utils/
- melvin/tests/
- melvin/tools/
- melvin/scripts/
- melvin/data/
- melvin/build/
- melvin/logging/
- bin/ (executables)
- backups/
- logs/, pids/
- scripts/ (top-level)
- tools/ (top-level)
- docs/ (top-level)
- All top-level documentation .md files
- All configuration files
- Python helper scripts not fitting other categories
- UCAConfig.h
- melvin/interfaces/python_bridge.cpp, hf_datasets.cpp
- melvin/v2/demos/ (general demos)
- streamline.sh, pick_camera.sh, see_melvin.sh

