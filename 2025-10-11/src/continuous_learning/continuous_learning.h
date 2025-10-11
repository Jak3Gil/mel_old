#pragma once
#include <string>
#include <functional>
#include <cstdint>

namespace melvin {
namespace cl {

struct CLConfig {
    std::string inbox_dir;
    std::string processed_dir;
    std::string failed_dir;
    std::string metrics_csv;
    int poll_seconds = 3;
    int snapshot_every_seconds = 60;
    int metrics_every_seconds  = 10;
    int max_files_per_tick     = 4;
    bool enable_decay = false;
    bool enable_srs   = false;
};

struct CLMetrics {
    uint64_t tick = 0;
    uint64_t files_seen = 0;
    uint64_t files_ok = 0;
    uint64_t files_failed = 0;
    uint64_t nodes = 0;
    uint64_t edges = 0;
};

class ContinuousLearner {
public:
    using TeachFn  = std::function<bool(const std::string& tch_path, std::string& err)>;
    using SnapshotFn = std::function<bool(std::string& out_path, std::string& err)>;
    using GetCountsFn = std::function<void(uint64_t& nodes, uint64_t& edges)>;
    using DecayFn = std::function<void()>;
    using SRSFn   = std::function<void()>;

    ContinuousLearner(CLConfig cfg, TeachFn teach, SnapshotFn snap, GetCountsFn counts,
                      DecayFn decay = {}, SRSFn srs = {});
    void run();  // blocking loop
    
private:
    bool process_one_batch();
    void maybe_snapshot();
    void maybe_metrics();
    void write_metrics_csv_header_if_needed();
    void write_metrics_row();
    void safe_move(const std::string& src, const std::string& dst_dir);

    CLConfig cfg_;
    TeachFn teach_;
    SnapshotFn snap_;
    GetCountsFn counts_;
    DecayFn decay_;
    SRSFn srs_;
    CLMetrics m_;
    long last_snapshot_s_ = 0;
    long last_metrics_s_  = 0;
};

} // namespace cl
} // namespace melvin

