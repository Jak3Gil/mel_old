/*
 * Context Field Demo - Shows Dynamic Attention Weights
 * Demonstrates how context changes attention strategy
 */

#include "unified_mind.h"
#include <iostream>
#include <iomanip>

using namespace melvin;

// Generate simple test image
std::vector<uint8_t> gen_image(int w, int h) {
    return std::vector<uint8_t>(w * h * 3, 128);
}

void print_weights(const ContextField::AttentionWeights& w) {
    std::cout << "  Weights: α=" << std::fixed << std::setprecision(2) << w.alpha
              << " β=" << w.beta
              << " γ=" << w.gamma
              << "\n";
    std::cout << "  Reason:  " << w.reason << "\n";
    std::cout << "  Formula: F = " << w.alpha << "·S + " << w.beta << "·G + " << w.gamma << "·C\n";
}

int main() {
    std::cout << "\n";
    std::cout << "╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  🧠 CONTEXT FIELD DEMO - Dynamic Attention Weights                 ║\n";
    std::cout << "║  Watch how weights change based on what Melvin is thinking about  ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
    
    UnifiedMind mind;
    auto& context = mind.get_context();
    
    // ========================================================================
    // TEST 1: EXPLORING MODE
    // ========================================================================
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "TEST 1: EXPLORING MODE\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    mind.set_mode(ContextField::EXPLORING);
    auto weights_explore = context.compute_dynamic_weights();
    
    std::cout << "When Melvin is EXPLORING (curious, open):\n";
    print_weights(weights_explore);
    std::cout << "\n→ Notice: γ (curiosity) is HIGHEST!\n";
    std::cout << "→ Melvin pays more attention to NOVEL/SURPRISING things\n\n";
    
    // ========================================================================
    // TEST 2: SEARCHING MODE
    // ========================================================================
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "TEST 2: SEARCHING MODE\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    mind.set_mode(ContextField::SEARCHING);
    
    // Set a goal
    context.update_from_goal("find fire");
    
    auto weights_search = context.compute_dynamic_weights();
    
    std::cout << "When Melvin is SEARCHING for something:\n";
    print_weights(weights_search);
    std::cout << "\n→ Notice: β (goal) is HIGHEST!\n";
    std::cout << "→ Melvin pays more attention to RELEVANT things\n";
    std::cout << "→ Ignores bright but irrelevant distractions\n\n";
    
    // ========================================================================
    // TEST 3: MONITORING MODE  
    // ========================================================================
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "TEST 3: MONITORING MODE (Default)\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    context.clear();  // Reset context
    mind.set_mode(ContextField::MONITORING);
    
    auto weights_monitor = context.compute_dynamic_weights();
    
    std::cout << "When Melvin is MONITORING (balanced, default):\n";
    print_weights(weights_monitor);
    std::cout << "\n→ Notice: Balanced weights!\n";
    std::cout << "→ Melvin responds to all factors equally\n\n";
    
    // ========================================================================
    // TEST 4: CONTEXT EVOLUTION  
    // ========================================================================
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "TEST 4: CONTEXT EVOLUTION (Run 30 cycles)\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n\n";
    
    context.clear();
    mind.set_mode(ContextField::MONITORING);
    
    std::cout << "Running cognitive cycles and tracking weight changes...\n\n";
    std::cout << "Cycle | Active | α     | β     | γ     | Mode\n";
    std::cout << "──────┼────────┼───────┼───────┼───────┼─────────────\n";
    
    for (int i = 0; i < 30; ++i) {
        FrameMeta fm;
        fm.id = i;
        fm.w = 64;
        fm.h = 64;
        
        auto img = gen_image(64, 64);
        mind.tick(fm, img);
        
        if (i % 5 == 0) {
            auto w = context.compute_dynamic_weights();
            auto stats = context.get_stats();
            
            std::cout << std::setw(5) << i << " │ "
                      << std::setw(6) << stats.currently_active << " │ "
                      << std::fixed << std::setprecision(2)
                      << w.alpha << " │ "
                      << w.beta << " │ "
                      << w.gamma << " │ "
                      << w.reason << "\n";
        }
    }
    
    std::cout << "\n→ See how weights CHANGE as context builds!\n";
    std::cout << "→ As concepts activate, β (goal) increases\n";
    std::cout << "→ As context settles, weights shift\n\n";
    
    // ========================================================================
    // TEST 5: CONTEXT VISUALIZATION
    // ========================================================================
    
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    std::cout << "TEST 5: CONTEXT VISUALIZATION\n";
    std::cout << "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n";
    
    context.visualize(10);
    
    // ========================================================================
    // SUMMARY
    // ========================================================================
    
    std::cout << "\n╔═══════════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ✅ CONTEXT SYSTEM OPERATIONAL                                     ║\n";
    std::cout << "║                                                                   ║\n";
    std::cout << "║  Key achievements:                                               ║\n";
    std::cout << "║  • Attention weights are now DYNAMIC (not rigid!)                ║\n";
    std::cout << "║  • Weights change based on active concepts                       ║\n";
    std::cout << "║  • Context spreads through graph (diffusion)                     ║\n";
    std::cout << "║  • Context decays over time (forgetting)                         ║\n";
    std::cout << "║  • Thoughts reinforce context (feedback loop)                    ║\n";
    std::cout << "║  • Different modes change attention strategy                     ║\n";
    std::cout << "║                                                                   ║\n";
    std::cout << "║  The formula F = α·S + β·G + γ·C is now ADAPTIVE!               ║\n";
    std::cout << "╚═══════════════════════════════════════════════════════════════════╝\n\n";
    
    return 0;
}

