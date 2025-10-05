# Makefile for Melvin Hardened Evolutionary Brain Simulation

UNAME_S := $(shell uname -s)

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -O2 -fPIC -pthread -DMELVIN_BUILD_DLL -fvisibility=default
LDFLAGS = -shared -pthread

# Platform-specific link flags
ifeq ($(UNAME_S),Linux)
  LDFLAGS += -Wl,--no-undefined
endif
ifeq ($(UNAME_S),Darwin)
  # macOS equivalent: fail on undefined at link of dylib
  LDFLAGS += -Wl,-undefined,error
endif

# Source files
SOURCES = melvin.cpp storage.cpp vm.cpp scoring.cpp learner.cpp beam.cpp nlg.cpp policy.cpp miner.cpp graph_reasoning.cpp LLMReasoningEngine.cpp
HEADERS = melvin.h melvin_types.h storage.h vm.h scoring.hpp learner.hpp beam.hpp nlg.hpp policy.hpp miner.hpp graph_reasoning.hpp UCAConfig.h LLMReasoningEngine.h
INCLUDES = -Iinclude -I.

# UCA Source files
UCA_SOURCES = src/uca/PerceptionEngine.cpp src/uca/ReasoningEngine.cpp src/uca/LearningEngine.cpp src/uca/ReflectionEngine.cpp src/uca/OutputManager.cpp src/uca/FeedbackBus.cpp
UCA_HEADERS = src/uca/uca_types.h src/uca/PerceptionEngine.hpp src/uca/ReasoningEngine.hpp src/uca/LearningEngine.hpp src/uca/ReflectionEngine.hpp src/uca/OutputManager.hpp src/uca/FeedbackBus.hpp
UCA_OBJECTS = $(UCA_SOURCES:.cpp=.o)

# Object files
OBJECTS = $(SOURCES:.cpp=.o)
ALL_OBJECTS = $(OBJECTS) $(UCA_OBJECTS)

# Targets
LIBRARY = libmelvin.so
MELVIN_SCHEDULER = melvin_scheduler
DEMO = evolutionary_brain_demo
META_DEMO = meta_evolution_demo
SIMPLE_META_DEMO = simple_meta_demo
MINIMAL_META_DEMO = minimal_meta_demo
INTEGRATED_META_DEMO = integrated_meta_demo
SIMPLE_INTEGRATED_DEMO = simple_integrated_demo
CONSTANT_MEMORY_DEMO = constant_memory_evolution_demo
BIOCHEM_TEST = biochem_test
ANCHOR_TEST = test_anchors
NEURAL_TEST = neural_melvin_test
ADVANCED_NEURAL_TEST = advanced_neural_test
PROOF_TEST = proof_of_reasoning_test
GRAPH_PROOF_TEST = graph_reasoning_proof
SIMPLE_PROOF_TEST = simple_reasoning_proof
NEURAL_GRAPH_TEST = test_neural_graph_reasoning
GRAPH_REASONING_TEST = test_graph_reasoning
AGI_TEST = agi_style_test
MELVIN_PROOF = melvin_proof
COMPREHENSIVE_TEST = comprehensive_system_test
LLM_UPGRADE_TEST = llm_upgrade_test
LLM_UPGRADE_SUMMARY = llm_upgrade_summary

# Default target
all: $(LIBRARY) $(MELVIN_SCHEDULER) $(UCA_TEST) $(NEURAL_GRAPH_TEST) $(GRAPH_REASONING_TEST) $(AGI_TEST) test_abstraction melvin_inject

# Build the shared library
$(LIBRARY): $(ALL_OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

# Build the Melvin scheduler (UCA main loop)
$(MELVIN_SCHEDULER): melvin_scheduler.cpp $(UCA_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(UCA_OBJECTS) $(INCLUDES)

# Build the UCA formula test harness
UCA_TEST = test_uca_formulas
$(UCA_TEST): test_uca_formulas.cpp $(UCA_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $< $(UCA_OBJECTS) $(INCLUDES)

# Build the demo program (removed - file deleted)
# $(DEMO): evolutionary_brain_demo.cpp $(LIBRARY)
#	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin

# Build the output evolution demo
OUTPUT_EVOLUTION_DEMO = output_evolution_demo
$(OUTPUT_EVOLUTION_DEMO): output_evolution_demo.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin

# Build the meta-evolution demo
$(META_DEMO): meta_evolution_demo.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin

# Build the simple meta-evolution demo
$(SIMPLE_META_DEMO): simple_meta_demo.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin

# Build the minimal meta-evolution demo (standalone)
$(MINIMAL_META_DEMO): minimal_meta_demo.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Build the integrated meta-evolution demo
$(INTEGRATED_META_DEMO): integrated_meta_demo.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin

# Build the simple integrated demo
$(SIMPLE_INTEGRATED_DEMO): simple_integrated_demo.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the constant memory evolution demo
$(CONSTANT_MEMORY_DEMO): constant_memory_evolution_demo.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Build the biochemistry test
$(BIOCHEM_TEST): biochem_test.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the anchor system test
$(ANCHOR_TEST): tests/test_anchors.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the neural network test
$(NEURAL_TEST): neural_melvin_test.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the advanced neural network test
$(ADVANCED_NEURAL_TEST): advanced_neural_test.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the proof of reasoning test
$(PROOF_TEST): proof_of_reasoning_test.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the graph reasoning proof test
$(GRAPH_PROOF_TEST): graph_reasoning_proof.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the simple reasoning proof test
$(SIMPLE_PROOF_TEST): simple_reasoning_proof.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the neural-graph reasoning test
$(NEURAL_GRAPH_TEST): test_neural_graph_reasoning.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the graph reasoning test
$(GRAPH_REASONING_TEST): test_graph_reasoning.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the AGI style test
$(AGI_TEST): agi_style_test.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the comprehensive system test
$(COMPREHENSIVE_TEST): comprehensive_system_test.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the LLM upgrade test
$(LLM_UPGRADE_TEST): llm_upgrade_test.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the LLM upgrade summary
$(LLM_UPGRADE_SUMMARY): llm_upgrade_summary.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

# Build the verification framework
verification_framework: verification_framework.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the LLM smoke test
llm_smoke_test: llm_smoke_test.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the Melvin proof-of-concept
$(MELVIN_PROOF): melvin_proof.cpp
	$(CXX) -std=c++17 -O2 -o $@ $<

# Build the abstraction test
test_abstraction: test_abstraction.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Build the data injector
melvin_inject: melvin_inject.cpp $(LIBRARY)
	$(CXX) $(CXXFLAGS) -o $@ $< -L. -lmelvin $(INCLUDES)

# Compile source files
%.o: %.cpp $(HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDES)

# Compile UCA source files
src/uca/%.o: src/uca/%.cpp $(UCA_HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDES)

# API source files are now integrated into melvin.cpp

# Clean build artifacts (keep brains/, genomes/, metrics/)
clean:
	rm -f $(OBJECTS) $(LIBRARY) $(OUTPUT_EVOLUTION_DEMO) $(META_DEMO) $(SIMPLE_META_DEMO) $(MINIMAL_META_DEMO) $(INTEGRATED_META_DEMO) $(SIMPLE_INTEGRATED_DEMO) $(CONSTANT_MEMORY_DEMO) $(BIOCHEM_TEST) $(ANCHOR_TEST) $(NEURAL_TEST) $(ADVANCED_NEURAL_TEST) $(PROOF_TEST) $(GRAPH_PROOF_TEST) $(SIMPLE_PROOF_TEST) $(NEURAL_GRAPH_TEST) $(AGI_TEST) $(COMPREHENSIVE_TEST) $(MELVIN_PROOF) test_abstraction melvin_inject $(UCA_TEST)

# Deep clean (remove all generated files)
distclean: clean
	rm -rf brains/ genomes/ metrics/ thoughts/ *_simulation/

# Install the library
install: $(LIBRARY)
	sudo cp $(LIBRARY) /usr/local/lib/
	sudo cp melvin.h /usr/local/include/
	sudo ldconfig

# Run the demo with proper file limits (removed - file deleted)
# run-demo: $(DEMO)
#	ulimit -n 4096
#	./$(DEMO)

# Run the output evolution demo
run-output-evolution-demo: $(OUTPUT_EVOLUTION_DEMO)
	./$(OUTPUT_EVOLUTION_DEMO)

# Run the meta-evolution demo
run-meta-demo: $(META_DEMO)
	ulimit -n 4096
	./$(META_DEMO)

# Run the simple meta-evolution demo
run-simple-meta-demo: $(SIMPLE_META_DEMO)
	./$(SIMPLE_META_DEMO)

# Run the minimal meta-evolution demo
run-minimal-meta-demo: $(MINIMAL_META_DEMO)
	./$(MINIMAL_META_DEMO)

# Run the integrated meta-evolution demo
run-integrated-meta-demo: $(INTEGRATED_META_DEMO)
	ulimit -n 4096
	./$(INTEGRATED_META_DEMO)

# Run the simple integrated demo
run-simple-integrated-demo: $(SIMPLE_INTEGRATED_DEMO)
	./$(SIMPLE_INTEGRATED_DEMO)

run-constant-memory-demo: $(CONSTANT_MEMORY_DEMO)
	./$(CONSTANT_MEMORY_DEMO)

# Run the biochemistry test
run-biochem-test: $(BIOCHEM_TEST)
	./$(BIOCHEM_TEST)

# Run the neural network test
run-neural-test: $(NEURAL_TEST)
	./$(NEURAL_TEST)

# Run the advanced neural network test
run-advanced-neural-test: $(ADVANCED_NEURAL_TEST)
	./$(ADVANCED_NEURAL_TEST)

# Run the proof of reasoning test
run-proof-test: $(PROOF_TEST)
	./$(PROOF_TEST)

# Run the graph reasoning proof test
run-graph-proof-test: $(GRAPH_PROOF_TEST)
	./$(GRAPH_PROOF_TEST)

# Run the simple reasoning proof test
run-simple-proof-test: $(SIMPLE_PROOF_TEST)
	./$(SIMPLE_PROOF_TEST)

# Run the neural-graph reasoning test
run-neural-graph-test: $(NEURAL_GRAPH_TEST)
	./$(NEURAL_GRAPH_TEST)

# Run the Melvin proof-of-concept
run-melvin-proof: $(MELVIN_PROOF)
	./$(MELVIN_PROOF)

# Run the abstraction test
run-abstraction-test: test_abstraction
	./test_abstraction

# Run the comprehensive system test
run-comprehensive-test: $(COMPREHENSIVE_TEST)
	./$(COMPREHENSIVE_TEST)

# Run the LLM upgrade test
run-llm-upgrade-test: $(LLM_UPGRADE_TEST)
	./$(LLM_UPGRADE_TEST)

# Run the verification framework
run-verification: verification_framework
	./verification_framework

# Run the LLM smoke test
run-llm-smoke: llm_smoke_test
	./llm_smoke_test

# Run the Melvin scheduler
run-melvin: $(MELVIN_SCHEDULER)
	./$(MELVIN_SCHEDULER)

# Run the Melvin scheduler with custom config
run-melvin-config: $(MELVIN_SCHEDULER)
	./$(MELVIN_SCHEDULER) --config config/ --data data/

# Run UCA formula tests
test-uca: $(UCA_TEST)
	./$(UCA_TEST)

# Run UCA ablation tests
test-uca-ablation: $(UCA_TEST)
	./$(UCA_TEST) --ablation

# Run smoke tests
test-smoke: $(MELVIN_SCHEDULER)
	./run_smoke_tests.sh

# Run full experiment suite
test-experiment: $(MELVIN_SCHEDULER)
	python3 experiment_runner.py

# Run quick experiment
test-quick: $(MELVIN_SCHEDULER)
	python3 experiment_runner.py --quick

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

# Sanity check (first 3 generations) (removed - file deleted)
# sanity-check: $(DEMO)
#	ulimit -n 4096
#	./$(DEMO)

# Full evolution run (100 generations) (removed - file deleted)
# full-evolution: $(DEMO)
#	ulimit -n 4096
#	timeout 3600 ./$(DEMO) || echo "Evolution completed or timed out"

# Help target
help:
	@echo "Available targets:"
	@echo "  all            - Build library and demos (default)"
	@echo "  $(LIBRARY)     - Build shared library"
	@echo "  $(MELVIN_SCHEDULER) - Build Melvin Unified Cognitive Architecture"
	@echo "  $(DEMO)        - Build original demo program (removed)"
	@echo "  $(META_DEMO)   - Build meta-evolution demo"
	@echo "  clean          - Remove build artifacts (keep data)"
	@echo "  distclean      - Remove all generated files"
	@echo "  install        - Install library system-wide"
	@echo "  run-demo       - Build and run the original demo (removed)"
	@echo "  run-meta-demo  - Build and run the meta-evolution demo"
	@echo "  run-simple-meta-demo - Build and run the simple meta-evolution demo"
	@echo "  run-minimal-meta-demo - Build and run the minimal meta-evolution demo"
	@echo "  run-integrated-meta-demo - Build and run the integrated meta-evolution demo"
	@echo "  run-simple-integrated-demo - Build and run the simple integrated demo"
	@echo "  run-constant-memory-demo - Build and run the constant memory evolution demo"
	@echo "  run-biochem-test - Build and run the biochemistry API test"
	@echo "  run-neural-test - Build and run the neural network test"
	@echo "  run-advanced-neural-test - Build and run the advanced neural network test"
	@echo "  run-proof-test - Build and run the proof of reasoning test"
	@echo "  run-graph-proof-test - Build and run the graph reasoning proof test"
	@echo "  run-simple-proof-test - Build and run the simple reasoning proof test"
	@echo "  run-neural-graph-test - Build and run the neural-graph reasoning test"
	@echo "  run-melvin-proof - Build and run the Melvin proof-of-concept"
	@echo "  run-abstraction-test - Build and run the abstraction test"
	@echo "  run-comprehensive-test - Build and run the comprehensive system test"
	@echo "  run-llm-upgrade-test - Build and run the LLM-style upgrade test"
	@echo "  run-verification     - Run the verification framework"
	@echo "  run-llm-smoke       - Run the LLM smoke test"
	@echo "  run-inject      - Build and run the data injector"
	@echo "  run-melvin      - Build and run the Melvin UCA scheduler"
	@echo "  run-melvin-config - Run Melvin with custom config directories"
	@echo "  test-uca        - Run UCA formula tests"
	@echo "  test-uca-ablation - Run UCA ablation tests"
	@echo "  test-smoke      - Run 1-minute smoke tests"
	@echo "  test-experiment - Run full experiment suite"
	@echo "  test-quick      - Run quick 2-minute experiments"
	@echo "  sanity-check   - Run 3-generation sanity check (removed)"
	@echo "  full-evolution - Run full 100-generation evolution (removed)"
	@echo "  test           - Run basic tests"
	@echo "  debug          - Build with debug symbols"
	@echo "  release        - Build with optimizations"
	@echo "  help           - Show this help message"

.PHONY: all clean distclean install run-meta-demo run-simple-meta-demo run-minimal-meta-demo run-integrated-meta-demo run-simple-integrated-demo run-constant-memory-demo run-biochem-test run-neural-test run-advanced-neural-test run-proof-test run-graph-proof-test run-simple-proof-test run-neural-graph-test run-melvin-proof run-abstraction-test run-comprehensive-test run-llm-upgrade-test run-verification run-llm-smoke run-inject run-melvin run-melvin-config test-uca test-uca-ablation test-smoke test-experiment test-quick test debug release help
