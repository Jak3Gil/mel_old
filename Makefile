# MELVIN Production Build System - Minimal Jetson Deployment
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -pthread -I. $(shell pkg-config --cflags opencv4 2>/dev/null || echo "")

# Platform-specific linking
UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
    LDFLAGS = -pthread $(shell pkg-config --libs opencv4 2>/dev/null || echo "-lopencv_core -lopencv_imgproc -lopencv_videoio") -lasound -lrt -lsocketcan
else
LDFLAGS = -pthread $(shell pkg-config --libs opencv4 2>/dev/null || echo "-lopencv_core -lopencv_imgproc -lopencv_videoio")
endif

# Directories
REASONING_DIR = core/reasoning
COGNITIVE_DIR = core/cognitive
VISION_DIR = core/vision
AUDIO_DIR = core/audio
EVOLUTION_DIR = core/evolution
FIELDS_DIR = core/fields
FEEDBACK_DIR = core/feedback
METACOGNITION_DIR = core/metacognition
ORCHESTRATOR_DIR = core/orchestrator
METRICS_DIR = core/metrics
LANGUAGE_DIR = core/language
COGNITIVE_OS_DIR = cognitive_os
VALIDATOR_DIR = validator
CROSSMODAL_DIR = crossmodal
STORAGE_DIR = storage
BUILD_DIR = build
BIN_DIR = bin

# Source files (only what's needed for production)
REASONING_SOURCES = \
	$(REASONING_DIR)/spreading_activation.cpp \
	$(REASONING_DIR)/predictor.cpp \
	$(REASONING_DIR)/memory_hierarchy.cpp \
	$(REASONING_DIR)/multi_hop_attention.cpp \
	$(REASONING_DIR)/output_generator.cpp \
	$(REASONING_DIR)/consolidation.cpp \
	$(REASONING_DIR)/unified_reasoning_engine.cpp \
	$(REASONING_DIR)/semantic_scorer.cpp \
	$(REASONING_DIR)/answer_synthesizer.cpp \
	$(REASONING_DIR)/intelligent_reasoner.cpp

COGNITIVE_SOURCES = \
	$(COGNITIVE_DIR)/cognitive_engine.cpp \
	$(COGNITIVE_DIR)/turn_taking_controller.cpp \
	$(COGNITIVE_DIR)/emotional_modulator.cpp \
	$(COGNITIVE_DIR)/conversation_goal_stack.cpp

VISION_SOURCES = \
	$(VISION_DIR)/vision_pipeline.cpp

AUDIO_SOURCES = \
	$(AUDIO_DIR)/audio_graph_layer.cpp \
	$(AUDIO_DIR)/vocal_synthesis.cpp

EVOLUTION_SOURCES = \
	$(EVOLUTION_DIR)/genome.cpp \
	$(EVOLUTION_DIR)/dynamic_genome.cpp

FIELDS_SOURCES = \
	$(FIELDS_DIR)/activation_field_unified.cpp \
	$(FIELDS_DIR)/parallel_graph_traversal.cpp

FEEDBACK_SOURCES = \
	$(FEEDBACK_DIR)/three_channel_feedback.cpp

METACOGNITION_SOURCES = \
	$(METACOGNITION_DIR)/reflective_controller.cpp \
	$(METACOGNITION_DIR)/reflection_controller_dynamic.cpp

ORCHESTRATOR_SOURCES = \
	$(ORCHESTRATOR_DIR)/continuous_mind.cpp

METRICS_SOURCES = \
	$(METRICS_DIR)/reasoning_metrics.cpp

LANGUAGE_SOURCES = \
	$(LANGUAGE_DIR)/intent_classifier.cpp

COGNITIVE_OS_SOURCES = \
	$(COGNITIVE_OS_DIR)/cognitive_os.cpp \
	$(COGNITIVE_OS_DIR)/event_bus.cpp \
	$(COGNITIVE_OS_DIR)/field_facade.cpp \
	$(COGNITIVE_OS_DIR)/metrics.cpp

VALIDATOR_SOURCES = \
	$(VALIDATOR_DIR)/validator.cpp

# Core unified intelligence (must be last to ensure all dependencies)
CORE_UNIFIED = \
	core/unified_intelligence.cpp

CROSSMODAL_SOURCES = \
	$(CROSSMODAL_DIR)/cm_space.cpp \
	$(CROSSMODAL_DIR)/cm_index.cpp \
	$(CROSSMODAL_DIR)/cm_binding.cpp \
	$(CROSSMODAL_DIR)/cm_grounder.cpp \
	$(CROSSMODAL_DIR)/cm_io.cpp

STORAGE_SOURCES = \
	$(STORAGE_DIR)/graph_loader.cpp

ALL_SOURCES = $(REASONING_SOURCES) $(COGNITIVE_SOURCES) $(VISION_SOURCES) $(AUDIO_SOURCES) $(EVOLUTION_SOURCES) $(FIELDS_SOURCES) $(FEEDBACK_SOURCES) $(METACOGNITION_SOURCES) $(ORCHESTRATOR_SOURCES) $(METRICS_SOURCES) $(LANGUAGE_SOURCES) $(COGNITIVE_OS_SOURCES) $(VALIDATOR_SOURCES) $(CORE_UNIFIED) $(CROSSMODAL_SOURCES) $(STORAGE_SOURCES)

# Object files
OBJECTS = $(ALL_SOURCES:%.cpp=$(BUILD_DIR)/%.o)

# Production targets only
TARGETS = $(BIN_DIR)/melvin_jetson $(BIN_DIR)/melvin_chat $(BIN_DIR)/test_cognitive_os $(BIN_DIR)/test_validator

.PHONY: all clean directories

all: directories $(TARGETS)

directories:
	@mkdir -p $(BUILD_DIR)/$(REASONING_DIR)
	@mkdir -p $(BUILD_DIR)/$(COGNITIVE_DIR)
	@mkdir -p $(BUILD_DIR)/$(VISION_DIR)
	@mkdir -p $(BUILD_DIR)/$(EVOLUTION_DIR)
	@mkdir -p $(BUILD_DIR)/$(FIELDS_DIR)
	@mkdir -p $(BUILD_DIR)/$(FEEDBACK_DIR)
	@mkdir -p $(BUILD_DIR)/$(METACOGNITION_DIR)
	@mkdir -p $(BUILD_DIR)/$(ORCHESTRATOR_DIR)
	@mkdir -p $(BUILD_DIR)/$(METRICS_DIR)
	@mkdir -p $(BUILD_DIR)/$(LANGUAGE_DIR)
	@mkdir -p $(BUILD_DIR)/$(COGNITIVE_OS_DIR)
	@mkdir -p $(BUILD_DIR)/$(VALIDATOR_DIR)
	@mkdir -p $(BUILD_DIR)/$(CROSSMODAL_DIR)
	@mkdir -p $(BUILD_DIR)/$(STORAGE_DIR)
	@mkdir -p $(BIN_DIR)
	@mkdir -p logs
	@mkdir -p data

# Production main executable
$(BIN_DIR)/melvin_jetson: melvin_jetson.cpp $(OBJECTS)
	@echo "🔨 Linking melvin_jetson (production)..."
	$(CXX) $(CXXFLAGS) $< $(OBJECTS) $(LDFLAGS) -o $@
	@echo "✅ Built: $@ (with USB cameras, audio, CAN bus)"

# ChatGPT-style interactive interface
$(BIN_DIR)/melvin_chat: melvin_chat.cpp $(OBJECTS)
	@echo "🔨 Linking melvin_chat (interactive)..."
	$(CXX) $(CXXFLAGS) $< $(OBJECTS) $(LDFLAGS) -o $@
	@echo "✅ Built: $@ (ChatGPT-style interface)"

# Production tests
$(BIN_DIR)/test_cognitive_os: test_cognitive_os.cpp $(OBJECTS)
	@echo "🔨 Linking test_cognitive_os..."
	$(CXX) $(CXXFLAGS) $< $(OBJECTS) $(LDFLAGS) -o $@
	@echo "✅ Built: $@"

$(BIN_DIR)/test_validator: test_validator.cpp $(OBJECTS)
	@echo "🔨 Linking test_validator..."
	$(CXX) $(CXXFLAGS) $< $(OBJECTS) $(LDFLAGS) -o $@
	@echo "✅ Built: $@"

# Object files
$(BUILD_DIR)/%.o: %.cpp
	@echo "🔧 Compiling $<..."
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	@echo "🧹 Cleaning build artifacts..."
	rm -rf $(BUILD_DIR) $(BIN_DIR)
	@echo "✅ Clean complete"
