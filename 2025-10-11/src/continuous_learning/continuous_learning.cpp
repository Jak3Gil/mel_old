#include "continuous_learning.h"
#include <filesystem>
#include <fstream>
#include <chrono>
#include <thread>
#include <vector>
#include <algorithm>
#include <cstdio>

using namespace std::chrono;
namespace fs = std::filesystem;

namespace melvin {
namespace cl {

static long now_s() {
    return static_cast<long>(duration_cast<seconds>(system_clock::now().time_since_epoch()).count());
}

ContinuousLearner::ContinuousLearner(CLConfig cfg, TeachFn teach, SnapshotFn snap, GetCountsFn counts,
                                     DecayFn decay, SRSFn srs)
: cfg_(std::move(cfg)), teach_(std::move(teach)), snap_(std::move(snap)), counts_(std::move(counts)),
  decay_(std::move(decay)), srs_(std::move(srs)) {}

void ContinuousLearner::run() {
    fs::create_directories(cfg_.inbox_dir);
    fs::create_directories(cfg_.processed_dir);
    fs::create_directories(cfg_.failed_dir);
    
    // Create logs directory if needed
    if (!cfg_.metrics_csv.empty()) {
        fs::path csv_path(cfg_.metrics_csv);
        if (csv_path.has_parent_path()) {
            fs::create_directories(csv_path.parent_path());
        }
    }

    write_metrics_csv_header_if_needed();
    last_snapshot_s_ = now_s();
    last_metrics_s_  = now_s();

    std::printf("[CL] Continuous Learning Mode Started\n");
    std::printf("[CL] Watching: %s\n", cfg_.inbox_dir.c_str());
    std::printf("[CL] Poll interval: %ds\n", cfg_.poll_seconds);
    std::printf("[CL] Snapshot interval: %ds\n", cfg_.snapshot_every_seconds);
    std::printf("[CL] Metrics interval: %ds\n", cfg_.metrics_every_seconds);
    std::printf("[CL] Press Ctrl+C to stop\n\n");

    while (true) {
        m_.tick++;
        process_one_batch();
        maybe_snapshot();
        maybe_metrics();

        if (cfg_.enable_decay && decay_) decay_();  // no-op if not wired yet
        if (cfg_.enable_srs   && srs_)   srs_();    // no-op if not wired yet

        std::this_thread::sleep_for(std::chrono::seconds(cfg_.poll_seconds));
    }
}

bool ContinuousLearner::process_one_batch() {
    std::vector<fs::directory_entry> items;
    
    // Scan inbox for .tch files
    if (!fs::exists(cfg_.inbox_dir)) {
        return false;
    }
    
    for (auto& e : fs::directory_iterator(cfg_.inbox_dir)) {
        if (e.is_regular_file() && e.path().extension() == ".tch") {
            items.push_back(e);
        }
    }
    
    if (items.empty()) {
        return false;
    }
    
    // Sort by modification time (oldest first)
    std::sort(items.begin(), items.end(),
              [](auto& a, auto& b){ 
                  return a.last_write_time() < b.last_write_time(); 
              });

    int processed = 0;
    for (auto& e : items) {
        if (processed >= cfg_.max_files_per_tick) break;
        
        m_.files_seen++;
        std::string path_str = e.path().string();
        std::string filename = e.path().filename().string();
        
        std::printf("[CL] Processing: %s\n", filename.c_str());
        
        std::string err;
        bool ok = teach_(path_str, err);
        
        if (ok) {
            m_.files_ok++;
            safe_move(path_str, cfg_.processed_dir);
            std::printf("[CL] ✓ Success: %s\n", filename.c_str());
        } else {
            m_.files_failed++;
            safe_move(path_str, cfg_.failed_dir);
            std::fprintf(stderr, "[CL] ✗ Failed: %s - %s\n",
                         filename.c_str(), err.c_str());
        }
        processed++;
    }
    
    if (processed > 0) {
        std::printf("[CL] Batch complete: %d files processed\n\n", processed);
    }
    
    return processed > 0;
}

void ContinuousLearner::maybe_snapshot() {
    if (now_s() - last_snapshot_s_ < cfg_.snapshot_every_seconds) return;
    
    std::string out; 
    std::string err;
    
    if (snap_) {
        if (snap_(out, err)) {
            std::printf("[CL] 📸 Snapshot saved: %s\n", out.c_str());
        } else {
            std::fprintf(stderr, "[CL] ⚠ Snapshot failed: %s\n", err.c_str());
        }
    }
    
    last_snapshot_s_ = now_s();
}

void ContinuousLearner::maybe_metrics() {
    if (now_s() - last_metrics_s_ < cfg_.metrics_every_seconds) return;
    
    if (counts_) {
        counts_(m_.nodes, m_.edges);
    }
    
    write_metrics_row();
    last_metrics_s_ = now_s();
}

void ContinuousLearner::write_metrics_csv_header_if_needed() {
    if (cfg_.metrics_csv.empty()) return;
    if (fs::exists(cfg_.metrics_csv)) return;
    
    std::ofstream f(cfg_.metrics_csv, std::ios::app);
    f << "timestamp,tick,files_seen,files_ok,files_failed,nodes,edges,"
      << "multihop_probe_success,multihop_avg_path_len,"
      << "decay_events,avg_edge_weight,pct_edges_below_0_1,"
      << "srs_due,srs_reviewed,srs_pass_rate,srs_avg_interval,srs_avg_ease\n";
}

void ContinuousLearner::write_metrics_row() {
    if (cfg_.metrics_csv.empty()) return;
    
    std::ofstream f(cfg_.metrics_csv, std::ios::app);
    f << now_s() << "," << m_.tick << "," << m_.files_seen << "," << m_.files_ok << ","
      << m_.files_failed << "," << m_.nodes << "," << m_.edges << ","
      << m_.multihop_probe_success << "," << m_.multihop_avg_path_len << ","
      << m_.decay_events << "," << m_.avg_edge_weight << "," << m_.pct_edges_below_0_1 << ","
      << m_.srs_due << "," << m_.srs_reviewed << "," << m_.srs_pass_rate << ","
      << m_.srs_avg_interval << "," << m_.srs_avg_ease << "\n";
}

void ContinuousLearner::safe_move(const std::string& src, const std::string& dst_dir) {
    fs::path dst = fs::path(dst_dir) / fs::path(src).filename();
    std::error_code ec;
    
    // Try rename first (fast, atomic)
    fs::rename(src, dst, ec);
    
    if (ec) {
        // Fallback to copy+remove on cross-device or permission issues
        fs::copy(src, dst, fs::copy_options::overwrite_existing, ec);
        if (!ec) {
            fs::remove(src);
        } else {
            std::fprintf(stderr, "[CL] Warning: Could not move %s: %s\n",
                         src.c_str(), ec.message().c_str());
        }
    }
}

} // namespace cl
} // namespace melvin

