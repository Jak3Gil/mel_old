#include "working_memory.h"
#include <algorithm>
#include <sstream>
#include <iostream>
#include <cmath>

namespace melvin::v2::memory {

// ============================================================================
// WORKING MEMORY IMPLEMENTATION
// ============================================================================

WorkingMemory::WorkingMemory(const evolution::Genome& genome)
    : config_(extract_wm_config(genome)), tick_count_(0) {
    
    // Initialize slots
    slots_.resize(config_.slot_count);
    for (auto& slot : slots_) {
        slot.is_active = false;
    }
    
    stats_ = Stats{};
}

WorkingMemory::WorkingMemory(const Config& config)
    : config_(config), tick_count_(0) {
    
    slots_.resize(config_.slot_count);
    for (auto& slot : slots_) {
        slot.is_active = false;
    }
    
    stats_ = Stats{};
}

WorkingMemory::~WorkingMemory() = default;

// ========================================================================
// GATING
// ========================================================================

bool WorkingMemory::try_add(NodeID item, const std::string& tag, 
                            float salience, float plasticity_rate) {
    
    // Check salience threshold (modulated by plasticity)
    float effective_threshold = config_.gating_threshold / plasticity_rate;
    
    if (salience < effective_threshold) {
        return false;  // Below threshold
    }
    
    // Check if item already in WM
    int existing_slot = find_slot_for_item(item);
    if (existing_slot >= 0) {
        // Already in WM - refresh it
        return refresh(item);
    }
    
    // Find empty slot
    int empty_slot = find_empty_slot();
    
    if (empty_slot >= 0) {
        // Use empty slot
        slots_[empty_slot] = WMSlot(item, tag, salience, config_.decay_tau);
        slots_[empty_slot].is_active = true;
        
        stats_.total_gates++;
        
        if (config_.enable_logging) {
            std::cout << "[WM] Gated: " << item << " (tag=" << tag 
                      << ", salience=" << salience << ")" << std::endl;
        }
        
        return true;
    }
    
    // WM full - try to evict lowest precision
    int evict_slot = find_lowest_precision_slot();
    
    if (evict_slot >= 0 && salience > slots_[evict_slot].precision) {
        // Evict and replace
        if (config_.enable_logging) {
            std::cout << "[WM] Evicting: " << slots_[evict_slot].item_ref 
                      << " (precision=" << slots_[evict_slot].precision << ")" << std::endl;
        }
        
        slots_[evict_slot] = WMSlot(item, tag, salience, config_.decay_tau);
        slots_[evict_slot].is_active = true;
        
        stats_.total_evictions++;
        stats_.total_gates++;
        
        return true;
    }
    
    // Couldn't add (WM full with higher-precision items)
    return false;
}

bool WorkingMemory::refresh(NodeID item) {
    int slot_idx = find_slot_for_item(item);
    
    if (slot_idx < 0) {
        return false;  // Not in WM
    }
    
    // Boost precision (rehearsal)
    float boost = config_.refresh_rate;
    slots_[slot_idx].precision = std::min(1.0f, slots_[slot_idx].precision + boost);
    
    // Reset decay timer
    slots_[slot_idx].decay_time = config_.decay_tau;
    slots_[slot_idx].last_updated = get_timestamp_ns();
    
    stats_.total_refreshes++;
    
    if (config_.enable_logging) {
        std::cout << "[WM] Refreshed: " << item 
                  << " (precision=" << slots_[slot_idx].precision << ")" << std::endl;
    }
    
    return true;
}

bool WorkingMemory::remove(NodeID item) {
    int slot_idx = find_slot_for_item(item);
    
    if (slot_idx < 0) {
        return false;
    }
    
    slots_[slot_idx].is_active = false;
    slots_[slot_idx].item_ref = 0;
    
    return true;
}

bool WorkingMemory::remove_slot(size_t slot_idx) {
    if (slot_idx >= slots_.size()) {
        return false;
    }
    
    slots_[slot_idx].is_active = false;
    slots_[slot_idx].item_ref = 0;
    
    return true;
}

void WorkingMemory::clear() {
    for (auto& slot : slots_) {
        slot.is_active = false;
        slot.item_ref = 0;
    }
}

// ========================================================================
// QUERY
// ========================================================================

std::vector<WMSlot> WorkingMemory::get_active_slots() const {
    std::vector<WMSlot> active;
    
    for (const auto& slot : slots_) {
        if (slot.is_active) {
            active.push_back(slot);
        }
    }
    
    // Sort by precision (descending)
    std::sort(active.begin(), active.end(),
        [](const WMSlot& a, const WMSlot& b) {
            return a.precision > b.precision;
        });
    
    return active;
}

std::vector<WMSlot> WorkingMemory::get_slots_by_tag(const std::string& tag) const {
    std::vector<WMSlot> tagged;
    
    for (const auto& slot : slots_) {
        if (slot.is_active && slot.binding_tag == tag) {
            tagged.push_back(slot);
        }
    }
    
    return tagged;
}

bool WorkingMemory::contains(NodeID item) const {
    return find_slot_for_item(item) >= 0;
}

std::optional<WMSlot> WorkingMemory::get_slot(NodeID item) const {
    int idx = find_slot_for_item(item);
    
    if (idx >= 0) {
        return slots_[idx];
    }
    
    return std::nullopt;
}

std::optional<WMSlot> WorkingMemory::get_slot_at(size_t idx) const {
    if (idx >= slots_.size() || !slots_[idx].is_active) {
        return std::nullopt;
    }
    
    return slots_[idx];
}

size_t WorkingMemory::active_count() const {
    size_t count = 0;
    for (const auto& slot : slots_) {
        if (slot.is_active) count++;
    }
    return count;
}

bool WorkingMemory::is_full() const {
    return active_count() >= config_.slot_count;
}

// ========================================================================
// TICK
// ========================================================================

void WorkingMemory::tick(float dt) {
    apply_decay(dt);
    prune_expired();
    tick_count_++;
}

// ========================================================================
// CONFIGURATION
// ========================================================================

void WorkingMemory::set_config(const Config& config) {
    // Update config
    Config old_config = config_;
    config_ = config;
    
    // Resize slots if needed
    if (config_.slot_count != old_config.slot_count) {
        size_t old_size = slots_.size();
        slots_.resize(config_.slot_count);
        
        // Initialize new slots
        for (size_t i = old_size; i < slots_.size(); ++i) {
            slots_[i].is_active = false;
        }
    }
}

WorkingMemory::Config WorkingMemory::get_config() const {
    return config_;
}

void WorkingMemory::set_gating_threshold(float threshold) {
    config_.gating_threshold = std::clamp(threshold, 0.0f, 1.0f);
}

// ========================================================================
// STATISTICS
// ========================================================================

WorkingMemory::Stats WorkingMemory::get_stats() const {
    Stats stats = stats_;
    stats.tick_count = tick_count_;
    
    // Compute averages
    auto active = get_active_slots();
    
    if (!active.empty()) {
        float total_precision = 0.0f;
        for (const auto& slot : active) {
            total_precision += slot.precision;
        }
        stats.avg_precision = total_precision / active.size();
    } else {
        stats.avg_precision = 0.0f;
    }
    
    // Average lifetime computed from decay times
    if (!active.empty()) {
        float total_lifetime = 0.0f;
        for (const auto& slot : active) {
            total_lifetime += (config_.decay_tau - slot.decay_time);
        }
        stats.avg_lifetime = total_lifetime / active.size();
    } else {
        stats.avg_lifetime = 0.0f;
    }
    
    return stats;
}

void WorkingMemory::reset_stats() {
    stats_ = Stats{};
}

// ========================================================================
// PRIVATE HELPERS
// ========================================================================

int WorkingMemory::find_slot_for_item(NodeID item) const {
    for (size_t i = 0; i < slots_.size(); ++i) {
        if (slots_[i].is_active && slots_[i].item_ref == item) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int WorkingMemory::find_empty_slot() const {
    for (size_t i = 0; i < slots_.size(); ++i) {
        if (!slots_[i].is_active) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int WorkingMemory::find_lowest_precision_slot() const {
    int lowest_idx = -1;
    float lowest_precision = 2.0f;  // > 1.0
    
    for (size_t i = 0; i < slots_.size(); ++i) {
        if (slots_[i].is_active && slots_[i].precision < lowest_precision) {
            lowest_precision = slots_[i].precision;
            lowest_idx = static_cast<int>(i);
        }
    }
    
    return lowest_idx;
}

void WorkingMemory::apply_decay(float dt) {
    // Exponential precision decay
    float decay_factor = std::pow(config_.precision_decay_rate, dt);
    
    for (auto& slot : slots_) {
        if (!slot.is_active) continue;
        
        // Decay precision
        slot.precision *= decay_factor;
        
        // Decay time remaining
        slot.decay_time -= dt;
    }
}

void WorkingMemory::prune_expired() {
    for (auto& slot : slots_) {
        if (!slot.is_active) continue;
        
        // Remove if precision too low or time expired
        if (slot.precision < 0.1f || slot.decay_time <= 0.0f) {
            slot.is_active = false;
            slot.item_ref = 0;
            stats_.total_decays++;
            
            if (config_.enable_logging) {
                std::cout << "[WM] Expired: slot (precision=" 
                          << slot.precision << ", time=" << slot.decay_time << ")" << std::endl;
            }
        }
    }
}

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

WorkingMemory::Config extract_wm_config(const evolution::Genome& genome) {
    WorkingMemory::Config config;
    
    auto module = genome.get_module("working_memory");
    if (!module) {
        return config;  // Return defaults
    }
    
    // Extract genes
    if (auto gene = module->get_gene("slot_count"))
        config.slot_count = static_cast<size_t>(gene->value);
    if (auto gene = module->get_gene("decay_tau"))
        config.decay_tau = gene->value;
    if (auto gene = module->get_gene("gating_threshold"))
        config.gating_threshold = gene->value;
    if (auto gene = module->get_gene("precision_decay"))
        config.precision_decay_rate = gene->value;
    if (auto gene = module->get_gene("refresh_rate"))
        config.refresh_rate = gene->value;
    if (auto gene = module->get_gene("binding_strength"))
        config.binding_strength = gene->value;
    
    return config;
}

std::string wm_state_to_string(const std::vector<WMSlot>& slots) {
    std::ostringstream oss;
    oss << "WorkingMemory[" << slots.size() << " active]:\n";
    
    for (size_t i = 0; i < slots.size(); ++i) {
        const auto& slot = slots[i];
        oss << "  Slot " << i << ": " << slot.item_ref 
            << " (" << slot.binding_tag << ") "
            << "prec=" << slot.precision 
            << " decay=" << slot.decay_time << "s\n";
    }
    
    return oss.str();
}

} // namespace melvin::v2::memory

