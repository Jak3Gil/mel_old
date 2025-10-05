#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <json/json.h>

namespace Melvin::Evaluation {

// Forward declarations
struct PredictionResult;
struct GroundTruth;
struct EvaluationMetrics;

/**
 * A/B Testing Framework for Component Ablations
 * Proves each upgrade helps by toggling components on/off
 */
class AblationTesting {
public:
    explicit AblationTesting(const std::string& output_dir = "reports");
    ~AblationTesting() = default;
    
    // Ablation configuration
    struct AblationConfig {
        std::string name;
        std::map<std::string, bool> flags;
        std::string description;
        int num_runs = 3;  // Multiple runs for statistical significance
    };
    
    // Component flags for ablation
    struct ComponentFlags {
        bool soft_traversal = true;
        bool prob_output = true;
        bool embeddings = true;
        bool dual_state = true;
        bool context_buffer = true;
        bool attention_mechanism = true;
        bool reasoning_engine = true;
        bool learning_engine = true;
    };
    
    // Ablation results
    struct AblationResult {
        std::string config_name;
        ComponentFlags flags;
        std::map<std::string, double> metrics;
        std::map<std::string, double> metric_stds;
        int num_runs;
        double p_value = -1.0;  // Statistical significance
        bool is_significant = false;
    };
    
    // Core ablation interface
    void runAblation(const std::string& dataset_name,
                    const std::vector<GroundTruth>& ground_truth,
                    const std::vector<AblationConfig>& configs);
    
    void runSingleAblation(const std::string& dataset_name,
                          const std::vector<GroundTruth>& ground_truth,
                          const AblationConfig& config);
    
    // Predefined ablation suites
    std::vector<AblationConfig> getCoreAblationSuite();
    std::vector<AblationConfig> getReasoningAblationSuite();
    std::vector<AblationConfig> getPerformanceAblationSuite();
    
    // Statistical analysis
    double calculatePValue(const std::vector<double>& baseline_metrics,
                          const std::vector<double>& treatment_metrics);
    bool isStatisticallySignificant(const std::vector<double>& baseline_metrics,
                                   const std::vector<double>& treatment_metrics,
                                   double alpha = 0.05);
    
    // Results analysis
    std::map<std::string, AblationResult> getResults() const { return results_; }
    void generateAblationReport(const std::string& output_file = "reports/ablations.md");
    Json::Value exportResults() const;
    
    // Component impact analysis
    struct ComponentImpact {
        std::string component;
        double accuracy_delta;
        double latency_delta;
        double entropy_delta;
        bool significant;
        double effect_size;
    };
    
    std::vector<ComponentImpact> analyzeComponentImpact(const std::string& dataset_name) const;
    
private:
    std::string output_dir_;
    std::map<std::string, std::vector<AblationResult>> results_;
    
    // Helper functions
    PredictionResult runWithFlags(const GroundTruth& item, const ComponentFlags& flags);
    std::vector<PredictionResult> runBatchWithFlags(const std::vector<GroundTruth>& items,
                                                  const ComponentFlags& flags);
    
    // Statistical helpers
    double calculateEffectSize(const std::vector<double>& baseline,
                             const std::vector<double>& treatment);
    std::vector<double> bootstrapSample(const std::vector<double>& data, int n_samples = 1000);
    double calculateConfidenceInterval(const std::vector<double>& data, double confidence = 0.95);
    
    // Result aggregation
    std::map<std::string, double> aggregateMetrics(const std::vector<std::vector<PredictionResult>>& runs);
    std::map<std::string, double> calculateMetricStds(const std::vector<std::vector<PredictionResult>>& runs);
    
    // Markdown report generation
    std::string generateMarkdownTable(const std::string& dataset_name) const;
    std::string generateComponentAnalysis(const std::string& dataset_name) const;
};

/**
 * Robustness Testing Framework
 * Tests model resilience to perturbations
 */
class RobustnessTesting {
public:
    explicit RobustnessTesting(const std::string& output_dir = "reports");
    ~RobustnessTesting() = default;
    
    // Robustness test types
    enum class TestType {
        PARAPHRASE,    // Synonym swap & passive/active flip
        NOISE,         // Inject 1-2 typos per query
        DISTRACTOR,    // Extra irrelevant sentence
        NEGATION,      // Add negation to questions
        CONTEXT_SHIFT, // Change context slightly
        LENGTH_VARIATION // Vary question length
    };
    
    // Robustness configuration
    struct RobustnessConfig {
        TestType test_type;
        double perturbation_strength = 0.5;
        int num_perturbations = 1;
        int num_runs = 3;
        int seed = 42;
    };
    
    // Robustness results
    struct RobustnessResult {
        TestType test_type;
        double baseline_accuracy;
        double perturbed_accuracy;
        double accuracy_drop;
        double relative_drop;
        bool passes_threshold;
        double threshold = 0.05;  // 5% max drop
        std::vector<std::string> failure_examples;
    };
    
    // Core robustness interface
    RobustnessResult runRobustnessTest(const std::string& dataset_name,
                                     const std::vector<GroundTruth>& ground_truth,
                                     const RobustnessConfig& config);
    
    void runFullRobustnessSuite(const std::string& dataset_name,
                              const std::vector<GroundTruth>& ground_truth);
    
    // Perturbation generators
    GroundTruth applyParaphrase(const GroundTruth& item, double strength = 0.5);
    GroundTruth applyNoise(const GroundTruth& item, int num_typos = 1);
    GroundTruth applyDistractor(const GroundTruth& item, const std::string& distractor);
    GroundTruth applyNegation(const GroundTruth& item);
    GroundTruth applyContextShift(const GroundTruth& item, double shift_amount = 0.3);
    
    // Results analysis
    std::map<TestType, RobustnessResult> getResults() const { return results_; }
    void generateRobustnessReport(const std::string& output_file = "reports/robustness.md");
    
    // Threshold validation
    bool validateRobustnessThresholds(const std::map<TestType, RobustnessResult>& results);
    
private:
    std::string output_dir_;
    std::map<std::string, std::map<TestType, RobustnessResult>> results_;
    
    // Text processing utilities
    std::vector<std::string> getSynonyms(const std::string& word);
    std::string introduceTypos(const std::string& text, int num_typos);
    std::string flipActivePassive(const std::string& text);
    std::string addNegation(const std::string& text);
    std::string addDistractor(const std::string& text, const std::string& distractor);
    
    // Random utilities
    std::mt19937 rng_;
    int randomInt(int min, int max);
    double randomDouble(double min, double max);
    std::string randomChoice(const std::vector<std::string>& options);
};

/**
 * Long-context validation system
 * Tests continuity and persona consistency
 */
class LongContextValidation {
public:
    explicit LongContextValidation(const std::string& output_dir = "reports");
    ~LongContextValidation() = default;
    
    // Threaded conversation structure
    struct ConversationThread {
        std::string thread_id;
        std::vector<GroundTruth> turns;
        std::string persona_anchor;
        std::vector<std::string> context_anchors;
        int max_turns = 16;
    };
    
    // Long-context results
    struct LongContextResult {
        std::string thread_id;
        double context_accuracy;
        double no_context_accuracy;
        double continuity_gain;
        bool passes_continuity_threshold;
        double persona_consistency;
        bool passes_persona_threshold;
        std::vector<std::string> context_anchor_matches;
        double drift_score;
    };
    
    // Core validation interface
    LongContextResult validateThread(const ConversationThread& thread);
    void validateMultipleThreads(const std::vector<ConversationThread>& threads);
    
    // Thread generation
    ConversationThread generateThread(const std::string& dataset_name, 
                                    const std::string& persona_anchor,
                                    int num_turns = 8);
    
    // Continuity analysis
    double calculateContinuityGain(const std::vector<GroundTruth>& turns_with_context,
                                 const std::vector<GroundTruth>& turns_without_context);
    
    // Persona consistency
    double calculatePersonaConsistency(const std::string& persona_anchor,
                                     const std::vector<GroundTruth>& turns);
    
    // Context drift detection
    double calculateContextDrift(const std::vector<std::string>& context_anchors,
                               const std::vector<GroundTruth>& turns);
    
    // Results analysis
    std::vector<LongContextResult> getResults() const { return results_; }
    void generateLongContextReport(const std::string& output_file = "reports/long_context.md");
    
    // Threshold validation
    bool validateLongContextThresholds(const std::vector<LongContextResult>& results);
    
private:
    std::string output_dir_;
    std::vector<LongContextResult> results_;
    
    // Helper functions
    std::vector<GroundTruth> extractContextAnchors(const ConversationThread& thread);
    std::string extractPersonaFromResponse(const GroundTruth& turn);
    double calculateSemanticSimilarity(const std::string& text1, const std::string& text2);
    
    // Context management
    std::string buildContextWindow(const std::vector<GroundTruth>& turns, int window_size = 4);
    std::string extractRelevantContext(const std::string& current_question,
                                     const std::string& context_window);
};

/**
 * Reproducibility harness
 * Creates replay JSON for exact reproduction of failed runs
 */
class ReproducibilityHarness {
public:
    explicit ReproducibilityHarness(const std::string& snapshots_dir = "snapshots");
    ~ReproducibilityHarness() = default;
    
    // Replay configuration
    struct ReplayConfig {
        std::string dataset;
        std::string item_id;
        int seed;
        std::string profile;
        std::map<std::string, bool> flags;
        std::vector<std::string> context_packet_ids;
        std::string graph_snapshot;
        std::string timestamp;
    };
    
    // Snapshot management
    bool createSnapshot(const std::string& snapshot_id, const Json::Value& state);
    bool loadSnapshot(const std::string& snapshot_id, Json::Value& state);
    bool deleteSnapshot(const std::string& snapshot_id);
    
    // Replay generation
    ReplayConfig createReplayConfig(const std::string& dataset,
                                  const std::string& item_id,
                                  const AblationTesting::ComponentFlags& flags,
                                  int seed = 42,
                                  const std::string& profile = "Balanced");
    
    bool saveReplayConfig(const ReplayConfig& config, const std::string& filename);
    ReplayConfig loadReplayConfig(const std::string& filename);
    
    // Replay execution
    PredictionResult replayPrediction(const ReplayConfig& config);
    bool validateReplay(const ReplayConfig& config, const PredictionResult& expected);
    
    // Batch replay
    std::vector<PredictionResult> replayBatch(const std::vector<ReplayConfig>& configs);
    
    // Snapshot utilities
    std::string generateSnapshotId();
    std::vector<std::string> listSnapshots();
    Json::Value getSnapshotMetadata(const std::string& snapshot_id);
    
private:
    std::string snapshots_dir_;
    std::map<std::string, Json::Value> snapshot_cache_;
    
    // Helper functions
    std::string createTimestamp();
    Json::Value serializeState(const std::string& dataset, const std::string& item_id,
                             const AblationTesting::ComponentFlags& flags);
    bool deserializeState(const Json::Value& state, std::string& dataset,
                        std::string& item_id, AblationTesting::ComponentFlags& flags);
    
    // File management
    std::string getSnapshotPath(const std::string& snapshot_id) const;
    std::string getReplayPath(const std::string& replay_id) const;
};

} // namespace Melvin::Evaluation
