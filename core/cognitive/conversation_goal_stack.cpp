/**
 * @file conversation_goal_stack.cpp
 * @brief Implementation of conversation goal stack
 */

#include "conversation_goal_stack.h"
#include <algorithm>
#include <sstream>
#include <iostream>

namespace melvin {
namespace cognitive {

ConversationGoalStack::ConversationGoalStack()
    : current_topic_("general")
    , total_goals_created_(0)
    , decay_rate_(0.95f)
    , reactivation_boost_(0.25f)
    , overlap_threshold_(0.25f)
    , max_turns_inactive_(10)
    , min_importance_threshold_(0.15f)
    , max_context_nodes_(20)
{
}

void ConversationGoalStack::update_from_input(
    const std::vector<int>& input_nodes,
    const std::string& input_text
) {
    if (input_nodes.empty()) return;
    
    // Decay all existing goals
    decay_all_goals();
    
    // Check if input relates to existing goals
    int matching_goal = find_matching_goal(input_nodes);
    
    if (matching_goal >= 0) {
        // Re-activate existing goal
        float overlap = compute_overlap(
            input_nodes, 
            goal_stack_[matching_goal].relevant_nodes
        );
        promote_goal(matching_goal, overlap);
        
        current_topic_ = goal_stack_[matching_goal].topic;
        
        std::cout << "🎯 [Goal Stack] Re-activated topic: \"" 
                  << current_topic_ << "\" (overlap: " 
                  << overlap << ")\n";
    } else {
        // New topic introduced
        create_new_goal(input_nodes, input_text);
        
        std::cout << "🎯 [Goal Stack] New topic: \"" 
                  << current_topic_ << "\"\n";
    }
    
    // Prune old goals
    prune_old_goals();
}

std::vector<int> ConversationGoalStack::get_context_nodes(int max_nodes) const {
    std::vector<std::pair<int, float>> node_importance;
    
    // Collect nodes from all active goals, weighted by importance
    for (const auto& goal : goal_stack_) {
        if (goal.importance > min_importance_threshold_) {
            for (int node_id : goal.relevant_nodes) {
                node_importance.push_back({node_id, goal.importance});
            }
        }
    }
    
    // Sort by importance
    std::sort(node_importance.begin(), node_importance.end(),
        [](const auto& a, const auto& b) {
            return a.second > b.second;
        });
    
    // Extract top nodes (deduplicated)
    std::vector<int> context_nodes;
    std::unordered_set<int> seen;
    
    for (const auto& [node_id, importance] : node_importance) {
        if (seen.find(node_id) == seen.end()) {
            context_nodes.push_back(node_id);
            seen.insert(node_id);
            
            if (context_nodes.size() >= (size_t)max_nodes) {
                break;
            }
        }
    }
    
    return context_nodes;
}

bool ConversationGoalStack::is_topic_active(const std::string& topic) const {
    for (const auto& goal : goal_stack_) {
        if (goal.topic == topic && goal.importance > min_importance_threshold_) {
            return true;
        }
    }
    return false;
}

void ConversationGoalStack::set_current_topic(const std::string& topic) {
    current_topic_ = topic;
    
    // Boost importance of matching goal
    for (auto& goal : goal_stack_) {
        if (goal.topic == topic) {
            goal.importance = std::min(1.0f, goal.importance + 0.3f);
            goal.turns_since_mention = 0;
            break;
        }
    }
}

void ConversationGoalStack::prune_old_goals() {
    goal_stack_.erase(
        std::remove_if(goal_stack_.begin(), goal_stack_.end(),
            [this](const ConversationGoal& g) {
                return (g.turns_since_mention > max_turns_inactive_ && 
                        g.importance < min_importance_threshold_);
            }),
        goal_stack_.end()
    );
}

void ConversationGoalStack::clear() {
    goal_stack_.clear();
    current_topic_ = "general";
}

ConversationGoalStack::Stats ConversationGoalStack::get_stats() const {
    Stats stats;
    stats.active_goals = 0;
    stats.total_goals_created = total_goals_created_;
    stats.avg_importance = 0.0f;
    
    float max_importance = 0.0f;
    std::string most_important = "";
    
    for (const auto& goal : goal_stack_) {
        if (goal.importance > min_importance_threshold_) {
            stats.active_goals++;
            stats.avg_importance += goal.importance;
            
            if (goal.importance > max_importance) {
                max_importance = goal.importance;
                most_important = goal.topic;
            }
        }
    }
    
    if (stats.active_goals > 0) {
        stats.avg_importance /= stats.active_goals;
    }
    
    stats.most_important_topic = most_important;
    
    return stats;
}

float ConversationGoalStack::compute_overlap(
    const std::vector<int>& a,
    const std::vector<int>& b
) const {
    if (a.empty() || b.empty()) return 0.0f;
    
    std::unordered_set<int> set_a(a.begin(), a.end());
    int overlap_count = 0;
    
    for (int node : b) {
        if (set_a.find(node) != set_a.end()) {
            overlap_count++;
        }
    }
    
    // Jaccard similarity
    float union_size = set_a.size() + b.size() - overlap_count;
    return (union_size > 0) ? (float)overlap_count / union_size : 0.0f;
}

std::string ConversationGoalStack::infer_topic(
    const std::vector<int>& nodes,
    const std::string& text
) const {
    // Simple topic inference: use first few words or node count
    if (!text.empty()) {
        std::istringstream iss(text);
        std::string first_word;
        iss >> first_word;
        
        // Remove common question words
        if (first_word == "what" || first_word == "how" || 
            first_word == "why" || first_word == "where") {
            iss >> first_word;  // Get next word
        }
        
        if (!first_word.empty()) {
            return first_word;
        }
    }
    
    // Fallback: use node-based topic
    return "topic_" + std::to_string(total_goals_created_ + 1);
}

int ConversationGoalStack::find_matching_goal(const std::vector<int>& nodes) {
    int best_match = -1;
    float best_overlap = 0.0f;
    
    for (size_t i = 0; i < goal_stack_.size(); ++i) {
        float overlap = compute_overlap(nodes, goal_stack_[i].relevant_nodes);
        
        if (overlap > best_overlap && overlap > overlap_threshold_) {
            best_overlap = overlap;
            best_match = i;
        }
    }
    
    return best_match;
}

void ConversationGoalStack::decay_all_goals() {
    for (auto& goal : goal_stack_) {
        goal.turns_since_mention++;
        goal.importance *= decay_rate_;
    }
}

void ConversationGoalStack::promote_goal(int goal_index, float overlap) {
    auto& goal = goal_stack_[goal_index];
    
    // Reset turn counter
    goal.turns_since_mention = 0;
    
    // Boost importance based on overlap
    float boost = reactivation_boost_ * overlap;
    goal.importance = std::min(1.0f, goal.importance + boost);
    
    // Increase activation strength
    goal.activation_strength = overlap;
}

void ConversationGoalStack::create_new_goal(
    const std::vector<int>& nodes,
    const std::string& text
) {
    ConversationGoal new_goal;
    new_goal.topic = infer_topic(nodes, text);
    new_goal.relevant_nodes = nodes;
    new_goal.importance = 0.75f;  // Start with high importance
    new_goal.turns_since_mention = 0;
    new_goal.activation_strength = 1.0f;
    
    goal_stack_.push_back(new_goal);
    current_topic_ = new_goal.topic;
    total_goals_created_++;
}

} // namespace cognitive
} // namespace melvin

