# Melvin Evolution System Makefile

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -fPIC -pthread
INCLUDES = -I.. -I.
LIBS = -L.. -lmelvin -pthread

# Source files
EVOLUTION_SOURCES = Genome.cpp Evolution.cpp Fitness.cpp PersistentMemory.cpp main.cpp
EVOLUTION_OBJECTS = $(EVOLUTION_SOURCES:.cpp=.o)
EVOLUTION_TARGET = melvin_evolution

# Logging sources (if implemented)
LOGGING_SOURCES = Logging.cpp
LOGGING_OBJECTS = $(LOGGING_SOURCES:.cpp=.o)

# Default target
all: $(EVOLUTION_TARGET)

# Main evolution executable
$(EVOLUTION_TARGET): $(EVOLUTION_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)
	@echo "✅ Evolution system built successfully!"

# Object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Clean
clean:
	rm -f *.o $(EVOLUTION_TARGET)
	@echo "🧹 Cleaned evolution build files"

# Install (copy to parent directory)
install: $(EVOLUTION_TARGET)
	cp $(EVOLUTION_TARGET) ../
	@echo "📦 Evolution system installed to parent directory"

# Run evolution with default parameters
run: $(EVOLUTION_TARGET)
	./$(EVOLUTION_TARGET)

# Run with custom parameters
run-fast: $(EVOLUTION_TARGET)
	./$(EVOLUTION_TARGET) --fast --population 20 --generations 20

run-verbose: $(EVOLUTION_TARGET)
	./$(EVOLUTION_TARGET) --verbose --population 40 --generations 50

# Test individual components
test-genome: $(EVOLUTION_TARGET)
	@echo "🧪 Testing genome functionality..."
	./$(EVOLUTION_TARGET) --population 5 --generations 2

# Development targets
debug: CXXFLAGS += -g -DDEBUG
debug: $(EVOLUTION_TARGET)

# Profile build
profile: CXXFLAGS += -pg
profile: $(EVOLUTION_TARGET)

# Release build with optimizations
release: CXXFLAGS += -O3 -DNDEBUG
release: $(EVOLUTION_TARGET)

# Documentation (if doxygen is available)
docs:
	@if command -v doxygen >/dev/null 2>&1; then \
		echo "📚 Generating documentation..."; \
		doxygen Doxyfile 2>/dev/null || echo "⚠️  Doxygen config not found"; \
	else \
		echo "⚠️  Doxygen not found, skipping documentation"; \
	fi

# Help
help:
	@echo "🧬 Melvin Evolution System Build Targets:"
	@echo ""
	@echo "Build targets:"
	@echo "  all       - Build evolution system (default)"
	@echo "  clean     - Remove build files"
	@echo "  install   - Install to parent directory"
	@echo ""
	@echo "Run targets:"
	@echo "  run       - Run with default parameters"
	@echo "  run-fast  - Run with fast parameters (20 pop, 20 gen)"
	@echo "  run-verbose - Run with verbose output"
	@echo ""
	@echo "Development targets:"
	@echo "  debug     - Build with debug symbols"
	@echo "  profile   - Build with profiling support"
	@echo "  release   - Build optimized release version"
	@echo "  test-genome - Quick test run"
	@echo ""
	@echo "Utility targets:"
	@echo "  docs      - Generate documentation (requires doxygen)"
	@echo "  help      - Show this help"
	@echo ""
	@echo "Command line options:"
	@echo "  --population SIZE    Population size (default: 40)"
	@echo "  --generations NUM    Max generations (default: 50)"
	@echo "  --fast              Use mini eval only"
	@echo "  --verbose           Verbose output"
	@echo "  --seed NUM          Random seed"
	@echo "  --help              Show help"

# Dependencies (simplified)
Genome.o: Genome.hpp
Evolution.o: Evolution.hpp Genome.hpp Fitness.hpp
Fitness.o: Fitness.hpp Genome.hpp
main.o: Genome.hpp Evolution.hpp Fitness.hpp Logging.hpp

# Phony targets
.PHONY: all clean install run run-fast run-verbose test-genome debug profile release docs help

# Default target
.DEFAULT_GOAL := all
