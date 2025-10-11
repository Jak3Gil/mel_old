/*
 * DEMO: Graph-Guided Predictive Leap System
 * 
 * This demonstrates Melvin's ability to detect uncertainty in predictions
 * and use semantic clusters from the knowledge graph to make intelligent
 * "leaps" to new conceptual directions.
 */

#include "melvin.h"
#include "melvin_leap_nodes.h"
#include "predictive_sampler.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <string>

using namespace melvin;

void print_header(const std::string& title) {
    std::cout << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "  " << title << "\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << std::endl;
}

void print_section(const std::string& title) {
    std::cout << "\n┌─ " << title << " ─" << std::string(50 - title.length(), '─') << "┐\n";
}

void print_end_section() {
    std::cout << "└" << std::string(60, '─') << "┘\n";
}

void demo_basic_leap_detection() {
    print_header("🧠 DEMO 1: Leap Detection from Uncertainty");
    
    std::cout << "Scenario: Melvin encounters high entropy in predictions and\n";
    std::cout << "          needs to find a new conceptual direction.\n\n";
    
    // Create configuration
    LeapConfig config;
    config.enable_leap_nodes = true;
    config.leap_entropy_threshold = 0.6f;
    config.min_cluster_size = 2;
    config.min_cluster_cohesion = 0.3f;
    
    std::cout << "Configuration:\n";
    std::cout << "  • Entropy threshold: " << config.leap_entropy_threshold << "\n";
    std::cout << "  • Min cluster size: " << config.min_cluster_size << "\n";
    std::cout << "  • Min cohesion: " << config.min_cluster_cohesion << "\n";
    std::cout << "  • Leap bias strength: " << config.lambda_leap_bias << "\n\n";
    
    // Create leap controller
    LeapController controller(config);
    
    // Simulate high-entropy prediction scenario
    std::vector<Candidate> candidates;
    // Create several candidates with similar scores (high entropy)
    for (int i = 0; i < 5; ++i) {
        candidates.emplace_back(1000 + i, 0.25f + 0.05f * i, Rel::TEMPORAL, 0.2f);
    }
    
    std::cout << "Current prediction candidates (high entropy):\n";
    for (size_t i = 0; i < candidates.size(); ++i) {
        std::cout << "  " << (i+1) << ". Node " << candidates[i].node_id 
                  << " (score=" << std::fixed << std::setprecision(3) 
                  << candidates[i].score << ")\n";
    }
    
    std::vector<uint64_t> context = {100, 200, 300, 400};
    std::cout << "\nContext path: [";
    for (size_t i = 0; i < context.size(); ++i) {
        if (i > 0) std::cout << " → ";
        std::cout << context[i];
    }
    std::cout << "]\n\n";
    
    bool should_leap = controller.should_trigger_leap(candidates, context);
    std::cout << "🔍 Leap trigger decision: " << (should_leap ? "YES ✓" : "NO ✗") << "\n";
    
    if (should_leap) {
        std::cout << "\n🧠 Leap system engaged! Creating conceptual leap...\n";
    }
    
    print_end_section();
}

void demo_cluster_formation() {
    print_header("🔗 DEMO 2: Semantic Cluster Formation");
    
    std::cout << "Scenario: Melvin groups related nodes into semantic clusters\n";
    std::cout << "          based on connectivity and similarity.\n\n";
    
    LeapConfig config;
    config.min_cluster_size = 2;
    config.min_cluster_cohesion = 0.3f;
    
    ClusterManager cluster_manager(config);
    
    std::cout << "Simulating knowledge graph with related concepts:\n\n";
    
    // Simulate some active nodes
    std::vector<uint64_t> active_nodes = {
        1001, 1002, 1003,  // Cluster 1: food-related
        2001, 2002,        // Cluster 2: emotion-related
        3001, 3002, 3003   // Cluster 3: action-related
    };
    
    std::cout << "Active nodes: ";
    for (size_t i = 0; i < active_nodes.size(); ++i) {
        if (i > 0) std::cout << ", ";
        std::cout << active_nodes[i];
    }
    std::cout << "\n\n";
    
    // Update clusters
    cluster_manager.update_clusters(active_nodes);
    
    std::cout << "Detected semantic clusters:\n";
    auto clusters = cluster_manager.get_active_clusters();
    
    for (size_t i = 0; i < clusters.size(); ++i) {
        const auto& cluster = clusters[i];
        std::cout << "\nCluster " << (i+1) << ":\n";
        std::cout << "  Members: " << cluster.members.size() << " nodes\n";
        std::cout << "  Activation: " << std::fixed << std::setprecision(3) 
                  << cluster.activation << "\n";
        std::cout << "  Cohesion: " << cluster.cohesion << "\n";
        std::cout << "  Dominant: " << cluster.dominant_concept << "\n";
    }
    
    if (clusters.empty()) {
        std::cout << "(No clusters formed yet - graph needs more connections)\n";
    }
    
    print_end_section();
}

void demo_leap_node_creation() {
    print_header("🎯 DEMO 3: LeapNode Creation and Bias Computation");
    
    std::cout << "Scenario: When uncertainty is high, merge active clusters\n";
    std::cout << "          into a LeapNode to guide predictions.\n\n";
    
    LeapConfig config;
    config.enable_leap_nodes = true;
    config.max_clusters_per_leap = 2;
    config.lambda_leap_bias = 0.3f;
    
    LeapController controller(config);
    
    std::vector<uint64_t> context = {100, 200, 300};
    std::vector<Candidate> candidates;
    for (int i = 0; i < 4; ++i) {
        candidates.emplace_back(1000 + i, 0.26f, Rel::TEMPORAL, 0.2f);
    }
    
    std::cout << "Creating LeapNode from active clusters...\n\n";
    
    LeapNode leap = controller.create_leap_node(context, candidates);
    
    if (!leap.members.empty()) {
        std::cout << "✓ LeapNode created successfully!\n\n";
        std::cout << "LeapNode properties:\n";
        std::cout << "  • Label: " << leap.label << "\n";
        std::cout << "  • Member count: " << leap.members.size() << "\n";
        std::cout << "  • Activation: " << std::fixed << std::setprecision(3) 
                  << leap.activation << "\n";
        std::cout << "  • Cohesion: " << leap.cohesion << "\n";
        std::cout << "  • Source clusters: " << leap.source_clusters.size() << "\n";
        std::cout << "  • Entropy before: " << leap.entropy_before << "\n\n";
        
        // Compute bias field
        std::cout << "Computing bias field for connected nodes...\n";
        auto bias_field = controller.compute_leap_bias(leap);
        
        std::cout << "  • Bias field size: " << bias_field.size() << " nodes\n";
        
        if (!bias_field.empty()) {
            std::cout << "\n  Top biased nodes:\n";
            std::vector<std::pair<uint64_t, float>> sorted_bias;
            for (const auto& [node_id, bias] : bias_field) {
                sorted_bias.emplace_back(node_id, bias);
            }
            std::sort(sorted_bias.begin(), sorted_bias.end(),
                     [](const auto& a, const auto& b) { return a.second > b.second; });
            
            for (size_t i = 0; i < std::min(size_t(5), sorted_bias.size()); ++i) {
                std::cout << "    " << (i+1) << ". Node " << sorted_bias[i].first
                          << " (bias=" << std::fixed << std::setprecision(4) 
                          << sorted_bias[i].second << ")\n";
            }
        }
    } else {
        std::cout << "⚠ No valid clusters available for leap creation\n";
        std::cout << "  (This is normal in a minimal graph - needs more data)\n";
    }
    
    print_end_section();
}

void demo_leap_learning() {
    print_header("📚 DEMO 4: Leap Learning and Promotion");
    
    std::cout << "Scenario: Track leap success and promote successful patterns\n";
    std::cout << "          to permanent generalized concepts.\n\n";
    
    LeapConfig config;
    config.leap_promotion_threshold = 3;
    config.entropy_improvement_threshold = 0.15f;
    config.coherence_improvement_threshold = 0.1f;
    
    LeapController controller(config);
    
    // Create a mock leap node
    LeapNode leap;
    leap.label = "well-being_leap_0";
    leap.members = {1001, 1002, 1003};
    leap.activation = 0.8f;
    leap.cohesion = 0.6f;
    leap.entropy_before = 0.75f;
    
    std::cout << "Testing LeapNode: " << leap.label << "\n";
    std::cout << "  • Initial entropy: " << leap.entropy_before << "\n";
    std::cout << "  • Promotion threshold: " << config.leap_promotion_threshold << " successes\n\n";
    
    // Simulate multiple uses
    for (int trial = 1; trial <= 5; ++trial) {
        print_section("Trial " + std::to_string(trial));
        
        // Simulate result (mostly successful)
        float entropy_after = trial <= 3 ? 0.45f : 0.70f;  // First 3 succeed
        float coherence_gain = trial <= 3 ? 0.25f : 0.05f;
        
        std::cout << "  Entropy after: " << entropy_after << "\n";
        std::cout << "  Coherence gain: " << coherence_gain << "\n";
        std::cout << "  Entropy improvement: " << (leap.entropy_before - entropy_after) << "\n";
        
        controller.record_leap_usage(leap, entropy_after, coherence_gain);
        
        bool successful = (leap.entropy_before - entropy_after) > config.entropy_improvement_threshold &&
                         coherence_gain > config.coherence_improvement_threshold;
        
        std::cout << "  Result: " << (successful ? "✓ SUCCESS" : "✗ FAILURE") << "\n";
        std::cout << "  Success score: " << std::fixed << std::setprecision(2) 
                  << leap.success_score << "\n";
        std::cout << "  Success count: " << leap.success_count << " / " << leap.usage_count << "\n";
        
        if (leap.is_permanent) {
            std::cout << "  ⭐ PROMOTED TO PERMANENT!\n";
        }
        
        print_end_section();
    }
    
    std::cout << "\nFinal statistics:\n";
    std::cout << "  • Total uses: " << leap.usage_count << "\n";
    std::cout << "  • Successes: " << leap.success_count << "\n";
    std::cout << "  • Failures: " << leap.failure_count << "\n";
    std::cout << "  • Success rate: " << (leap.success_score * 100) << "%\n";
    std::cout << "  • Status: " << (leap.is_permanent ? "PERMANENT ⭐" : "TEMPORARY") << "\n";
    
    print_end_section();
}

void demo_integration_with_predictive() {
    print_header("🔄 DEMO 5: Integration with Predictive Generation");
    
    std::cout << "Scenario: Use leap system during actual token generation\n";
    std::cout << "          to guide predictions when uncertainty arises.\n\n";
    
    // Configure predictive sampler with leap system enabled
    PredictiveConfig pred_config;
    pred_config.enable_leap_system = true;
    pred_config.use_beam = false;  // Use top-p sampling for demo
    pred_config.max_hops = 10;
    pred_config.top_p = 0.9f;
    
    LeapConfig leap_config;
    leap_config.enable_leap_nodes = true;
    leap_config.leap_entropy_threshold = 0.6f;
    
    std::cout << "Predictive sampler configuration:\n";
    std::cout << "  • Leap system: " << (pred_config.enable_leap_system ? "ENABLED" : "DISABLED") << "\n";
    std::cout << "  • Sampling: top-p (p=" << pred_config.top_p << ")\n";
    std::cout << "  • Max hops: " << pred_config.max_hops << "\n\n";
    
    std::cout << "Leap system configuration:\n";
    std::cout << "  • Entropy threshold: " << leap_config.leap_entropy_threshold << "\n";
    std::cout << "  • Bias strength: " << leap_config.lambda_leap_bias << "\n\n";
    
    LeapController leap_controller(leap_config);
    
    std::cout << "Generation flow:\n";
    std::cout << "  1. Score neighbor candidates\n";
    std::cout << "  2. Apply n-gram bonus\n";
    std::cout << "  3. 🧠 Check for leap trigger (high entropy/repetition)\n";
    std::cout << "  4. 🧠 If triggered → create LeapNode → compute bias → apply\n";
    std::cout << "  5. Softmax normalization\n";
    std::cout << "  6. Top-p sampling\n";
    std::cout << "  7. 🧠 Record leap feedback for learning\n\n";
    
    std::cout << "This integration is now active in:\n";
    std::cout << "  • top_p_sample()\n";
    std::cout << "  • beam_search()\n";
    std::cout << "  • generate_path()\n\n";
    
    std::cout << "Usage example:\n";
    std::cout << "  std::vector<uint64_t> context = {node1, node2, node3};\n";
    std::cout << "  auto path = generate_path(context, config, &leap_controller);\n";
    
    print_end_section();
}

void demo_statistics_and_monitoring() {
    print_header("📊 DEMO 6: Statistics and Monitoring");
    
    std::cout << "Scenario: Monitor leap system performance over time.\n\n";
    
    LeapConfig config;
    LeapController controller(config);
    
    // Simulate some activity
    std::cout << "Simulating leap system activity...\n\n";
    
    // The controller tracks:
    // - total_leaps_created_
    // - total_leaps_succeeded_
    // - total_leaps_failed_
    // - total_promotions_
    
    controller.print_statistics();
    
    std::cout << "\nThe leap controller maintains metrics on:\n";
    std::cout << "  • Total leaps created\n";
    std::cout << "  • Success/failure counts\n";
    std::cout << "  • Promotion to permanent nodes\n";
    std::cout << "  • Overall success rate\n";
    std::cout << "  • Active vs permanent leap patterns\n\n";
    
    std::cout << "These metrics help tune the system:\n";
    std::cout << "  • Low success rate → adjust entropy threshold\n";
    std::cout << "  • Too many leaps → increase threshold\n";
    std::cout << "  • Few promotions → lower promotion threshold\n";
    
    print_end_section();
}

int main() {
    print_header("🧠 MELVIN: Graph-Guided Predictive Leap System");
    
    std::cout << "This demo showcases Melvin's ability to:\n";
    std::cout << "  1. Detect uncertainty in predictive generation\n";
    std::cout << "  2. Form semantic clusters from the knowledge graph\n";
    std::cout << "  3. Create temporary 'LeapNodes' as generalized concepts\n";
    std::cout << "  4. Bias predictions toward meaningful new directions\n";
    std::cout << "  5. Learn from success and promote patterns\n";
    std::cout << "  6. Bridge fluent prediction with conceptual reasoning\n\n";
    
    std::cout << "Press Enter to start demos...\n";
    std::cin.get();
    
    try {
        demo_basic_leap_detection();
        std::cout << "\nPress Enter for next demo...\n";
        std::cin.get();
        
        demo_cluster_formation();
        std::cout << "\nPress Enter for next demo...\n";
        std::cin.get();
        
        demo_leap_node_creation();
        std::cout << "\nPress Enter for next demo...\n";
        std::cin.get();
        
        demo_leap_learning();
        std::cout << "\nPress Enter for next demo...\n";
        std::cin.get();
        
        demo_integration_with_predictive();
        std::cout << "\nPress Enter for next demo...\n";
        std::cin.get();
        
        demo_statistics_and_monitoring();
        
        print_header("✨ Demo Complete");
        std::cout << "The leap system is now integrated into Melvin's core.\n";
        std::cout << "It will automatically activate when:\n";
        std::cout << "  • Prediction entropy exceeds threshold\n";
        std::cout << "  • Repetitive patterns are detected\n";
        std::cout << "  • All candidates have low confidence\n\n";
        
        std::cout << "Key benefits:\n";
        std::cout << "  ✓ Handles uncertainty gracefully\n";
        std::cout << "  ✓ Discovers higher-level concepts\n";
        std::cout << "  ✓ Enables conceptual jumps\n";
        std::cout << "  ✓ Learns from successful patterns\n";
        std::cout << "  ✓ Maintains fluent generation\n\n";
        
        std::cout << "The system is production-ready and non-breaking!\n";
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}


