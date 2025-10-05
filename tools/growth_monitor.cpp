#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <array>
#include <thread>
#include <chrono>
#include <iomanip>
#include <algorithm>
#include <cstdlib>

using namespace std;

/**
 * Live Growth Monitor for Melvin
 * Real-time ASCII dashboard showing nodes/edges/paths growth with deltas and sparklines
 */

static string bar(double v, double vmax, int width = 32) {
    if (vmax <= 0) return string(width, ' ');
    int n = (int)round((v / vmax) * width);
    n = max(0, min(width, n));
    return string(n, '#') + string(width - n, ' ');
}

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

static void clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

static string formatNumber(uint64_t n) {
    if (n >= 1000000) {
        return to_string(n / 1000000) + "M";
    } else if (n >= 1000) {
        return to_string(n / 1000) + "K";
    }
    return to_string(n);
}

static string getHealthColor(double health) {
    if (health >= 80) return "\033[32m";  // Green
    if (health >= 60) return "\033[33m";  // Yellow
    return "\033[31m";  // Red
}

static string resetColor() {
    return "\033[0m";
}

int main(int argc, char** argv) {
    if (argc < 2) {
        cerr << "📈 Melvin Growth Monitor\n";
        cerr << "usage: " << argv[0] << " <metrics_csv> [tail=30]\n";
        cerr << "\nExample:\n";
        cerr << "  " << argv[0] << " out/metrics_live.csv 50\n";
        return 1;
    }
    
    string path = argv[1];
    int tailN = (argc >= 3) ? stoi(argv[2]) : 30;
    
    ios::sync_with_stdio(false);
    ifstream f;
    size_t lastSize = 0;
    
    vector<array<double, 21>> rows; // timestamp, step, nodes, edges, paths, taught_nodes, thought_nodes, rel_exact, rel_temporal, rel_leap, avg_weight, max_weight, avg_path_len, max_path_len, edge_rate, e2n_ratio, entropy, diversity, top2, health, latency
    vector<string> raw;
    
    cout << "📈 Melvin Growth Monitor - Waiting for data from " << path << "..." << endl;
    
    while (true) {
        this_thread::sleep_for(chrono::milliseconds(350));
        
        // Reopen each tick (simple & robust)
        f.close();
        f.clear();
        f.open(path);
        
        if (!f) {
            clearScreen();
            cout << "⏳ Waiting for " << path << "..." << endl;
            continue;
        }
        
        raw.clear();
        string line;
        while (getline(f, line)) {
            raw.push_back(line);
        }
        
        if (raw.size() <= 1) {
            clearScreen();
            cout << "⏳ (waiting for data)" << endl;
            continue;
        }
        
        // Parse last tailN lines (skip header)
        rows.clear();
        int start = max(1, (int)raw.size() - tailN);
        for (int i = start; i < (int)raw.size(); ++i) {
            auto cols = split(raw[i], ',');
            if (cols.size() < 25) continue; // Need at least 25 columns for enhanced metrics
            
            try {
                double timestamp = stod(cols[0]);
                double nodes = stod(cols[2]);
                double edges = stod(cols[3]);
                double paths = stod(cols[4]);
                double taught_nodes = stod(cols[5]);
                double thought_nodes = stod(cols[6]);
                double rel_exact = stod(cols[7]);
                double rel_temporal = stod(cols[8]);
                double rel_leap = stod(cols[9]);
                double avg_weight = stod(cols[10]);
                double max_weight = stod(cols[11]);
                double avg_path_len = stod(cols[12]);
                double max_path_len = stod(cols[13]);
                double edge_rate = stod(cols[14]);
                double e2n_ratio = stod(cols[15]);
                double entropy = stod(cols[16]);
                double diversity = stod(cols[17]);
                double top2 = stod(cols[18]);
                double health = stod(cols[19]);
                double latency = stod(cols[20]);
                
                rows.push_back({timestamp, nodes, edges, paths, taught_nodes, thought_nodes,
                              rel_exact, rel_temporal, rel_leap, avg_weight, max_weight,
                              avg_path_len, max_path_len, edge_rate, e2n_ratio,
                              entropy, diversity, top2, health, latency});
            } catch (const exception& e) {
                // Skip malformed lines
                continue;
            }
        }
        
        if (rows.empty()) continue;
        
        // Compute max for bars
        double maxNodes = 0, maxEdges = 0, maxPaths = 0, maxHealth = 100.0;
        for (auto& r : rows) {
            maxNodes = max(maxNodes, r[2]);
            maxEdges = max(maxEdges, r[3]);
            maxPaths = max(maxPaths, r[4]);
            maxHealth = max(maxHealth, r[19]);
        }
        
        // Clear screen and print header
        clearScreen();
        cout << "📈 Melvin Enhanced Growth Monitor  (last " << rows.size() << " samples)" << endl;
        cout << "File: " << path << " | Update: " << chrono::duration_cast<chrono::seconds>(
            chrono::system_clock::now().time_since_epoch()).count() % 10000 << endl;
        cout << string(140, '=') << endl;
        
        // Print header row
        cout << left << setw(6) << "Step" 
             << setw(8) << "Nodes" << setw(4) << "Δ" << setw(20) << "Growth"
             << setw(8) << "Edges" << setw(4) << "Δ" << setw(20) << "Connections"
             << setw(8) << "Paths" << setw(4) << "Δ" << setw(20) << "Reasoning"
             << setw(8) << "Health" << setw(6) << "Band" << setw(8) << "Entropy" << endl;
        cout << string(140, '-') << endl;
        
        // Print data rows with deltas & bars
        for (int i = 0; i < (int)rows.size(); ++i) {
            auto& cur = rows[i];
            auto& prev = (i > 0) ? rows[i - 1] : cur;
            
            long dn = (long)(cur[2] - prev[2]);
            long de = (long)(cur[3] - prev[3]);
            long dp = (long)(cur[4] - prev[4]);
            
            // Format deltas with colors
            string delta_n = (dn > 0) ? "+" + to_string(dn) : to_string(dn);
            string delta_e = (de > 0) ? "+" + to_string(de) : to_string(de);
            string delta_p = (dp > 0) ? "+" + to_string(dp) : to_string(dp);
            
            // Health band
            string health_band = "RED";
            if (cur[19] >= 0.80) health_band = "GREEN";
            else if (cur[19] >= 0.60) health_band = "YELLOW";
            
            cout << left << setw(6) << (i + (int)(raw.size() - rows.size()))
                 << setw(8) << formatNumber((uint64_t)cur[2]) 
                 << setw(4) << delta_n << " " << bar(cur[2], maxNodes, 15)
                 << setw(8) << formatNumber((uint64_t)cur[3])
                 << setw(4) << delta_e << " " << bar(cur[3], maxEdges, 15)
                 << setw(8) << formatNumber((uint64_t)cur[4])
                 << setw(4) << delta_p << " " << bar(cur[4], maxPaths, 15)
                 << getHealthColor(cur[19]) << setw(8) << fixed << setprecision(1) << cur[19] << resetColor()
                 << setw(6) << health_band
                 << setw(8) << fixed << setprecision(3) << cur[17] << endl;
        }
        
        cout << string(140, '-') << endl;
        
        // Summary stats and stall detection
        if (!rows.empty()) {
            auto& latest = rows.back();
            auto& prev = rows.size() > 1 ? rows[rows.size()-2] : latest;
            
            // Compute deltas for stall detection
            uint64_t delta_nodes = (uint64_t)(latest[2] - prev[2]);
            uint64_t delta_edges = (uint64_t)(latest[3] - prev[3]);
            uint64_t delta_paths = (uint64_t)(latest[4] - prev[4]);
            uint64_t delta_rel_temporal = (uint64_t)(latest[8] - prev[8]);
            uint64_t delta_rel_leap = (uint64_t)(latest[9] - prev[9]);
            
            // Stall detection
            bool stalled = (delta_nodes == 0 && delta_edges == 0 && delta_paths == 0);
            bool runaway = (delta_edges > 50 && latest[17] > 0.40);
            bool locked = (latest[17] < 0.06 && latest[18] < 0.30);
            bool spiky_leap = (delta_rel_leap > (delta_rel_temporal * 2));
            
            cout << "📊 Latest: Nodes=" << formatNumber((uint64_t)latest[2]) 
                 << " Edges=" << formatNumber((uint64_t)latest[3])
                 << " Paths=" << formatNumber((uint64_t)latest[4])
                 << " Health=" << getHealthColor(latest[19]) << fixed << setprecision(1) << latest[19] << resetColor()
                 << "% Entropy=" << fixed << setprecision(3) << latest[17] << endl;
            
            // Composition summary
            cout << "🧠 Composition: Taught=" << formatNumber((uint64_t)latest[5]) 
                 << " Thought=" << formatNumber((uint64_t)latest[6])
                 << " | Exact=" << formatNumber((uint64_t)latest[7])
                 << " Temporal=" << formatNumber((uint64_t)latest[8])
                 << " Leap=" << formatNumber((uint64_t)latest[9]) << endl;
            
            cout << "📏 Paths: Avg=" << fixed << setprecision(1) << latest[12]
                 << " Max=" << latest[13]
                 << " | E2N=" << fixed << setprecision(2) << latest[15]
                 << " Rate=" << latest[14] << "/ms" << endl;
            
            // Stall alerts
            if (stalled) {
                cout << "⚠️ STALLED: No growth detected — check input or increase exploration" << endl;
            }
            if (runaway) {
                cout << "⚠️ RUNAWAY: Excessive edge growth (entropy " << fixed << setprecision(3) << latest[17] 
                     << ") — consider lowering fanout_k or raising leap_threshold" << endl;
            }
            if (locked) {
                cout << "🔒 LOCKED: Mode lock detected — consider increasing temperature or top_p" << endl;
            }
            if (spiky_leap) {
                cout << "🎯 SPIKY: Excessive leap edges — consider raising leap_threshold" << endl;
            }
        }
        
        cout << "💡 Legend: # = relative scale; Δ = change vs previous; Health bands: GREEN≥80% YELLOW≥60% RED<60%" << endl;
        cout << "🔄 Auto-refresh every 350ms | Press Ctrl+C to exit" << endl;
    }
    
    return 0;
}
