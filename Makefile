# ============================================================================
# MELVIN - 8-Stage Cognitive Pipeline
# Makefile for brain-inspired AI system
# ============================================================================

CXX = g++
CXXFLAGS = -std=gnu++20 -O2 -Wall -Wextra -pedantic -march=native
LDFLAGS = -pthread

# Pipeline directories
INPUT_DIR = 1-input-sensors
TOKEN_DIR = 2-tokenization
CONNECT_DIR = 3-connection-layer
CONTEXT_DIR = 4-context-field
LEARN_DIR = 5-learning-chemistry
PREDICT_DIR = 6-prediction
OUTPUT_DIR = 7-output
FEEDBACK_DIR = 8-feedback-loop
OTHER_DIR = 9-other

# Include paths for all pipeline stages
INCLUDES = -I$(OTHER_DIR)/include \
           -I$(INPUT_DIR) \
           -I$(TOKEN_DIR) \
           -I$(CONNECT_DIR) \
           -I$(CONTEXT_DIR) \
           -I$(LEARN_DIR) \
           -I$(PREDICT_DIR) \
           -I$(OUTPUT_DIR) \
           -I$(FEEDBACK_DIR) \
           -I$(OTHER_DIR)/core

CXXFLAGS += $(INCLUDES)

# Build directory
BUILD_DIR = $(OTHER_DIR)/build
OBJ_DIR = $(BUILD_DIR)/obj

# Collect source files from pipeline directories
CORE_SOURCES = $(wildcard $(OTHER_DIR)/core/*.cpp)
TOKEN_SOURCES = $(wildcard $(TOKEN_DIR)/*.cpp)
CONNECT_SOURCES = $(wildcard $(CONNECT_DIR)/*.cpp)
CONTEXT_SOURCES = $(wildcard $(CONTEXT_DIR)/*.cpp) \
                  $(wildcard $(CONTEXT_DIR)/v2_core/*.cpp) \
                  $(wildcard $(CONTEXT_DIR)/v2_memory/*.cpp) \
                  $(wildcard $(CONTEXT_DIR)/v2_attention/*.cpp)
LEARN_SOURCES = $(wildcard $(LEARN_DIR)/*.cpp)
PREDICT_SOURCES = $(wildcard $(PREDICT_DIR)/*.cpp)
OUTPUT_SOURCES = $(wildcard $(OUTPUT_DIR)/*.cpp) \
                 $(wildcard $(OUTPUT_DIR)/v2_reasoning/*.cpp)
FEEDBACK_SOURCES = $(wildcard $(FEEDBACK_DIR)/*.cpp)

ALL_SOURCES = $(CORE_SOURCES) $(TOKEN_SOURCES) $(CONNECT_SOURCES) \
              $(CONTEXT_SOURCES) $(LEARN_SOURCES) $(PREDICT_SOURCES) \
              $(OUTPUT_SOURCES) $(FEEDBACK_SOURCES)

# Generate object file paths
OBJECTS = $(patsubst %.cpp,$(OBJ_DIR)/%.o,$(notdir $(ALL_SOURCES)))

.PHONY: all clean help info status

all: status
	@echo ""
	@echo "✅ Melvin reorganized into 8-stage pipeline"
	@echo "   See README.md for details"
	@echo ""
	@echo "   To build legacy systems: cd 9-other && make"

# Show reorganization status
status:
	@echo "╔═══════════════════════════════════════════════════════════════════╗"
	@echo "║  🧠 MELVIN - 8-Stage Cognitive Pipeline                           ║"
	@echo "╚═══════════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "PIPELINE STAGES:"
	@echo "  1. Input/Sensors          → $(INPUT_DIR)/"
	@echo "  2. Tokenization           → $(TOKEN_DIR)/"
	@echo "  3. Connection Layer       → $(CONNECT_DIR)/"
	@echo "  4. Context Field          → $(CONTEXT_DIR)/"
	@echo "  5. Learning Chemistry     → $(LEARN_DIR)/"
	@echo "  6. Prediction             → $(PREDICT_DIR)/"
	@echo "  7. Output                 → $(OUTPUT_DIR)/"
	@echo "  8. Feedback Loop          → $(FEEDBACK_DIR)/"
	@echo "  9. Infrastructure         → $(OTHER_DIR)/"
	@echo ""
	@echo "BUILD COMMANDS:"
	@echo "  cd 9-other && make        Build legacy v1 system"
	@echo "  cd 9-other && make test   Run test suite"
	@echo "  make help                 Show this help"
	@echo ""

# Build legacy v1 system (in 9-other)
legacy:
	@cd $(OTHER_DIR) && $(MAKE)

# Run legacy tests
test:
	@cd $(OTHER_DIR) && $(MAKE) test

# Clean all build artifacts
clean:
	@echo "🧹 Cleaning build artifacts..."
	@rm -rf $(OTHER_DIR)/build
	@echo "✅ Clean complete"

# Help
help: status
	@echo "DIRECTORY STRUCTURE:"
	@echo "  Each directory represents a stage in the cognitive pipeline."
	@echo "  See README.md in each directory for details."
	@echo ""
	@echo "COGNITIVE FLOW:"
	@echo "  INPUT → TOKENIZATION → CONNECTIONS → CONTEXT → LEARNING"
	@echo "                                           ↓"
	@echo "  FEEDBACK ← OUTPUT ← PREDICTION ← ───────┘"
	@echo ""
	@echo "DOCUMENTATION:"
	@echo "  README.md            - Main architecture overview"
	@echo "  */README.md          - Per-stage documentation"
	@echo "  9-other/v2_docs/     - Version 2 documentation"
	@echo ""

# Show file counts
info:
	@echo "Pipeline File Counts:"
	@echo "  Input/Sensors:    $$(find $(INPUT_DIR) -name '*.cpp' -o -name '*.h' | wc -l) files"
	@echo "  Tokenization:     $$(find $(TOKEN_DIR) -name '*.cpp' -o -name '*.h' | wc -l) files"
	@echo "  Connection Layer: $$(find $(CONNECT_DIR) -name '*.cpp' -o -name '*.h' | wc -l) files"
	@echo "  Context Field:    $$(find $(CONTEXT_DIR) -name '*.cpp' -o -name '*.h' | wc -l) files"
	@echo "  Learning:         $$(find $(LEARN_DIR) -name '*.cpp' -o -name '*.h' | wc -l) files"
	@echo "  Prediction:       $$(find $(PREDICT_DIR) -name '*.cpp' -o -name '*.h' | wc -l) files"
	@echo "  Output:           $$(find $(OUTPUT_DIR) -name '*.cpp' -o -name '*.h' | wc -l) files"
	@echo "  Feedback:         $$(find $(FEEDBACK_DIR) -name '*.cpp' -o -name '*.h' | wc -l) files"
	@echo "  Infrastructure:   $$(find $(OTHER_DIR) -name '*.cpp' -o -name '*.h' | wc -l) files"

