# Makefile for Melvin Core System

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

# Core source files
SOURCES = melvin.cpp storage.cpp vm.cpp scoring.cpp learner.cpp beam.cpp nlg.cpp policy.cpp miner.cpp graph_reasoning.cpp LLMReasoningEngine.cpp GraphTokenSampler.cpp predictive_sampler.cpp melvin_leap_nodes.cpp melvin_scheduler.cpp
HEADERS = melvin.h melvin_types.h storage.h vm.h scoring.hpp learner.hpp beam.hpp nlg.hpp policy.hpp miner.hpp graph_reasoning.hpp UCAConfig.h LLMReasoningEngine.h GraphTokenSampler.h text_norm.h question_router.h answerer.h nlg_templates.h predictive_sampler.h melvin_leap_nodes.h melvin_metrics.h melvin_evolution.h melvin_memory.h melvin_scheduler.h melvin_guardrails.h melvin_evaluation.h melvin_homeostat.h melvin_telemetry.h melvin_data_ingestion.h TuningConfig.h uca_types.h

# Scale demo source files
SCALE_DEMO_SOURCES = scale_demo_generators.cpp scale_demo_writer.cpp scale_demo_metrics.cpp scale_demo_multimodal.cpp
SCALE_DEMO_HEADERS = scale_demo_generators.h scale_demo_writer.h scale_demo_metrics.h scale_demo_multimodal.h
SCALE_DEMO_OBJECTS = $(SCALE_DEMO_SOURCES:.cpp=.o)

INCLUDES = -Iinclude -I.

# Object files
OBJECTS = $(SOURCES:.cpp=.o)

# Targets
LIBRARY = libmelvin.so
MELVIN_SCHEDULER = melvin_scheduler
THINKING_DEMO = thinking_brain_demo
EVOLUTION_TEST = test_evolution_loop
EVOLUTION_DEMO = evolution_demo
TRAINING_DEMO = melvin_training_demo
CONTINUOUS_LEARNING = melvin_continuous_learning
STRESS_TEST = melvin_stress_test
SCALE_DEMO = melvin_scale_demo
MULTIMODAL_DEMO = melvin_multimodal_demo
LEARN_TEST_EVOLVE = melvin_learn_test_evolve
OLLAMA_CONTINUOUS = melvin_ollama_continuous
OLLAMA_INTEGRATED = melvin_ollama_integrated
DEMO_GUARDRAILS = demo_guardrails
DUMP_GRAPH = dump_graph
TEST_ENHANCED_LEAP = test_enhanced_leap
FUZZ_TEMPLATES = fuzz_templates
TELEMETRY_ROLLUP = telemetry_rollup
DEMO_CROSS_MODAL = demo_cross_modal
INGEST_TINYCHAT = melvin_ingest_tinychat
TRANSFER_STORAGE = transfer_to_storage
INTERACTIVE = melvin_interactive
DEMO_PREDICTIVE = demo_predictive
DEMO_LEAP_SYSTEM = demo_leap_system
DEMO_EMBEDDING_BRIDGE = demo_embedding_bridge
DIAGNOSTIC_MAIN = diagnostic_main

.PHONY: all clean library scheduler demo evolution_test evolution_demo training_demo continuous_learning stress_test scale_demo multimodal_demo learn_test_evolve ollama_continuous ollama_integrated demo_guardrails dump_graph test_enhanced_leap fuzz_templates telemetry_rollup demo_cross_modal ingest_tinychat transfer_storage interactive demo_predictive demo_leap_system diagnostic

all: library scheduler demo evolution_test evolution_demo training_demo continuous_learning stress_test scale_demo multimodal_demo learn_test_evolve ollama_continuous ollama_integrated test_enhanced_leap dump_graph demo_guardrails fuzz_templates telemetry_rollup demo_cross_modal

library: $(LIBRARY)

scheduler: $(MELVIN_SCHEDULER)

demo: $(THINKING_DEMO)

evolution_test: $(EVOLUTION_TEST)

evolution_demo: $(EVOLUTION_DEMO)

training_demo: $(TRAINING_DEMO)

continuous_learning: $(CONTINUOUS_LEARNING)

$(LIBRARY): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $^

$(MELVIN_SCHEDULER): melvin_scheduler.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDES)

$(THINKING_DEMO): thinking_brain_demo.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDES)

$(EVOLUTION_TEST): test_evolution_loop.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDES)

$(EVOLUTION_DEMO): evolution_demo.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

$(TRAINING_DEMO): melvin_training_demo.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

$(CONTINUOUS_LEARNING): melvin_continuous_learning.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

$(STRESS_TEST): melvin_stress_test.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDES)

stress_test: $(STRESS_TEST)

$(SCALE_DEMO): melvin_scale_demo.cpp $(SCALE_DEMO_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDES) -lssl -lcrypto

scale_demo: $(SCALE_DEMO)

$(MULTIMODAL_DEMO): melvin_multimodal_demo.cpp $(SCALE_DEMO_OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDES) -lssl -lcrypto

multimodal_demo: $(MULTIMODAL_DEMO)

$(LEARN_TEST_EVOLVE): melvin_learn_test_evolve.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDES)

learn_test_evolve: $(LEARN_TEST_EVOLVE)

$(OLLAMA_CONTINUOUS): melvin_ollama_continuous.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES) -lcurl

ollama_continuous: $(OLLAMA_CONTINUOUS)

$(OLLAMA_INTEGRATED): melvin_ollama_integrated.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDES) -lcurl

ollama_integrated: $(OLLAMA_INTEGRATED)

$(TEST_ENHANCED_LEAP): test_enhanced_leap.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

test_enhanced_leap: $(TEST_ENHANCED_LEAP)

$(DEMO_GUARDRAILS): demos/demo_guardrails.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

demo_guardrails: $(DEMO_GUARDRAILS)

$(DUMP_GRAPH): src/tools/dump_graph.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

dump_graph: $(DUMP_GRAPH)

$(FUZZ_TEMPLATES): tools/fuzz_templates.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

fuzz_templates: $(FUZZ_TEMPLATES)

$(TELEMETRY_ROLLUP): tools/telemetry_rollup.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

telemetry_rollup: $(TELEMETRY_ROLLUP)

$(DEMO_CROSS_MODAL): demos/demo_cross_modal.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

demo_cross_modal: $(DEMO_CROSS_MODAL)

$(INGEST_TINYCHAT): melvin_ingest_tinychat.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

ingest_tinychat: $(INGEST_TINYCHAT)

$(TRANSFER_STORAGE): transfer_to_storage.cpp storage.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(INCLUDES)

transfer_storage: $(TRANSFER_STORAGE)

$(INTERACTIVE): melvin_interactive.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

interactive: $(INTERACTIVE)

$(DEMO_PREDICTIVE): demos/demo_predictive.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(INCLUDES)

demo_predictive: $(DEMO_PREDICTIVE)

%.o: %.cpp $(HEADERS) $(SCALE_DEMO_HEADERS)
	$(CXX) $(CXXFLAGS) -c $< -o $@ $(INCLUDES)

clean:
	rm -f $(OBJECTS) $(SCALE_DEMO_OBJECTS) $(LIBRARY) $(MELVIN_SCHEDULER) $(THINKING_DEMO) $(EVOLUTION_TEST) $(EVOLUTION_DEMO) $(TRAINING_DEMO) $(CONTINUOUS_LEARNING) $(STRESS_TEST) $(SCALE_DEMO) $(MULTIMODAL_DEMO) $(LEARN_TEST_EVOLVE) $(OLLAMA_CONTINUOUS) $(OLLAMA_INTEGRATED) $(DEMO_GUARDRAILS) $(DUMP_GRAPH) $(TEST_ENHANCED_LEAP) $(FUZZ_TEMPLATES) $(TELEMETRY_ROLLUP) $(DEMO_CROSS_MODAL) $(DIAGNOSTIC_MAIN) melvin_stress_test_metrics.csv melvin_evolution_log.csv melvin_ollama_metrics.csv melvin_ollama_continuous.bin melvin_ollama_processed_files.txt melvin_integrated_metrics.csv melvin_integrated_processed.txt
	rm -f scale_demo_*.bin scale_demo_*.csv scale_demo_*.jsonl validation_*.txt validation_*.csv multimodal_demo_memory.bin
	rm -f test_guardrails*.bin guardrails_telemetry.jsonl melvin_telemetry.jsonl test_enhanced_leap.bin fuzz_test*.bin fuzz_telemetry.jsonl cross_modal*.bin cross_modal_telemetry.jsonl
	rm -f leap_diagnostics.csv leap_tuning_report.md leap_tuning_results.txt
	rm -f test_learning learning_hooks.o melvin_brain.bin
	rm -f teach_file teach_curriculum src/teaching/*.o
	rm -f bin/melvin_learn_watch melvin_learn_watch src/continuous_learning/*.o
	rm -f logs/continuous_learning_metrics.csv
	rm -rf scale_demo_*_checkpoints chemistry_snapshots_* emergence_log_*.jsonl melvin_lte_data

install: library
	cp $(LIBRARY) /usr/local/lib/
	cp $(HEADERS) /usr/local/include/

.PHONY: help run_scale_demo_A run_scale_demo_B run_scale_demo_C run_scale_demo_comprehensive test_guardrails run_diagnostic run_diagnostic_auto_tune run_diagnostic_quick
help:
	@echo "Available targets:"
	@echo "  all                - Build library, scheduler, demo, evolution test, evolution demo, training demo, continuous learning, scale demo, and LEAP tools"
	@echo "  library            - Build shared library"
	@echo "  scheduler          - Build melvin scheduler"
	@echo "  demo               - Build thinking brain demo"
	@echo "  evolution_test     - Build evolution pipeline test"
	@echo "  evolution_demo     - Build standalone evolution demo"
	@echo "  training_demo      - Build complete LLM training system demo"
	@echo "  continuous_learning - Build continuous learning system with real data"
	@echo "  learn_test_evolve  - Build learn-test-evolve system (feed, test, grade, evolve)"
	@echo "  ollama_continuous  - Build Ollama-powered continuous learning daemon (standalone)"
	@echo "  ollama_integrated  - Build Ollama integrated with Melvin's full system (RECOMMENDED)"
	@echo "  scale_demo         - Build large-scale demo (10k-100k nodes)"
	@echo "  multimodal_demo    - Build multimodal pipeline demo (audio+text unified)"
	@echo "  stress_test        - Build stress test"
	@echo "  test_enhanced_leap - Build enhanced LEAP test (smart reasoning)"
	@echo "  demo_guardrails    - Build LEAP guardrails test suite (6 golden tests)"
	@echo "  dump_graph         - Build graph dump tool (text/JSON output)"
	@echo "  fuzz_templates     - Build template fuzz/regression tester"
	@echo "  telemetry_rollup   - Build telemetry analysis tool"
	@echo "  demo_cross_modal   - Build cross-modal reasoning demo (image+audio+text)"
	@echo "  diagnostic         - Build graph-guided predictive system diagnostic tool"
	@echo "  test_guardrails    - Run guardrails test suite"
	@echo "  clean              - Remove build artifacts"
	@echo "  install            - Install library and headers"
	@echo "  help               - Show this help"
	@echo ""
	@echo "Scale Demo Runners:"
	@echo "  run_scale_demo_A   - Run Phase A (10k nodes, ~5 min)"
	@echo "  run_scale_demo_B   - Run Phase B (50k nodes, ~15-25 min)"
	@echo "  run_scale_demo_C   - Run Phase C (100k nodes, ~30-60 min)"
	@echo "  run_scale_demo_comprehensive - Run all phases A→C with report"
	@echo ""
	@echo "Diagnostic & Tuning Runners:"
	@echo "  run_diagnostic           - Run basic diagnostic suite (10 tests)"
	@echo "  run_diagnostic_auto_tune - Run full auto-tuning with detailed report"
	@echo "  run_diagnostic_quick     - Run quick diagnostic (5 tests)"
	@echo ""
	@echo "Continuous Learning System:"
	@echo "  cl                       - Build continuous learning watcher"
	@echo "  run_cl                   - Run continuous learning mode (watches data/inbox/)"
	@echo "  teach FILE=<path>        - Teach a single .tch file"
	@echo "  teach_dir DIR=<path>     - Teach entire curriculum directory"

# Scale demo convenience targets
run_scale_demo_A: scale_demo
	chmod +x run_scale_A.sh
	./run_scale_A.sh

run_scale_demo_B: scale_demo
	chmod +x run_scale_B.sh
	./run_scale_B.sh

run_scale_demo_C: scale_demo
	chmod +x run_scale_C.sh
	./run_scale_C.sh

run_scale_demo_comprehensive: scale_demo
	chmod +x run_comprehensive_demo.sh
	./run_comprehensive_demo.sh

# Test guardrails suite
test_guardrails: demo_guardrails
	./$(DEMO_GUARDRAILS)

# Leap system demo
$(DEMO_LEAP_SYSTEM): demo_leap_system.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

demo_leap_system: $(DEMO_LEAP_SYSTEM)

run_leap_demo: demo_leap_system
	./$(DEMO_LEAP_SYSTEM)

# Embedding bridge demo
$(DEMO_EMBEDDING_BRIDGE): demo_embedding_bridge.cpp $(OBJECTS)
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

demo_embedding_bridge: $(DEMO_EMBEDDING_BRIDGE)

run_embedding_demo: demo_embedding_bridge
	./$(DEMO_EMBEDDING_BRIDGE)

# Diagnostic and auto-tuning tool
$(DIAGNOSTIC_MAIN): diagnostic_main.cpp leap_diagnostic.cpp leap_auto_tune.cpp melvin_leap_nodes.cpp predictive_sampler.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

diagnostic: $(DIAGNOSTIC_MAIN)

run_diagnostic: diagnostic
	./$(DIAGNOSTIC_MAIN)

run_diagnostic_auto_tune: diagnostic
	./$(DIAGNOSTIC_MAIN) --auto-tune --full-report

run_diagnostic_quick: diagnostic
	./$(DIAGNOSTIC_MAIN) --quick

# Learning system with in-memory updates and binary persistence
LEARNING_OBJS = learning_hooks.o storage.o

learning_hooks.o: learning_hooks.cpp learning_hooks.h storage.h melvin_leap_nodes.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

test_learning: test_learning.cpp $(LEARNING_OBJS) predictive_sampler.o melvin_leap_nodes.o vm.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

run_learning_test: test_learning
	./test_learning

.PHONY: test_learning run_learning_test

# Teaching system with curriculum and verification
TEACHING_OBJS = src/teaching/teaching_format.o src/teaching/teaching_ingest.o src/teaching/teaching_verify.o src/teaching/teaching_metrics.o

src/teaching/teaching_format.o: src/teaching/teaching_format.cpp src/teaching/teaching_format.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

src/teaching/teaching_ingest.o: src/teaching/teaching_ingest.cpp src/teaching/teaching_ingest.h src/teaching/teaching_format.h learning_hooks.h storage.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

src/teaching/teaching_verify.o: src/teaching/teaching_verify.cpp src/teaching/teaching_verify.h src/teaching/teaching_format.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

src/teaching/teaching_metrics.o: src/teaching/teaching_metrics.cpp src/teaching/teaching_metrics.h src/teaching/teaching_format.h
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

teach_file: src/tools/teach_file.cpp $(TEACHING_OBJS) $(LEARNING_OBJS) vm.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

teach_curriculum: src/tools/teach_curriculum.cpp $(TEACHING_OBJS) $(LEARNING_OBJS) vm.o
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o $@ $^

teach: teach_file
	@if [ -z "$(FILE)" ]; then echo "Usage: make teach FILE=lessons/00_basics.tch"; exit 1; fi
	./teach_file --file $(FILE) --verify --snapshot

teach_dir: teach_curriculum
	@if [ -z "$(DIR)" ]; then echo "Usage: make teach_dir DIR=lessons/"; exit 1; fi
	./teach_curriculum --dir $(DIR) --stop-on-fail

teach_verify: teach_file
	@if [ -z "$(FILE)" ]; then echo "Usage: make teach_verify FILE=lessons/00_basics.tch"; exit 1; fi
	./teach_file --file $(FILE) --verify --no-snapshot

teach_reset:
	@echo "⚠️  Resetting Melvin's brain (deleting melvin_brain.bin)..."
	@rm -f melvin_brain.bin
	@echo "✅ Brain reset complete. Melvin will start fresh on next teach."

teach_report:
	@echo "📊 Teaching Session Reports:"
	@ls -lh logs/teaching/ 2>/dev/null || echo "No logs found"

.PHONY: teach_file teach_curriculum teach teach_dir teach_verify teach_reset teach_report

# --- Continuous Learning System ---
BIN_DIR ?= bin
CL_OBJS = src/continuous_learning/continuous_learning.o

src/continuous_learning/continuous_learning.o: src/continuous_learning/continuous_learning.cpp src/continuous_learning/continuous_learning.h
	@mkdir -p src/continuous_learning
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

melvin_learn_watch: src/tools/melvin_learn_watch.cpp $(CL_OBJS) $(TEACHING_OBJS) $(LEARNING_OBJS) storage.o vm.o
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) $(INCLUDES) -o bin/$@ $^

cl: melvin_learn_watch
	@echo "✅ Continuous Learning watcher built: bin/melvin_learn_watch"

run_cl: cl
	@./scripts/run_continuous_learning.sh

.PHONY: cl melvin_learn_watch run_cl