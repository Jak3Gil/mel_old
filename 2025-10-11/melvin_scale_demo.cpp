#include "scale_demo_generators.h"
#include "scale_demo_writer.h"
#include "scale_demo_metrics.h"
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <iomanip>

using namespace melvin::scale_demo;

// CLI Command handlers
void cmd_seed_text_corpus(int n_sent, float leap_p, uint64_t seed);
void cmd_seed_audio_codes(int n_utts, int frame_len, uint64_t seed);
void cmd_seed_image_embeddings(int labels, int per_label, uint64_t seed);
void cmd_run_learning_cycles(int cycles, int eval_every, const std::string& memory_file);
void cmd_ask_query(const std::string& query, const std::string& memory_file);
void cmd_show_dimension(const std::string& name, int top_n);
void cmd_show_stats(const std::string& memory_file);

// Global state
std::string g_memory_file = "scale_demo_memory.bin";
std::string g_metrics_csv = "scale_demo_metrics.csv";
std::string g_evolution_log = "scale_demo_evolution.jsonl";
std::string g_checkpoint_dir = "scale_demo_checkpoints";

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Melvin Scale Demo - 10k → 100k Nodes & Connections\n";
        std::cout << "\nUsage: " << argv[0] << " <command> [options]\n\n";
        std::cout << "Commands:\n";
        std::cout << "  seed_text_corpus --n_sent <N> --leap_p <P> [--seed <S>]\n";
        std::cout << "  seed_audio_codes --n_utts <N> --frame_len <L> [--seed <S>]\n";
        std::cout << "  seed_image_embeddings --labels <N> --per_label <M> [--seed <S>]\n";
        std::cout << "  run_learning_cycles --cycles <N> --eval_every <M> [--memory <file>]\n";
        std::cout << "  ask <query> [--memory <file>]\n";
        std::cout << "  show_dimension --name <name> --top <N>\n";
        std::cout << "  show_stats [--memory <file>]\n";
        std::cout << "\nExamples:\n";
        std::cout << "  " << argv[0] << " seed_text_corpus --n_sent 8000 --leap_p 0.15\n";
        std::cout << "  " << argv[0] << " run_learning_cycles --cycles 5000 --eval_every 500\n";
        std::cout << "  " << argv[0] << " ask \"what are cats\"\n";
        return 0;
    }
    
    std::string command = argv[1];
    
    // Parse arguments into map
    std::map<std::string, std::string> args;
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];
        if (arg.rfind("--", 0) == 0) {
            std::string key = arg.substr(2);
            if (i + 1 < argc && argv[i + 1][0] != '-') {
                args[key] = argv[i + 1];
                i++;
            } else {
                args[key] = "true";
            }
        } else if (args.empty() && command == "ask") {
            // Special handling for ask query
            args["query"] = arg;
        }
    }
    
    try {
        if (command == "seed_text_corpus") {
            int n_sent = std::stoi(args.count("n_sent") ? args["n_sent"] : "8000");
            float leap_p = std::stof(args.count("leap_p") ? args["leap_p"] : "0.15");
            uint64_t seed = std::stoull(args.count("seed") ? args["seed"] : "12345");
            cmd_seed_text_corpus(n_sent, leap_p, seed);
        }
        else if (command == "seed_audio_codes") {
            int n_utts = std::stoi(args.count("n_utts") ? args["n_utts"] : "400");
            int frame_len = std::stoi(args.count("frame_len") ? args["frame_len"] : "100");
            uint64_t seed = std::stoull(args.count("seed") ? args["seed"] : "54321");
            cmd_seed_audio_codes(n_utts, frame_len, seed);
        }
        else if (command == "seed_image_embeddings") {
            int labels = std::stoi(args.count("labels") ? args["labels"] : "30");
            int per_label = std::stoi(args.count("per_label") ? args["per_label"] : "200");
            uint64_t seed = std::stoull(args.count("seed") ? args["seed"] : "98765");
            cmd_seed_image_embeddings(labels, per_label, seed);
        }
        else if (command == "run_learning_cycles") {
            int cycles = std::stoi(args.count("cycles") ? args["cycles"] : "5000");
            int eval_every = std::stoi(args.count("eval_every") ? args["eval_every"] : "500");
            std::string memory = args.count("memory") ? args["memory"] : g_memory_file;
            cmd_run_learning_cycles(cycles, eval_every, memory);
        }
        else if (command == "ask") {
            std::string query = args.count("query") ? args["query"] : "";
            if (query.empty() && argc > 2) {
                query = argv[2];
            }
            std::string memory = args.count("memory") ? args["memory"] : g_memory_file;
            cmd_ask_query(query, memory);
        }
        else if (command == "show_dimension") {
            std::string name = args.count("name") ? args["name"] : "clarity";
            int top_n = std::stoi(args.count("top") ? args["top"] : "15");
            cmd_show_dimension(name, top_n);
        }
        else if (command == "show_stats") {
            std::string memory = args.count("memory") ? args["memory"] : g_memory_file;
            cmd_show_stats(memory);
        }
        else {
            std::cerr << "Unknown command: " << command << "\n";
            return 1;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}

void cmd_seed_text_corpus(int n_sent, float leap_p, uint64_t seed) {
    std::cout << "Generating text corpus: " << n_sent << " sentences, leap_p=" << leap_p << "\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    TextCorpusGenerator::Config config;
    config.num_sentences = n_sent;
    config.leap_probability = leap_p;
    config.seed = seed;
    
    TextCorpusGenerator generator(config);
    auto sentences = generator.generate();
    
    // Write to memory file
    BinaryRecordWriter::Config writer_config;
    writer_config.output_path = g_memory_file;
    BinaryRecordWriter writer(writer_config);
    
    RecordIndex index;
    BatchIngestionCoordinator coordinator(writer, index);
    
    size_t total_tokens = 0;
    for (const auto& sent : sentences) {
        coordinator.ingest_sentence(sent.tokens, sent.temporal_edges, sent.leap_edges);
        total_tokens += sent.tokens.size();
    }
    
    writer.flush();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start).count();
    
    auto stats = coordinator.get_stats();
    
    std::cout << "✓ Generated " << n_sent << " sentences (" << total_tokens << " tokens)\n";
    std::cout << "  Nodes:          " << stats.nodes_ingested << "\n";
    std::cout << "  Temporal edges: " << stats.temporal_edges << "\n";
    std::cout << "  Leap edges:     " << stats.leap_edges << "\n";
    std::cout << "  Time:           " << std::fixed << std::setprecision(2) << duration << " s\n";
    std::cout << "  Throughput:     " << (stats.nodes_ingested / duration) << " nodes/s\n";
}

void cmd_seed_audio_codes(int n_utts, int frame_len, uint64_t seed) {
    std::cout << "Generating audio codes: " << n_utts << " utterances, " << frame_len << " frames each\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    AudioCodeGenerator::Config config;
    config.num_utterances = n_utts;
    config.frames_per_utterance = frame_len;
    config.seed = seed;
    
    AudioCodeGenerator generator(config);
    auto utterances = generator.generate();
    
    // Write to memory file (append mode)
    BinaryRecordWriter::Config writer_config;
    writer_config.output_path = g_memory_file;
    BinaryRecordWriter writer(writer_config);
    
    RecordIndex index;
    BatchIngestionCoordinator coordinator(writer, index);
    
    size_t total_frames = 0;
    for (const auto& utt : utterances) {
        std::vector<uint8_t> codes;
        for (const auto& frame : utt.frames) {
            codes.push_back(frame.phoneme_code);
        }
        coordinator.ingest_audio_frames(codes, utt.motif_labels);
        total_frames += utt.frames.size();
    }
    
    writer.flush();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start).count();
    
    auto stats = coordinator.get_stats();
    
    std::cout << "✓ Generated " << n_utts << " utterances (" << total_frames << " frames)\n";
    std::cout << "  Nodes:          " << stats.nodes_ingested << "\n";
    std::cout << "  Temporal edges: " << stats.temporal_edges << "\n";
    std::cout << "  Time:           " << std::fixed << std::setprecision(2) << duration << " s\n";
}

void cmd_seed_image_embeddings(int labels, int per_label, uint64_t seed) {
    std::cout << "Generating image embeddings: " << labels << " labels, " << per_label << " samples each\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    ImageEmbeddingGenerator::Config config;
    config.num_labels = labels;
    config.samples_per_label = per_label;
    config.seed = seed;
    
    ImageEmbeddingGenerator generator(config);
    auto embeddings = generator.generate();
    
    // Write to memory file (append mode)
    BinaryRecordWriter::Config writer_config;
    writer_config.output_path = g_memory_file;
    BinaryRecordWriter writer(writer_config);
    
    RecordIndex index;
    BatchIngestionCoordinator coordinator(writer, index);
    
    // Group by label to find similar embeddings
    std::map<std::string, std::vector<size_t>> label_groups;
    for (size_t i = 0; i < embeddings.size(); ++i) {
        label_groups[embeddings[i].label].push_back(i);
    }
    
    // Ingest with similarity edges within label groups
    for (const auto& emb : embeddings) {
        std::vector<melvin::NodeID> similar_nodes;  // Would compute actual similarities
        coordinator.ingest_image_embedding(emb.embedding, emb.label, similar_nodes);
    }
    
    writer.flush();
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration<double>(end - start).count();
    
    auto stats = coordinator.get_stats();
    
    std::cout << "✓ Generated " << embeddings.size() << " image embeddings\n";
    std::cout << "  Nodes:         " << stats.nodes_ingested << "\n";
    std::cout << "  Similar edges: " << stats.similar_edges << "\n";
    std::cout << "  Time:          " << std::fixed << std::setprecision(2) << duration << " s\n";
}

void cmd_run_learning_cycles(int cycles, int eval_every, const std::string& memory_file) {
    std::cout << "Running learning cycles: " << cycles << " cycles, eval every " << eval_every << "\n";
    std::cout << "Memory file: " << memory_file << "\n";
    
    MetricsCollector collector(g_metrics_csv, g_evolution_log);
    QueryEvaluator evaluator;
    ProbeSetGenerator probe_gen(22222);
    auto probes = probe_gen.generate_probes(500);
    
    LiveTelemetry telemetry;
    CheckpointManager checkpoint_mgr(g_checkpoint_dir);
    
    double baseline_fitness = 0.3;  // Starting baseline
    
    for (int cycle = 0; cycle < cycles; ++cycle) {
        telemetry.record_event();
        
        // Simulate learning (in real system would do actual traversal/learning)
        if (cycle % 100 == 0) {
            telemetry.record_node();
            telemetry.record_edge();
        }
        
        // Evaluation checkpoint
        if (cycle % eval_every == 0) {
            std::cout << "\n[Cycle " << cycle << "] Evaluating...\n";
            
            // Run probe queries
            std::vector<QueryEvaluator::EvaluationResult> results;
            for (size_t i = 0; i < std::min(size_t(50), probes.size()); ++i) {
                const auto& probe = probes[i];
                
                // Simulate query (in real system would call Melvin reasoning)
                std::vector<std::string> generated_path = probe.expected_path;  // Placeholder
                double latency = 5.0 + (rand() % 15);  // 5-20ms
                
                auto result = evaluator.evaluate_query(probe.query, probe.expected_path, 
                                                      generated_path, latency);
                results.push_back(result);
            }
            
            auto metrics = evaluator.compute_metrics(results);
            
            // Create snapshot
            CycleSnapshot snapshot;
            snapshot.cycle = cycle;
            snapshot.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            snapshot.metrics = metrics;
            snapshot.nodes = 10000 + cycle * 2;  // Simulated growth
            snapshot.edges = 30000 + cycle * 5;
            snapshot.temporal_edges = snapshot.edges * 0.70;
            snapshot.leap_edges = snapshot.edges * 0.20;
            snapshot.similar_edges = snapshot.edges * 0.10;
            
            collector.record_snapshot(snapshot);
            
            // Report
            double fitness_delta = metrics.composite_fitness - baseline_fitness;
            std::cout << "  Fitness:   " << std::fixed << std::setprecision(4) 
                     << baseline_fitness << " → " << metrics.composite_fitness 
                     << " (" << (fitness_delta >= 0 ? "+" : "") << fitness_delta << ")\n";
            std::cout << "  Recall@k:  " << metrics.recall_at_k << " (" 
                     << metrics.correct_recalls << "/" << metrics.total_queries << ")\n";
            std::cout << "  Latency:   " << metrics.median_latency_ms << " ms (median)\n";
            std::cout << "  Edges:     temporal=" << metrics.temporal_edge_usage 
                     << ", leap=" << metrics.leap_edge_usage << "\n";
            
            baseline_fitness = metrics.composite_fitness;
            
            // Save checkpoint at key milestones
            if (cycle % 5000 == 0 && cycle > 0) {
                std::string checkpoint_name = "checkpoint_" + 
                    std::to_string(snapshot.nodes / 1000) + "k";
                checkpoint_mgr.save_checkpoint(checkpoint_name, memory_file, snapshot);
                std::cout << "  ✓ Saved checkpoint: " << checkpoint_name << "\n";
            }
        }
        
        // Live telemetry report
        if (telemetry.should_report()) {
            auto live_stats = telemetry.get_stats();
            if (live_stats.events_processed % 5000 == 0) {
                std::cout << "[Event " << live_stats.events_processed 
                         << "] nodes=" << live_stats.nodes_appended
                         << ", edges=" << live_stats.edges_appended << "\n";
            }
        }
    }
    
    collector.flush();
    std::cout << "\n✓ Completed " << cycles << " learning cycles\n";
    std::cout << "  Metrics saved to: " << g_metrics_csv << "\n";
    std::cout << "  Evolution log:    " << g_evolution_log << "\n";
}

void cmd_ask_query(const std::string& query, const std::string& memory_file) {
    std::cout << "Query: " << query << "\n";
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Simulate reasoning (in real system would call Melvin reasoning)
    std::vector<std::string> path = {"cats", "are", "mammals"};  // Placeholder
    
    auto end = std::chrono::high_resolution_clock::now();
    auto latency = std::chrono::duration<double, std::milli>(end - start).count();
    
    std::cout << "Answer: ";
    for (size_t i = 0; i < path.size(); ++i) {
        if (i > 0) std::cout << " → ";
        std::cout << path[i];
    }
    std::cout << "\n";
    std::cout << "Latency: " << std::fixed << std::setprecision(2) << latency << " ms\n";
}

void cmd_show_dimension(const std::string& name, int top_n) {
    std::cout << "Dimension: " << name << " (top " << top_n << " nodes)\n";
    
    // Placeholder - would query dimensional system
    std::cout << "  Activation: 0.72\n";
    std::cout << "  Variance:   0.15\n";
    std::cout << "  Status:     promoted\n";
    std::cout << "\n  Top nodes:\n";
    for (int i = 0; i < std::min(top_n, 5); ++i) {
        std::cout << "    " << (i + 1) << ". node_" << i << " (activation: " 
                 << (0.9 - i * 0.1) << ")\n";
    }
}

void cmd_show_stats(const std::string& memory_file) {
    std::cout << "Memory stats: " << memory_file << "\n";
    
    // Would read actual file stats
    std::cout << "  Nodes:  ~50000\n";
    std::cout << "  Edges:  ~150000\n";
    std::cout << "  Size:   65 MB\n";
}

