#pragma once

#include "Genome.hpp"
#include "Evolution.hpp"
#include <string>
#include <fstream>
#include <vector>
#include <chrono>
#include <memory>

namespace melvin::evolution {

/**
 * Evolution Logging System
 * 
 * Handles CSV/JSON logging, visualization data, and analysis
 */

class EvolutionLogger {
public:
    struct LogConfig {
        std::string output_directory = "evolution_logs/";
        std::string population_csv = "population.csv";
        std::string generation_csv = "generations.csv";
        std::string best_genome_json = "best_genome.json";
        std::string evolution_summary = "evolution_summary.txt";
        
        bool log_population_details = true;
        bool log_generation_summary = true;
        bool log_best_genome = true;
        bool log_parameter_trajectories = true;
        
        // Logging frequency
        int population_log_frequency = 1;    // Every N generations
        int parameter_log_frequency = 5;     // Every N generations
    };
    
private:
    LogConfig config_;
    std::string session_id_;
    std::chrono::steady_clock::time_point start_time_;
    
    // File streams
    std::ofstream population_csv_;
    std::ofstream generation_csv_;
    std::ofstream summary_file_;
    
    // Logging state
    bool initialized_ = false;
    int generation_count_ = 0;
    
    // Statistics tracking
    struct GenerationStats {
        int generation;
        double best_fitness;
        double avg_fitness;
        double worst_fitness;
        double fitness_std;
        double success_rate;
        double global_eta;
        double diversity;
        std::chrono::milliseconds elapsed_time;
    };
    
    std::vector<GenerationStats> generation_history_;
    
public:
    explicit EvolutionLogger(const LogConfig& config);
    ~EvolutionLogger();
    
    // Initialization and cleanup
    bool initialize(const std::string& session_name = "");
    void cleanup();
    
    // Generation logging
    void logGeneration(const std::vector<Genome>& population, 
                      const EvolutionEngine& engine);
    void logGenerationSummary(const std::vector<Genome>& population,
                             const EvolutionEngine& engine);
    
    // Individual genome logging
    void logBestGenome(const Genome& genome, const std::string& filename = "");
    void logPopulationSnapshot(const std::vector<Genome>& population);
    
    // Parameter trajectory logging
    void logParameterTrajectories(const std::vector<Genome>& population);
    
    // Evolution summary
    void generateEvolutionSummary();
    void exportParameterAnalysis();
    
    // Utility functions
    std::string generateSessionId();
    std::string getTimestamp();
    void createOutputDirectory();
    
    // Statistics calculation
    GenerationStats calculateGenerationStats(const std::vector<Genome>& population,
                                           const EvolutionEngine& engine);
    double calculatePopulationDiversity(const std::vector<Genome>& population);
    
    // Configuration
    void setConfig(const LogConfig& config) { config_ = config; }
    const LogConfig& getConfig() const { return config_; }
    
    // Data export
    void exportToCSV(const std::string& filename, const std::vector<Genome>& population);
    void exportToJSON(const std::string& filename, const std::vector<Genome>& population);
    void exportParameterEvolution(const std::string& filename);
    
private:
    // Helper functions
    void writeCSVHeader(std::ofstream& file);
    void writeGenerationHeader();
    void writePopulationHeader();
    
    // File management
    bool openFiles();
    void closeFiles();
    std::string getFilePath(const std::string& filename);
    
    // Data formatting
    std::string formatGenomeCSV(const Genome& genome);
    std::string formatGenerationCSV(const GenerationStats& stats);
    std::string formatParameterTrajectory(const std::vector<Genome>& population, int param_index);
};

/**
 * Visualization data exporter
 */
class VisualizationExporter {
public:
    struct PlotConfig {
        std::string output_format = "png";  // png, svg, pdf
        int width = 1200;
        int height = 800;
        std::string style = "seaborn";       // matplotlib style
        bool show_grid = true;
        bool show_legend = true;
    };
    
private:
    PlotConfig config_;
    std::string output_directory_;
    
public:
    explicit VisualizationExporter(const std::string& output_dir = "evolution_plots/");
    
    // Fitness evolution plots
    void plotFitnessEvolution(const std::string& log_file, const std::string& output_file = "");
    void plotParameterEvolution(const std::string& log_file, const std::string& output_file = "");
    void plotDiversityEvolution(const std::string& log_file, const std::string& output_file = "");
    
    // Population analysis plots
    void plotFitnessDistribution(const std::vector<Genome>& population, const std::string& output_file = "");
    void plotParameterCorrelations(const std::vector<Genome>& population, const std::string& output_file = "");
    void plotGenomeSimilarityMatrix(const std::vector<Genome>& population, const std::string& output_file = "");
    
    // Statistical analysis plots
    void plotParameterHistograms(const std::vector<Genome>& population, const std::string& output_file = "");
    void plotFitnessLandscape(const std::vector<Genome>& population, const std::string& output_file = "");
    
    // Configuration
    void setConfig(const PlotConfig& config) { config_ = config; }
    const PlotConfig& getConfig() const { return config_; }
    
private:
    // Helper functions for data processing
    std::vector<double> extractFitnessData(const std::string& log_file);
    std::vector<std::vector<double>> extractParameterData(const std::string& log_file);
    std::vector<double> extractDiversityData(const std::string& log_file);
    
    // Python script generation
    std::string generatePythonScript(const std::string& plot_type, const std::string& data_file);
    bool executePythonScript(const std::string& script_content, const std::string& output_file);
};

/**
 * Evolution analysis and reporting
 */
class EvolutionAnalyzer {
public:
    struct AnalysisResult {
        double convergence_generation = -1;
        double final_fitness = 0.0;
        double fitness_improvement = 0.0;
        double parameter_stability = 0.0;
        double diversity_trend = 0.0;
        
        std::vector<int> best_performing_parameters;
        std::vector<int> worst_performing_parameters;
        
        std::string convergence_analysis;
        std::string parameter_analysis;
        std::string recommendation;
    };
    
private:
    std::vector<Genome> evolution_history_;
    std::vector<double> generation_fitness_;
    std::vector<double> generation_diversity_;
    
public:
    EvolutionAnalyzer() = default;
    
    // Analysis functions
    AnalysisResult analyzeEvolution(const std::string& log_file);
    AnalysisResult analyzePopulation(const std::vector<Genome>& population);
    
    // Convergence analysis
    bool hasConverged(const std::vector<double>& fitness_history, double threshold = 1e-6);
    int findConvergenceGeneration(const std::vector<double>& fitness_history);
    
    // Parameter analysis
    std::vector<int> findBestParameters(const std::vector<Genome>& population);
    std::vector<int> findWorstParameters(const std::vector<Genome>& population);
    double calculateParameterStability(const std::vector<Genome>& population);
    
    // Diversity analysis
    double calculateDiversityTrend(const std::vector<double>& diversity_history);
    double calculateCurrentDiversity(const std::vector<Genome>& population);
    
    // Reporting
    std::string generateAnalysisReport(const AnalysisResult& result);
    void exportAnalysis(const AnalysisResult& result, const std::string& filename);
    
private:
    // Helper functions
    void loadEvolutionHistory(const std::string& log_file);
    std::vector<double> calculateParameterVariance(const std::vector<Genome>& population);
    double calculateGenomeSimilarity(const Genome& g1, const Genome& g2);
    
    // Statistical functions
    double calculateMean(const std::vector<double>& data);
    double calculateStd(const std::vector<double>& data);
    double calculateCorrelation(const std::vector<double>& x, const std::vector<double>& y);
};

} // namespace melvin::evolution
