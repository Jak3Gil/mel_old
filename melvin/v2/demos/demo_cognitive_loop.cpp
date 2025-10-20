/*
 * MELVIN v2 - Complete Cognitive Loop Demo
 * 
 * Demonstrates the full perception→attention→reasoning→action cycle
 * with all genome-driven components working together:
 * 
 * - Genome (49 genes, 7 modules)
 * - Global Workspace (consciousness)
 * - Neuromodulators (DA/NE/ACh/5-HT)
 * - Working Memory (7±2 slots)
 * - Semantic Memory (v1 AtomicGraph bridge)
 * - Attention (saliency + topdown + arbitration)
 * - Unified Loop (complete cycle)
 */

#include "../unified_loop_v2.h"
#include "../evolution/genome.h"
#include "../../core/atomic_graph.h"
#include <iostream>
#include <iomanip>

using namespace melvin::v2;
using namespace melvin::v2::evolution;

void print_section(const std::string& title) {
    std::cout << "\n" << std::string(70, '=') << "\n";
    std::cout << title << "\n";
    std::cout << std::string(70, '=') << "\n";
}

void print_cycle_result(const UnifiedLoopV2::CycleResult& result) {
    std::cout << "\n[Cycle " << result.cycle_number << "]" << std::endl;
    std::cout << "  Focused: object_" << result.focused_object << std::endl;
    std::cout << "  Thoughts: " << result.active_thoughts.size() << " active" << std::endl;
    std::cout << "  WM Slots: " << result.wm_slots.size() << " occupied" << std::endl;
    std::cout << "  Surprise: " << std::fixed << std::setprecision(3) 
              << result.total_surprise << std::endl;
    std::cout << "  Cycle time: " << (result.cycle_time_ns / 1e6) << " ms" << std::endl;
    std::cout << "  Neuromod: " 
              << neuromod_state_to_string(result.neuromod_state) << std::endl;
}

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════╗\n";
    std::cout << "║  MELVIN v2 - Complete Cognitive Loop Demo               ║\n";
    std::cout << "║  Evolution-First Architecture                           ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════╝\n";
    
    try {
        // ====================================================================
        // SETUP
        // ====================================================================
        
        print_section("1. Creating Genome");
        
        Genome genome = GenomeFactory::create_base();
        std::cout << genome_to_string(genome) << std::endl;
        
        std::cout << "\nKey Parameters (from genome):\n";
        std::cout << "  Attention: α=" << genome.get_alpha_saliency() 
                  << ", β=" << genome.get_beta_goal()
                  << ", γ=" << genome.get_gamma_curiosity() << "\n";
        std::cout << "  Working Memory: " << genome.get_wm_slot_count() << " slots\n";
        std::cout << "  Neuromod: DA gain=" << genome.get_da_gain() << "\n";
        
        // ====================================================================
        // CREATE SEMANTIC MEMORY
        // ====================================================================
        
        print_section("2. Setting Up Semantic Memory (v1 Bridge)");
        
        melvin::AtomicGraph* v1_graph = new melvin::AtomicGraph();
        
        // Try to load existing knowledge base
        try {
            v1_graph->load("melvin/data/nodes.melvin", "melvin/data/edges.melvin");
            std::cout << "✓ Loaded existing knowledge base: "
                      << v1_graph->node_count() << " nodes, "
                      << v1_graph->edge_count() << " edges" << std::endl;
        } catch (...) {
            std::cout << "✓ Starting with fresh knowledge base" << std::endl;
            
            // Add some basic concepts for demo
            auto fire = v1_graph->get_or_create_node("fire", 0);
            auto heat = v1_graph->get_or_create_node("heat", 0);
            auto light = v1_graph->get_or_create_node("light", 0);
            
            v1_graph->add_edge(fire, heat, 1, 0.9f);  // fire produces heat
            v1_graph->add_edge(fire, light, 1, 0.8f); // fire produces light
            
            std::cout << "✓ Created basic knowledge: 3 nodes, 2 edges" << std::endl;
        }
        
        // ====================================================================
        // CREATE UNIFIED BRAIN
        // ====================================================================
        
        print_section("3. Building Unified Brain from Genome");
        
        UnifiedLoopV2::Config loop_config;
        loop_config.target_cycle_hz = 20.0f;
        loop_config.enable_prediction = true;
        loop_config.enable_telemetry = true;
        loop_config.verbose = true;
        
        UnifiedLoopV2 brain(genome, 12345, loop_config);
        brain.set_semantic_memory(v1_graph);
        brain.set_goal("Explore environment", 1.0f);
        
        std::cout << "✓ Unified brain created" << std::endl;
        std::cout << "  • Global Workspace" << std::endl;
        std::cout << "  • Neuromodulators (DA/NE/ACh/5-HT)" << std::endl;
        std::cout << "  • Working Memory (7 slots)" << std::endl;
        std::cout << "  • Attention System (saliency + topdown + arbitration)" << std::endl;
        std::cout << "  • Semantic Bridge (v1 graph: " << v1_graph->edge_count() << " edges)" << std::endl;
        
        // ====================================================================
        // RUN COGNITIVE CYCLES
        // ====================================================================
        
        print_section("4. Running Cognitive Cycles");
        
        std::cout << "\nRunning 10 cognitive cycles..." << std::endl;
        
        // Mock image data (not used yet, perception is mocked)
        std::vector<uint8_t> mock_image(640 * 480 * 3, 128);
        
        for (int i = 0; i < 10; ++i) {
            auto result = brain.tick(mock_image.data(), 640, 480);
            
            if (i < 3 || i >= 7) {  // Show first 3 and last 3
                print_cycle_result(result);
            } else if (i == 3) {
                std::cout << "\n  ... (cycles 4-7 running) ...\n" << std::endl;
            }
        }
        
        // ====================================================================
        // INTROSPECTION
        // ====================================================================
        
        print_section("5. Introspecting Brain State");
        
        // Global Workspace
        auto conscious = brain.get_conscious_content();
        std::cout << "\nGlobal Workspace (Conscious Content):\n";
        std::cout << "  Goal: \"" << conscious.current_goal 
                  << "\" (priority=" << conscious.goal_priority << ")\n";
        std::cout << "  Thoughts: " << conscious.thoughts.size() << " active\n";
        for (size_t i = 0; i < std::min(size_t(3), conscious.thoughts.size()); ++i) {
            std::cout << "    • " << thought_to_string(conscious.thoughts[i]) << "\n";
        }
        
        // Neuromodulators
        auto neuromod = brain.get_neuromod_state();
        std::cout << "\nNeuromodulators:\n";
        std::cout << "  " << neuromod_state_to_string(neuromod) << "\n";
        std::cout << "  Plasticity rate: " << (0.1f + neuromod.dopamine * 1.9f) << "\n";
        std::cout << "  Exploration bias: " << (neuromod.norepinephrine - 0.5f) << "\n";
        
        // Working Memory
        auto wm_slots = brain.get_wm_state();
        std::cout << "\nWorking Memory:\n";
        std::cout << "  Active slots: " << wm_slots.size() << "\n";
        for (const auto& slot : wm_slots) {
            std::cout << "    • Item " << slot.item_ref 
                      << " (" << slot.binding_tag << ") "
                      << "precision=" << slot.precision << "\n";
        }
        
        // ====================================================================
        // STATISTICS
        // ====================================================================
        
        print_section("6. System Statistics");
        
        auto stats = brain.get_stats();
        std::cout << "\nUnified Loop:\n";
        std::cout << "  Total cycles: " << stats.total_cycles << "\n";
        std::cout << "  Objects perceived: " << stats.objects_perceived << "\n";
        std::cout << "  Focus switches: " << stats.focus_switches << "\n";
        std::cout << "  WM gates: " << stats.wm_gates << "\n";
        std::cout << "  Avg cycle time: " << stats.avg_cycle_time_ms << " ms\n";
        std::cout << "  Avg surprise: " << stats.avg_surprise << "\n";
        
        // ====================================================================
        // TELEMETRY EXPORT
        // ====================================================================
        
        print_section("7. Exporting Telemetry");
        
        bool exported = brain.export_telemetry("/tmp/melvin_v2_telemetry.json");
        
        if (exported) {
            std::cout << "✓ Telemetry exported to /tmp/melvin_v2_telemetry.json\n";
            std::cout << "  View with: cat /tmp/melvin_v2_telemetry.json | jq .\n";
            std::cout << "  Visualize with: python melvin/tools/python/v2/plot_neuromod.py\n";
        }
        
        // ====================================================================
        // CLEANUP
        // ====================================================================
        
        delete v1_graph;
        
        // ====================================================================
        // SUMMARY
        // ====================================================================
        
        print_section("✓ Cognitive Loop Demo Complete");
        
        std::cout << "\nWhat was demonstrated:\n";
        std::cout << "  ✓ Genome creation (49 genes)\n";
        std::cout << "  ✓ Genome → Brain expression (all components)\n";
        std::cout << "  ✓ Global Workspace (consciousness model)\n";
        std::cout << "  ✓ Neuromodulators (DA/NE/ACh/5-HT dynamics)\n";
        std::cout << "  ✓ Working Memory (gating & decay)\n";
        std::cout << "  ✓ Semantic Bridge (v1 graph access)\n";
        std::cout << "  ✓ Attention System (saliency + topdown + arbitration)\n";
        std::cout << "  ✓ Complete cognitive cycle (10 iterations)\n";
        std::cout << "  ✓ Telemetry export (JSON)\n";
        
        std::cout << "\nNext steps:\n";
        std::cout << "  • Connect real camera (perception bridge)\n";
        std::cout << "  • Add evolution framework (mutation + selection)\n";
        std::cout << "  • Run evolution to optimize genome\n";
        std::cout << "  • Visualize Pareto fronts\n";
        
        std::cout << "\n🧠 MELVIN v2 cognitive loop is operational!\n\n";
        
    } catch (const std::exception& e) {
        std::cerr << "\n❌ Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

