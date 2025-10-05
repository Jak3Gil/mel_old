#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <cstdio>

using namespace std;

/**
 * Micro HUD - Minimal progress bar for single-test runs
 * Shows 1 line per step with key metrics
 */

static vector<string> split(const string& s, char sep) {
    vector<string> t;
    string cur;
    for (char c : s) {
        if (c == sep) {
            t.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    t.push_back(cur);
    return t;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "📊 Micro HUD for Melvin\n";
        cerr << "usage: " << argv[0] << " <metrics_csv>\n";
        cerr << "\nExample:\n";
        cerr << "  " << argv[0] << " out/metrics_live.csv\n";
        return 1;
    }
    
    string path = argv[1];
    ifstream f;
    size_t lastSize = 0;
    vector<string> raw;
    
    cout << "📊 Micro HUD - Monitoring " << path << "..." << endl;
    cout << "Press Ctrl+C to exit" << endl << endl;
    
    while (true) {
        this_thread::sleep_for(chrono::milliseconds(200));
        
        // Reopen each tick
        f.close();
        f.clear();
        f.open(path);
        
        if (!f) {
            fprintf(stderr, "\r⏳ Waiting for %s...", path.c_str());
            fflush(stderr);
            continue;
        }
        
        raw.clear();
        string line;
        while (getline(f, line)) {
            raw.push_back(line);
        }
        
        if (raw.size() <= 1) {
            fprintf(stderr, "\r⏳ (waiting for data)");
            fflush(stderr);
            continue;
        }
        
        // Parse last line
        auto cols = split(raw.back(), ',');
        if (cols.size() < 21) continue;
        
        try {
            uint64_t step = stoull(cols[1]);
            uint64_t nodes = stoull(cols[2]);
            uint64_t edges = stoull(cols[3]);
            uint64_t paths = stoull(cols[4]);
            uint64_t taught = stoull(cols[5]);
            uint64_t thought = stoull(cols[6]);
            uint64_t rel_temporal = stoull(cols[8]);
            uint64_t rel_leap = stoull(cols[9]);
            double avg_path_len = stod(cols[12]);
            double health = stod(cols[19]);
            double entropy = stod(cols[16]);
            uint64_t fanout = stoull(cols[21]);
            
            // Calculate deltas (simplified - would need previous values for accuracy)
            int64_t delta_nodes = 0, delta_edges = 0, delta_paths = 0;
            if (raw.size() > 1) {
                auto prev_cols = split(raw[raw.size()-2], ',');
                if (prev_cols.size() >= 4) {
                    delta_nodes = nodes - stoull(prev_cols[2]);
                    delta_edges = edges - stoull(prev_cols[3]);
                    delta_paths = paths - stoull(prev_cols[4]);
                }
            }
            
            // Health color
            const char* health_color = "\033[31m"; // Red
            if (health >= 0.80) health_color = "\033[32m"; // Green
            else if (health >= 0.60) health_color = "\033[33m"; // Yellow
            
            // Stall detection
            bool stalled = (delta_nodes == 0 && delta_edges == 0 && delta_paths == 0);
            bool runaway = (delta_edges > 50 && entropy > 0.40);
            bool locked = (entropy < 0.06);
            
            string alert = "";
            if (stalled) alert = " ⏸️ STALLED";
            else if (runaway) alert = " ⚠️ RUNAWAY";
            else if (locked) alert = " 🔒 LOCKED";
            
            // Print single line with key metrics
            fprintf(stderr, 
                "\rStep:%4llu  Nodes:%5llu (+%3ld)  Edges:%6llu (+%4ld)  Paths:%5llu (+%3ld)  "
                "H=%s%.2f\033[0m  Ent=%.3f  k=%2llu  T:%llu/%llu  TL:%llu/%llu  PL:%.1f%s",
                step, nodes, delta_nodes, edges, delta_edges, paths, delta_paths,
                health_color, health, entropy, fanout, taught, thought, 
                rel_temporal, rel_leap, avg_path_len, alert.c_str());
            fflush(stderr);
            
        } catch (const exception& e) {
            // Skip malformed lines
            continue;
        }
    }
    
    return 0;
}
