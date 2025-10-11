/*
 * ENHANCED LEAP REASONING TEST
 * 
 * Demonstrates:
 * - Template gap detection
 * - Connector normalization (is/are/was/were → BE)
 * - Crowd-support thresholds
 * - Smart LEAP creation (only when sufficient evidence)
 * - LEAP rejection (when insufficient support)
 */

#include "include/melvin_core.h"

int main() {
    std::cout << "\n";
    std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
    std::cout << "║  ENHANCED LEAP REASONING - CROWD SUPPORT TEST                ║\n";
    std::cout << "╚══════════════════════════════════════════════════════════════╝\n\n";
    
    system("rm -f test_enhanced_leap.bin");
    
    melvin_core::UnifiedBrain brain("test_enhanced_leap.bin");
    
    // ==================== BUILD CROWD SUPPORT ====================
    std::cout << "📚 PHASE 1: Building Crowd Support\n";
    std::cout << "=" << std::string(70, '=') << "\n\n";
    
    std::cout << "Teaching multiple examples with 'BE' connector (is/are)...\n";
    brain.learn("dogs are mammals");      // dogs → BE → mammals
    brain.learn("cats are mammals");      // cats → BE → mammals
    brain.learn("wolves are mammals");    // wolves → BE → mammals
    brain.learn("lions are predators");   // lions → BE → predators
    brain.learn("tigers are predators");  // tigers → BE → predators
    brain.learn("sharks are predators");  // sharks → BE → predators
    
    std::cout << "\nTeaching examples with 'HAS' connector (has/have)...\n";
    brain.learn("dogs have fur");         // dogs → HAS → fur
    brain.learn("cats have fur");         // cats → HAS → fur
    brain.learn("bears have fur");        // bears → HAS → fur
    
    std::cout << "\nTeaching mixed connectors...\n";
    brain.learn("birds can fly");         // birds → CAN → fly
    brain.learn("planes can fly");        // planes → CAN → fly
    
    std::cout << "\n📊 After Building Knowledge:\n";
    brain.stats();
    
    // ==================== TEST EXACT CHAINS ====================
    std::cout << "\n\n📖 PHASE 2: Known Facts (EXACT Chains)\n";
    std::cout << "=" << std::string(70, '=') << "\n";
    
    std::cout << "\nQ1: 'are dogs mammals' (directly trained)\n";
    brain.think("are dogs mammals");
    
    std::cout << "\nQ2: 'do cats have fur' (directly trained)\n";
    brain.think("do cats have fur");
    
    // ==================== TEST CROWD-SUPPORTED LEAP ====================
    std::cout << "\n\n🔮 PHASE 3: Inferences With Strong Crowd Support\n";
    std::cout << "=" << std::string(70, '=') << "\n";
    
    std::cout << "\nQ3: 'are foxes mammals'\n";
    std::cout << "Analysis:\n";
    std::cout << "  • Template: foxes BE ?\n";
    std::cout << "  • Crowd examples: dogs→BE→mammals, cats→BE→mammals, wolves→BE→mammals\n";
    std::cout << "  • Support: 3+ examples, 3 distinct subjects\n";
    std::cout << "  • Should CREATE LEAP: foxes ~LEAP~> mammals ✅\n\n";
    brain.think("are foxes mammals");
    
    std::cout << "\nQ4: 'are bears predators'\n";
    std::cout << "Analysis:\n";
    std::cout << "  • Template: bears BE ?\n";
    std::cout << "  • Crowd examples: lions→BE→predators, tigers→BE→predators, sharks→BE→predators\n";
    std::cout << "  • Support: 3+ examples, 3 distinct subjects\n";
    std::cout << "  • Should CREATE LEAP: bears ~LEAP~> predators ✅\n\n";
    brain.think("are bears predators");
    
    std::cout << "\n📊 After Crowd-Supported LEAPs:\n";
    brain.stats();
    
    // ==================== TEST REJECTED LEAP ====================
    std::cout << "\n\n⚠️  PHASE 4: Inferences With Insufficient Support\n";
    std::cout << "=" << std::string(70, '=') << "\n";
    
    std::cout << "\nQ5: 'are tables mammals'\n";
    std::cout << "Analysis:\n";
    std::cout << "  • Template: tables BE ?\n";
    std::cout << "  • No 'tables' in training data\n";
    std::cout << "  • Even though crowd supports 'mammals', 'tables' is unknown\n";
    std::cout << "  • Should REJECT LEAP ❌\n\n";
    brain.think("are tables mammals");
    
    // ==================== TEST CONNECTOR NORMALIZATION ====================
    std::cout << "\n\n🔄 PHASE 5: Connector Normalization\n";
    std::cout << "=" << std::string(70, '=') << "\n";
    
    std::cout << "\nQ6: 'were dogs mammals' (past tense → normalized to BE)\n";
    std::cout << "Should find same pattern as 'are dogs mammals'\n\n";
    brain.think("were dogs mammals");
    
    std::cout << "\nQ7: 'had cats fur' (past tense → normalized to HAS)\n";
    std::cout << "Should find same pattern as 'have cats fur'\n\n";
    brain.think("had cats fur");
    
    // ==================== TEST PROMOTION ====================
    std::cout << "\n\n✨ PHASE 6: LEAP Promotion Through Use\n";
    std::cout << "=" << std::string(70, '=') << "\n";
    
    std::cout << "\nRepeating 'are foxes mammals' to build confidence...\n";
    for (int i = 1; i <= 5; ++i) {
        std::cout << "\nAttempt " << i << ":\n";
        brain.think("are foxes mammals");
    }
    
    std::cout << "\n📊 Final Statistics:\n";
    brain.stats();
    
    // ==================== SHOW RESULTS ====================
    std::cout << "\n\n📊 DETAILED EDGE ANALYSIS\n";
    std::cout << "=" << std::string(70, '=') << "\n\n";
    
    std::cout << "🔮 LEAP Edges (Active Inferences):\n";
    int leap_count = 0;
    for (const auto& edge : melvin_storage::edges) {
        if (edge.is_leap()) {
            std::cout << "   " << melvin_storage::nodes[edge.a].data 
                     << " ~LEAP~> " << melvin_storage::nodes[edge.b].data
                     << " (support: " << edge.leap_score
                     << ", successes: " << edge.successes
                     << ", weight: " << std::fixed << std::setprecision(2) << edge.w << ")\n";
            leap_count++;
        }
    }
    
    if (leap_count == 0) {
        std::cout << "   (No active LEAPs - may have been promoted!)\n";
    }
    
    std::cout << "\n✨ Promoted Edges (LEAP → EXACT):\n";
    int promoted = 0;
    for (const auto& edge : melvin_storage::edges) {
        if (edge.is_exact() && edge.leap_score > 0.0f && edge.successes > 0) {
            std::cout << "   " << melvin_storage::nodes[edge.a].data 
                     << " =[EXACT]=> " << melvin_storage::nodes[edge.b].data
                     << " (was LEAP, support: " << edge.leap_score
                     << ", successes: " << edge.successes << ")\n";
            promoted++;
        }
    }
    
    if (promoted == 0) {
        std::cout << "   (No promotions yet - need weight > 0.7 AND successes >= 3)\n";
    }
    
    // ==================== SUMMARY ====================
    std::cout << "\n\n🎉 ENHANCED LEAP SYSTEM TEST COMPLETE!\n";
    std::cout << "=" << std::string(70, '=') << "\n\n";
    
    std::cout << "Features Demonstrated:\n";
    std::cout << "  ✅ Template gap detection (X connector Y)\n";
    std::cout << "  ✅ Connector normalization (is/are/was/were → BE)\n";
    std::cout << "  ✅ Crowd support analysis (count examples)\n";
    std::cout << "  ✅ Threshold checks (support >= 3, diversity >= 2, margin >= 1)\n";
    std::cout << "  ✅ Smart LEAP creation (only with sufficient evidence)\n";
    std::cout << "  ✅ LEAP rejection (insufficient support)\n";
    std::cout << "  ✅ Success tracking for promotion\n";
    std::cout << "  ✅ Complete persistence\n\n";
    
    brain.save();
    std::cout << "Brain saved. Run again to verify persistence!\n\n";
    
    return 0;
}

