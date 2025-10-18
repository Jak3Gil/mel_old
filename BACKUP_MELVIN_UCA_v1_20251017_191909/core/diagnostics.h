#pragma once

#include <string>
#include <fstream>

namespace melvin {
namespace diagnostics {

/**
 * Log reasoning energy per hop for analysis
 */
inline void log_reasoning_energy(int hop, float energy, const std::string& query = "") {
    std::ofstream log("data/reasoning_energy.csv", std::ios::app);
    
    // Create header if file is new
    static bool header_written = false;
    if (!header_written) {
        std::ifstream check("data/reasoning_energy.csv");
        if (!check.good()) {
            log << "query,hop,energy\n";
        }
        header_written = true;
    }
    
    log << "\"" << query << "\"," << hop << "," << energy << "\n";
}

/**
 * Log complete reasoning session
 */
inline void log_reasoning_session(
    const std::string& query,
    int actual_hops,
    float final_energy,
    float confidence,
    bool early_termination
) {
    std::ofstream log("data/reasoning_sessions.csv", std::ios::app);
    
    static bool header_written = false;
    if (!header_written) {
        std::ifstream check("data/reasoning_sessions.csv");
        if (!check.good()) {
            log << "query,hops,final_energy,confidence,early_term\n";
        }
        header_written = true;
    }
    
    log << "\"" << query << "\"," << actual_hops << "," 
        << final_energy << "," << confidence << "," 
        << (early_termination ? "1" : "0") << "\n";
}

} // namespace diagnostics
} // namespace melvin

