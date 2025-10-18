# ============================================================================
# MELVIN UCA v1 - Unified Cognitive Architecture
# Makefile for brain-inspired AI system
# ============================================================================

CXX = g++
CXXFLAGS = -std=gnu++20 -O2 -Wall -Wextra -pedantic -march=native -I./include
LDFLAGS = -pthread

# Directories
INC_DIR = include
SRC_DIR = src
TEST_DIR = tests
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files (exclude main_unified.cpp for library)
SOURCES = $(filter-out $(SRC_DIR)/main_unified.cpp, $(wildcard $(SRC_DIR)/*.cpp))
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SOURCES))

# Targets
TARGET = $(BUILD_DIR)/test_uca_system
MAIN_TARGET = $(BUILD_DIR)/melvin_uca

.PHONY: all run test clean format help

all: $(TARGET)

# Build test program
$(TARGET): $(OBJECTS) $(TEST_DIR)/test_uca_system.cpp | $(BUILD_DIR)
	@echo "🔧 Building test program..."
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(TEST_DIR)/test_uca_system.cpp -o $@ $(LDFLAGS)
	@echo "✅ Built: $@"

# Build main program (optional)
main: $(OBJECTS) $(SRC_DIR)/main_unified.cpp | $(BUILD_DIR)
	@echo "🔧 Building main program..."
	$(CXX) $(CXXFLAGS) $(OBJECTS) $(SRC_DIR)/main_unified.cpp -o $(MAIN_TARGET) $(LDFLAGS)
	@echo "✅ Built: $(MAIN_TARGET)"

# Compile object files
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	@echo "📦 Compiling $<..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# Create directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	@mkdir -p $(OBJ_DIR)

# Run tests
run: $(TARGET)
	@echo "🧪 Running UCA test suite..."
	@./$(TARGET)

test: run

# Clean build artifacts
clean:
	@echo "🧹 Cleaning..."
	@rm -rf $(BUILD_DIR)
	@echo "✅ Clean complete"

# Format code (if clang-format available)
format:
	@echo "📐 Formatting code..."
	@find $(INC_DIR) $(SRC_DIR) $(TEST_DIR) -name "*.h" -o -name "*.cpp" | xargs clang-format -i 2>/dev/null || echo "clang-format not available"

# Help
help:
	@echo ""
	@echo "╔═══════════════════════════════════════════════════════════════════╗"
	@echo "║  🧠 MELVIN UCA v1 - Build System                                  ║"
	@echo "╚═══════════════════════════════════════════════════════════════════╝"
	@echo ""
	@echo "TARGETS:"
	@echo "  make              Build test program"
	@echo "  make run          Build and run tests"
	@echo "  make test         Same as 'make run'"
	@echo "  make main         Build main program (optional)"
	@echo "  make clean        Remove build artifacts"
	@echo "  make format       Format code with clang-format"
	@echo "  make help         Show this help"
	@echo ""
	@echo "ARCHITECTURE:"
	@echo "  • melvin_types.h      Core data structures"
	@echo "  • melvin_graph.*      AtomicGraph (Hippocampus + Cortex)"
	@echo "  • melvin_vision.*     Vision pipeline (V1→V4→IT)"
	@echo "  • melvin_focus.*      Attention control (FEF/SC)"
	@echo "  • melvin_reasoning.*  PFC reasoning"
	@echo "  • melvin_reflect.*    Predictive coding"
	@echo "  • melvin_output.*     Motor/Speech output"
	@echo "  • unified_mind.*      Complete orchestrator"
	@echo ""
	@echo "COGNITIVE LOOP:"
	@echo "  INPUT → PERCEPTION → ATTENTION → REASONING → REFLECTION → OUTPUT"
	@echo "     ↑                                                        ↓"
	@echo "     └────────────────── FEEDBACK ───────────────────────────┘"
	@echo ""
	@echo "TEST SUITE (6 demos):"
	@echo "  1. Tokenize & Link       - Frame/patch creation, temporal edges"
	@echo "  2. Gestalt Group         - Object formation from patches"
	@echo "  3. Saliency vs Goal      - Attention scoring (α*S + β*G + γ*C)"
	@echo "  4. Reasoning Hop         - Multi-hop graph inference"
	@echo "  5. Predictive Error      - Prediction error → learning → LEAPs"
	@echo "  6. Closed Loop           - 100 complete cognitive cycles"
	@echo ""
	@echo "CONSTANTS:"
	@echo "  α (saliency) = 0.45,  β (goal) = 0.35,  γ (curiosity) = 0.20"
	@echo "  λ (decay) = 0.0025/s,  η (reinforce) = 0.10"
	@echo ""

# Show build info
info:
	@echo "Source files: $(SOURCES)"
	@echo "Object files: $(OBJECTS)"
	@echo "Compiler: $(CXX)"
	@echo "Flags: $(CXXFLAGS)"
