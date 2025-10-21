# ============================================================================
# MELVIN v2 - 100% C++ Vision System
# ============================================================================

CXX := g++
CXXFLAGS := -std=c++17 -O3 -Wall -Wextra -I.. -I.

# OpenCV
OPENCV_CFLAGS := $(shell pkg-config --cflags opencv4 2>/dev/null || pkg-config --cflags opencv)
OPENCV_LIBS := $(shell pkg-config --libs opencv4 2>/dev/null || pkg-config --libs opencv)

# All flags
ALL_CFLAGS := $(CXXFLAGS) $(OPENCV_CFLAGS)
ALL_LIBS := $(OPENCV_LIBS) -lpthread

# Directories
BUILD_DIR := build
BIN_DIR := ../../bin

# Core source files
CORE_SRCS := \
	core/global_workspace.cpp \
	core/neuromodulators.cpp \
	memory/working_memory.cpp \
	memory/semantic_bridge.cpp \
	attention/saliency.cpp \
	attention/topdown.cpp \
	attention/arbitration.cpp \
	evolution/genome.cpp \
	perception/camera_bridge.cpp \
	unified_loop_v2.cpp \
	../core/atomic_graph.cpp

# Demo sources
DEMO_CAMERA_SRC := demos/demo_camera_cpp.cpp

# Object files
CORE_OBJS := $(CORE_SRCS:%.cpp=$(BUILD_DIR)/%.o)
DEMO_CAMERA_OBJ := $(BUILD_DIR)/$(DEMO_CAMERA_SRC:%.cpp=%.o)

# Targets
DEMO_CAMERA := $(BIN_DIR)/demo_camera_v2

# ============================================================================
# BUILD RULES
# ============================================================================

.PHONY: all clean demo camera help

all: $(DEMO_CAMERA)

demo: $(DEMO_CAMERA)

camera: $(DEMO_CAMERA)

# Demo camera program
$(DEMO_CAMERA): $(CORE_OBJS) $(DEMO_CAMERA_OBJ) | $(BIN_DIR)
	@echo "🔗 Linking $@"
	@$(CXX) $(ALL_CFLAGS) $^ $(ALL_LIBS) -o $@
	@echo "✓ Built: $@"
	@echo ""
	@echo "Run: $(DEMO_CAMERA)"
	@echo "     $(DEMO_CAMERA) 1  (use camera 1)"

# Compile core objects
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@echo "📦 Compiling $<"
	@mkdir -p $(dir $@)
	@$(CXX) $(ALL_CFLAGS) -c $< -o $@

# Create directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# ============================================================================
# CLEAN
# ============================================================================

clean:
	@echo "🧹 Cleaning build artifacts..."
	@rm -rf $(BUILD_DIR)
	@rm -f $(DEMO_CAMERA)
	@echo "✓ Clean complete"

# ============================================================================
# HELP
# ============================================================================

help:
	@echo "MELVIN v2 - 100% C++ Vision System"
	@echo ""
	@echo "Targets:"
	@echo "  make         - Build all demos"
	@echo "  make camera  - Build camera demo"
	@echo "  make clean   - Remove build artifacts"
	@echo "  make help    - Show this help"
	@echo ""
	@echo "Run:"
	@echo "  ./$(DEMO_CAMERA)     - Use default camera (0)"
	@echo "  ./$(DEMO_CAMERA) 1   - Use camera 1"
	@echo ""
	@echo "Requirements:"
	@echo "  - OpenCV 4.x (C++ API)"
	@echo "  - C++17 compiler"
	@echo ""
	@echo "This is 100% C++ - NO PYTHON!"

# ============================================================================
# DEPENDENCIES (auto-generated)
# ============================================================================

-include $(CORE_OBJS:.o=.d)
-include $(DEMO_CAMERA_OBJ:.o=.d)

$(BUILD_DIR)/%.d: %.cpp
	@mkdir -p $(dir $@)
	@$(CXX) $(ALL_CFLAGS) -MM -MT '$(BUILD_DIR)/$*.o' $< > $@

.PHONY: all clean demo camera help

