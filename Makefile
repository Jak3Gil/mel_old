# Makefile for Melvin Hardened Evolutionary Brain Simulation

CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -O2 -fPIC -pthread
LDFLAGS = -shared -pthread

# Source files
SOURCES = melvin.cpp storage.cpp vm.cpp
HEADERS = melvin.h melvin_types.h storage.h vm.h

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Targets
LIBRARY = libmelvin.so
DEMO = evolutionary_brain_demo

# Default target
all: $(LIBRARY) $(DEMO)

# Build the shared library
$(LIBRARY): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

# Build the demo program
$(DEMO): evolutionary_brain_demo.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin

# Compile source files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts (keep brains/, genomes/, metrics/)
clean:
	rm -f $(OBJECTS) $(LIBRARY) $(DEMO)

# Deep clean (remove all generated files)
distclean: clean
	rm -rf brains/ genomes/ metrics/ thoughts/ *_simulation/

# Install the library
install: $(LIBRARY)
	sudo cp $(LIBRARY) /usr/local/lib/
	sudo cp melvin.h /usr/local/include/
	sudo ldconfig

# Run the demo with proper file limits
run-demo: $(DEMO)
	ulimit -n 4096
	./$(DEMO)

# Test the basic functionality
test: $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o test_basic tests/test_basic.cpp -L. -lmelvin
	./test_basic

# Development build with debug symbols
debug: CXXFLAGS += -g -DDEBUG
debug: clean all

# Release build with optimizations
release: CXXFLAGS += -O3 -march=native -DNDEBUG
release: clean all
	@echo "Build ID: $(shell date +%Y%m%d_%H%M%S)" > build_id.txt

# Sanity check (first 3 generations)
sanity-check: $(DEMO)
	ulimit -n 4096
	./$(DEMO)

# Full evolution run (100 generations)
full-evolution: $(DEMO)
	ulimit -n 4096
	timeout 3600 ./$(DEMO) || echo "Evolution completed or timed out"

# Help target
help:
	@echo "Available targets:"
	@echo "  all            - Build library and demo (default)"
	@echo "  $(LIBRARY)     - Build shared library"
	@echo "  $(DEMO)        - Build demo program"
	@echo "  clean          - Remove build artifacts (keep data)"
	@echo "  distclean      - Remove all generated files"
	@echo "  install        - Install library system-wide"
	@echo "  run-demo       - Build and run the demo"
	@echo "  sanity-check   - Run 3-generation sanity check"
	@echo "  full-evolution - Run full 100-generation evolution"
	@echo "  test           - Run basic tests"
	@echo "  debug          - Build with debug symbols"
	@echo "  release        - Build with optimizations"
	@echo "  help           - Show this help message"

.PHONY: all clean distclean install run-demo sanity-check full-evolution test debug release help
