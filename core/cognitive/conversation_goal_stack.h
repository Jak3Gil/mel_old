/**
 * @file conversation_goal_stack.h
 * @brief DLPFC-inspired conversation goal tracking
 * 
 * Maintains conversation topics and goals across turns,
 * providing context for coherent multi-turn dialogue.
 */

#ifndef MELVIN_CONVERSATION_GOAL_STACK_H
#define MELVIN_CONVERSATION_GOAL_STACK_H

#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace melvin {
namespace cognitive {

/**
 * @brief Conversation goal representation
 */
struct ConversationGoal {
    std::string topic;                  // Inferred topic name
    std::vector<int> relevant_nodes;    // Associated concept nodes
    float importance;                   // Current importance (0-1)
    int turns_since_mention;            // How many turns ago
    float activation_strength;          // How strongly activated
    
    ConversationGoal()
        : importance(0.5f)
        , turns_since_mention(0)
        , activation_strength(0.0f)
    {}
};

/**
 * @brief Conversation goal stack (DLPFC analog)
 * 
 * Tracks conversation topics and maintains context across turns.
 * Goals decay over time but can be re-activated by related input.
 */
class ConversationGoalStack {
public:
    ConversationGoalStack();
    ~ConversationGoalStack() = default;
    
    /**
     * @brief Update from new input
     * 
     * Checks if input relates to existing goals or introduces new topic
     * 
     * @param input_nodes Concepts from current input
     * @param input_text Raw text input (for topic extraction)
     */
    void update_from_input(
        const std::vector<int>& input_nodes,
        const std::string& input_text = ""
    );
    
    /**
     * @brief Get context nodes for response generation
     * 
     * Returns nodes from all active goals to provide conversation context
     * 
     * @param max_nodes Maximum number of context nodes to return
     * @return Context nodes from active goals
     */
    std::vector<int> get_context_nodes(int max_nodes = 20) const;
    
    /**
     * @brief Get current topic
     */
    std::string get_current_topic() const { return current_topic_; }
    
    /**
     * @brief Get all active goals
     */
    const std::vector<ConversationGoal>& get_goals() const { return goal_stack_; }
    
    /**
     * @brief Check if topic is currently active
     */
    bool is_topic_active(const std::string& topic) const;
    
    /**
     * @brief Force a topic to be current
     */
    void set_current_topic(const std::string& topic);
    
    /**
     * @brief Clear old goals
     */
    void prune_old_goals();
    
    /**
     * @brief Reset goal stack
     */
    void clear();
    
    /**
     * @brief Get goal statistics
     */
    struct Stats {
        int active_goals;
        int total_goals_created;
        float avg_importance;
        std::string most_important_topic;
    };
    Stats get_stats() const;
    
    // Genome-configurable parameters
    void set_decay_rate(float rate) { decay_rate_ = rate; }
    void set_reactivation_boost(float boost) { reactivation_boost_ = boost; }
    void set_overlap_threshold(float thresh) { overlap_threshold_ = thresh; }
    void set_max_turns_inactive(int turns) { max_turns_inactive_ = turns; }
    void set_min_importance(float importance) { min_importance_threshold_ = importance; }
    void set_max_context_nodes(int nodes) { max_context_nodes_ = nodes; }
    
private:
    std::vector<ConversationGoal> goal_stack_;
    std::string current_topic_;
    int total_goals_created_;
    
    // Goal management parameters
    float decay_rate_;                // How fast importance decays
    float reactivation_boost_;        // How much to boost when re-mentioned
    float overlap_threshold_;         // Min overlap to reactivate goal
    int max_turns_inactive_;          // Max turns before pruning
    float min_importance_threshold_;   // Min importance to keep
    int max_context_nodes_;           // Max context nodes to return
    
    // Helper methods
    float compute_overlap(
        const std::vector<int>& a, 
        const std::vector<int>& b
    ) const;
    
    std::string infer_topic(
        const std::vector<int>& nodes,
        const std::string& text
    ) const;
    
    int find_matching_goal(const std::vector<int>& nodes);
    
    void decay_all_goals();
    void promote_goal(int goal_index, float overlap);
    void create_new_goal(const std::vector<int>& nodes, const std::string& text);
};

} // namespace cognitive
} // namespace melvin

#endif // MELVIN_CONVERSATION_GOAL_STACK_H

