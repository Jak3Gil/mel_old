/*
 * MELVIN BIGRAM CONNECTOR CACHE
 * 
 * O(1)-ish lookups for connector-based queries:
 * - Right expansions: C -> topK {(Y, support, distinct_subjects)}
 * - Left expansions:  C -> topK {(X, support, distinct_subjects)}
 * 
 * Updated on EXACT edge creation and LEAP promotion
 */

#pragma once
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <chrono>
#include <cstdlib>

namespace melvin_bigram_cache {

// ==================== CONFIGURATION ====================

// Configurable via environment
inline int get_cache_topk() {
    const char* env = std::getenv("CACHE_TOPK");
    return env ? std::atoi(env) : 128;
}

inline int get_cache_ttl() {
    const char* env = std::getenv("CACHE_TTL_S");
    return env ? std::atoi(env) : 300;
}

// ==================== DATA STRUCTURES ====================

struct ConnStats {
    int node = -1;
    float support = 0.0f;
    int distinct = 0;
    
    ConnStats() = default;
    ConnStats(int n, float s, int d) : node(n), support(s), distinct(d) {}
    
    bool operator<(const ConnStats& other) const {
        return support > other.support;  // Descending
    }
};

struct CacheEntry {
    std::vector<ConnStats> right_expansions;  // C -> Y
    std::vector<ConnStats> left_expansions;   // X -> C
    uint64_t last_update = 0;
    bool dirty = true;
    
    CacheEntry() {
        using namespace std::chrono;
        last_update = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    }
};

// ==================== CACHE STORAGE ====================

// Per-connector cache: connector_id -> CacheEntry
static std::unordered_map<int, CacheEntry> connector_cache;

// Statistics
struct CacheStats {
    uint64_t hits = 0;
    uint64_t misses = 0;
    uint64_t refreshes = 0;
    uint64_t insertions = 0;
};

static CacheStats cache_stats;

// ==================== CACHE OPERATIONS ====================

// Mark connector as dirty (needs rebuild)
inline void cache_invalidate(int connector) {
    auto it = connector_cache.find(connector);
    if (it != connector_cache.end()) {
        it->second.dirty = true;
    }
}

// Update cache on new EXACT edge
inline void cache_on_exact(int connector, int left, int right, float weight) {
    cache_invalidate(connector);
    cache_stats.insertions++;
}

// Update cache on LEAP promotion
inline void cache_on_promotion(int connector, int left, int right, float weight) {
    cache_invalidate(connector);
    cache_stats.insertions++;
}

// Check if cache is stale
inline bool cache_is_stale(const CacheEntry& entry) {
    using namespace std::chrono;
    uint64_t now = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    uint64_t age = now - entry.last_update;
    return age > (uint64_t)get_cache_ttl();
}

// Rebuild cache entry from graph
// NOTE: This needs access to the graph structure, so we'll provide a callback
using GraphScanCallback = std::function<std::vector<ConnStats>(int connector, bool right_dir)>;
static GraphScanCallback graph_scanner = nullptr;

inline void set_graph_scanner(GraphScanCallback callback) {
    graph_scanner = callback;
}

inline void cache_rebuild(int connector) {
    if (!graph_scanner) return;
    
    auto& entry = connector_cache[connector];
    
    // Scan graph for expansions
    entry.right_expansions = graph_scanner(connector, true);   // C -> Y
    entry.left_expansions = graph_scanner(connector, false);   // X -> C
    
    // Sort and trim to topK
    int topk = get_cache_topk();
    
    std::sort(entry.right_expansions.begin(), entry.right_expansions.end());
    if (entry.right_expansions.size() > (size_t)topk) {
        entry.right_expansions.resize(topk);
    }
    
    std::sort(entry.left_expansions.begin(), entry.left_expansions.end());
    if (entry.left_expansions.size() > (size_t)topk) {
        entry.left_expansions.resize(topk);
    }
    
    using namespace std::chrono;
    entry.last_update = duration_cast<seconds>(system_clock::now().time_since_epoch()).count();
    entry.dirty = false;
    
    cache_stats.refreshes++;
}

// Get right expansions (C -> Y)
inline std::vector<ConnStats> cache_get_right(int connector) {
    auto it = connector_cache.find(connector);
    
    if (it == connector_cache.end() || it->second.dirty || cache_is_stale(it->second)) {
        cache_stats.misses++;
        cache_rebuild(connector);
        it = connector_cache.find(connector);
        if (it == connector_cache.end()) {
            return {};
        }
    } else {
        cache_stats.hits++;
    }
    
    return it->second.right_expansions;
}

// Get left expansions (X -> C)
inline std::vector<ConnStats> cache_get_left(int connector) {
    auto it = connector_cache.find(connector);
    
    if (it == connector_cache.end() || it->second.dirty || cache_is_stale(it->second)) {
        cache_stats.misses++;
        cache_rebuild(connector);
        it = connector_cache.find(connector);
        if (it == connector_cache.end()) {
            return {};
        }
    } else {
        cache_stats.hits++;
    }
    
    return it->second.left_expansions;
}

// Force refresh if needed
inline void cache_maybe_refresh(int connector) {
    auto it = connector_cache.find(connector);
    if (it == connector_cache.end() || it->second.dirty || cache_is_stale(it->second)) {
        cache_rebuild(connector);
    }
}

// Clear all cache
inline void cache_clear() {
    connector_cache.clear();
}

// Get statistics
inline const CacheStats& cache_get_stats() {
    return cache_stats;
}

inline void cache_reset_stats() {
    cache_stats = CacheStats();
}

inline void cache_print_stats(std::ostream& os) {
    os << "📊 Bigram Cache Statistics:\n";
    os << "   Hits:      " << cache_stats.hits << "\n";
    os << "   Misses:    " << cache_stats.misses << "\n";
    os << "   Refreshes: " << cache_stats.refreshes << "\n";
    os << "   Insertions:" << cache_stats.insertions << "\n";
    
    if (cache_stats.hits + cache_stats.misses > 0) {
        float hit_rate = 100.0f * cache_stats.hits / (cache_stats.hits + cache_stats.misses);
        os << "   Hit Rate:  " << std::fixed << std::setprecision(1) << hit_rate << "%\n";
    }
    
    os << "   Cached Connectors: " << connector_cache.size() << "\n";
}

} // namespace melvin_bigram_cache

