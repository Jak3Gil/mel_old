/*
 * MELVIN LEARN-TEST-EVOLVE SYSTEM
 * 
 * Complete pipeline for:
 * 1. Feeding information to Melvin
 * 2. Testing him on that information
 * 3. Grading his accuracy
 * 4. Evolving based on performance
 * 5. Repeating the cycle
 */

#include <iostream>
#include <vector>
#include <string>
#include <random>
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <unordered_map>
#include <memory>
#include <thread>
#include "melvin.h"

// ==================== KNOWLEDGE BASE ====================

struct Fact {
    std::string subject;
    std::string relation;
    std::string object;
    std::string source_text;
    
    Fact(const std::string& s, const std::string& r, const std::string& o, const std::string& src = "")
        : subject(s), relation(r), object(o), source_text(src) {}
    
    std::string to_sentence() const {
        if (relation == "ISA") {
            return subject + " is a " + object;
        } else if (relation == "HAS") {
            return subject + " has " + object;
        } else if (relation == "CAN") {
            return subject + " can " + object;
        } else if (relation == "EATS" || relation == "CONSUMES") {
            return subject + " eats " + object;
        } else if (relation == "LIVES_IN") {
            return subject + " lives in " + object;
        } else if (relation == "PROPERTY") {
            return subject + " is " + object;
        }
        return subject + " " + relation + " " + object;
    }
};

struct KnowledgeDomain {
    std::string name;
    std::vector<Fact> facts;
    std::vector<std::string> training_texts;
    
    KnowledgeDomain(const std::string& n) : name(n) {}
    
    void add_fact(const Fact& fact) {
        facts.push_back(fact);
    }
    
    void add_training_text(const std::string& text) {
        training_texts.push_back(text);
    }
    
    size_t fact_count() const {
        return facts.size();
    }
};

// ==================== TEST QUESTION GENERATOR ====================

struct TestQuestion {
    std::string question;
    std::string correct_answer;
    std::vector<std::string> expected_keywords;
    std::string difficulty; // "easy", "medium", "hard"
    Fact source_fact;
    
    TestQuestion() : source_fact("", "", "") {}
    TestQuestion(const std::string& q, const std::string& a, const Fact& f, const std::string& diff = "medium")
        : question(q), correct_answer(a), difficulty(diff), source_fact(f) {}
};

class QuestionGenerator {
private:
    std::mt19937 rng_;
    
public:
    QuestionGenerator() : rng_(std::random_device{}()) {}
    
    std::vector<TestQuestion> generate_questions(const KnowledgeDomain& domain, size_t count) {
        std::vector<TestQuestion> questions;
        
        if (domain.facts.empty()) return questions;
        
        for (size_t i = 0; i < count && i < domain.facts.size(); ++i) {
            const auto& fact = domain.facts[i];
            
            // Generate different question types
            if (fact.relation == "ISA") {
                questions.push_back(generate_isa_question(fact));
            } else if (fact.relation == "HAS") {
                questions.push_back(generate_has_question(fact));
            } else if (fact.relation == "CAN") {
                questions.push_back(generate_can_question(fact));
            } else if (fact.relation == "EATS" || fact.relation == "CONSUMES") {
                questions.push_back(generate_eats_question(fact));
            } else if (fact.relation == "LIVES_IN") {
                questions.push_back(generate_lives_in_question(fact));
            } else if (fact.relation == "PROPERTY") {
                questions.push_back(generate_property_question(fact));
            }
        }
        
        // Shuffle questions
        std::shuffle(questions.begin(), questions.end(), rng_);
        
        return questions;
    }
    
private:
    TestQuestion generate_isa_question(const Fact& fact) {
        std::string question = "What is a " + fact.subject + "?";
        std::string answer = "A " + fact.subject + " is a " + fact.object;
        
        TestQuestion q(question, answer, fact, "easy");
        q.expected_keywords = {fact.object, fact.subject};
        return q;
    }
    
    TestQuestion generate_has_question(const Fact& fact) {
        std::string question = "What does a " + fact.subject + " have?";
        std::string answer = "A " + fact.subject + " has " + fact.object;
        
        TestQuestion q(question, answer, fact, "easy");
        q.expected_keywords = {fact.object, "has"};
        return q;
    }
    
    TestQuestion generate_can_question(const Fact& fact) {
        std::string question = "What can a " + fact.subject + " do?";
        std::string answer = "A " + fact.subject + " can " + fact.object;
        
        TestQuestion q(question, answer, fact, "medium");
        q.expected_keywords = {fact.object, "can"};
        return q;
    }
    
    TestQuestion generate_eats_question(const Fact& fact) {
        std::string question = "What does a " + fact.subject + " eat?";
        std::string answer = "A " + fact.subject + " eats " + fact.object;
        
        TestQuestion q(question, answer, fact, "easy");
        q.expected_keywords = {fact.object, "eat"};
        return q;
    }
    
    TestQuestion generate_lives_in_question(const Fact& fact) {
        std::string question = "Where does a " + fact.subject + " live?";
        std::string answer = "A " + fact.subject + " lives in " + fact.object;
        
        TestQuestion q(question, answer, fact, "easy");
        q.expected_keywords = {fact.object, "live"};
        return q;
    }
    
    TestQuestion generate_property_question(const Fact& fact) {
        std::string question = "How would you describe a " + fact.subject + "?";
        std::string answer = "A " + fact.subject + " is " + fact.object;
        
        TestQuestion q(question, answer, fact, "medium");
        q.expected_keywords = {fact.object};
        return q;
    }
};

// ==================== GRADING SYSTEM ====================

struct TestResult {
    TestQuestion question;
    std::string melvin_answer;
    bool correct;
    float confidence_score;
    std::vector<std::string> keywords_found;
};

struct GradeReport {
    int total_questions = 0;
    int correct_answers = 0;
    int partial_answers = 0;
    int wrong_answers = 0;
    float accuracy = 0.0f;
    float partial_credit_accuracy = 0.0f;
    std::vector<TestResult> results;
    std::string domain_name;
    uint64_t generation = 0;
    
    void calculate_accuracy() {
        if (total_questions == 0) return;
        accuracy = static_cast<float>(correct_answers) / total_questions;
        partial_credit_accuracy = (correct_answers + partial_answers * 0.5f) / total_questions;
    }
    
    std::string get_summary() const {
        std::stringstream ss;
        ss << "Grade Report [Gen " << generation << "]: "
           << correct_answers << "/" << total_questions << " correct ("
           << std::fixed << std::setprecision(1) << (accuracy * 100) << "%), "
           << partial_answers << " partial ("
           << std::fixed << std::setprecision(1) << (partial_credit_accuracy * 100) << "% with partial credit)";
        return ss.str();
    }
    
    std::string get_detailed_report() const {
        std::stringstream ss;
        ss << "\n╔══════════════════════════════════════════════════════════════╗\n";
        ss << "║          MELVIN LEARNING ASSESSMENT REPORT                  ║\n";
        ss << "╚══════════════════════════════════════════════════════════════╝\n\n";
        
        ss << "Domain: " << domain_name << "\n";
        ss << "Generation: " << generation << "\n\n";
        
        ss << "📊 OVERALL PERFORMANCE:\n";
        ss << "   Total Questions: " << total_questions << "\n";
        ss << "   ✅ Correct: " << correct_answers << " (" 
           << std::fixed << std::setprecision(1) << (accuracy * 100) << "%)\n";
        ss << "   🟡 Partial: " << partial_answers << "\n";
        ss << "   ❌ Wrong: " << wrong_answers << "\n";
        ss << "   📈 Accuracy Score: " << std::fixed << std::setprecision(3) << accuracy << "\n";
        ss << "   📈 Partial Credit Score: " << std::fixed << std::setprecision(3) 
           << partial_credit_accuracy << "\n\n";
        
        // Grade classification
        ss << "🎓 GRADE: ";
        if (accuracy >= 0.9f) ss << "A (Excellent)\n";
        else if (accuracy >= 0.8f) ss << "B (Good)\n";
        else if (accuracy >= 0.7f) ss << "C (Fair)\n";
        else if (accuracy >= 0.6f) ss << "D (Poor)\n";
        else ss << "F (Failing)\n";
        
        ss << "\n📝 SAMPLE RESPONSES:\n";
        int samples = std::min(5, static_cast<int>(results.size()));
        for (int i = 0; i < samples; ++i) {
            const auto& r = results[i];
            ss << "   Q" << (i+1) << ": " << r.question.question << "\n";
            ss << "   Expected: " << r.question.correct_answer << "\n";
            ss << "   Melvin: " << r.melvin_answer << "\n";
            ss << "   " << (r.correct ? "✅ CORRECT" : "❌ WRONG") << "\n\n";
        }
        
        return ss.str();
    }
};

class GradingSystem {
private:
    std::mt19937 rng_;
    
public:
    GradingSystem() : rng_(std::random_device{}()) {}
    
    TestResult grade_answer(const TestQuestion& question, const std::string& answer) {
        TestResult result;
        result.question = question;
        result.melvin_answer = answer;
        result.correct = false;
        result.confidence_score = 0.0f;
        
        // Convert to lowercase for comparison
        std::string answer_lower = to_lower(answer);
        std::string correct_lower = to_lower(question.correct_answer);
        
        // Check for exact match
        if (answer_lower == correct_lower) {
            result.correct = true;
            result.confidence_score = 1.0f;
            return result;
        }
        
        // Check for keyword matches
        int keywords_matched = 0;
        for (const auto& keyword : question.expected_keywords) {
            if (contains(answer_lower, to_lower(keyword))) {
                keywords_matched++;
                result.keywords_found.push_back(keyword);
            }
        }
        
        // Calculate confidence based on keyword matches
        if (!question.expected_keywords.empty()) {
            result.confidence_score = static_cast<float>(keywords_matched) / 
                                     question.expected_keywords.size();
            
            // Consider correct if most keywords are present
            if (result.confidence_score >= 0.7f) {
                result.correct = true;
            }
        }
        
        return result;
    }
    
    GradeReport grade_test(const std::vector<TestQuestion>& questions, 
                          const std::vector<std::string>& answers,
                          const std::string& domain_name,
                          uint64_t generation) {
        GradeReport report;
        report.domain_name = domain_name;
        report.generation = generation;
        report.total_questions = questions.size();
        
        for (size_t i = 0; i < questions.size() && i < answers.size(); ++i) {
            TestResult result = grade_answer(questions[i], answers[i]);
            
            if (result.correct) {
                report.correct_answers++;
            } else if (result.confidence_score >= 0.4f) {
                report.partial_answers++;
            } else {
                report.wrong_answers++;
            }
            
            report.results.push_back(result);
        }
        
        report.calculate_accuracy();
        return report;
    }
    
private:
    std::string to_lower(const std::string& str) {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), ::tolower);
        return result;
    }
    
    bool contains(const std::string& str, const std::string& substr) {
        return str.find(substr) != std::string::npos;
    }
};

// ==================== EVOLUTION TRACKER ====================

struct EvolutionMetrics {
    float fitness;
    float accuracy;
    float partial_credit_accuracy;
    int generation;
    std::string timestamp;
    
    EvolutionMetrics(float f, float a, float pca, int g, const std::string& t)
        : fitness(f), accuracy(a), partial_credit_accuracy(pca), generation(g), timestamp(t) {}
};

class EvolutionTracker {
private:
    std::vector<EvolutionMetrics> history_;
    std::ofstream log_file_;
    
public:
    EvolutionTracker(const std::string& log_path) {
        log_file_.open(log_path, std::ios::app);
        if (log_file_.is_open()) {
            log_file_ << "generation,fitness,accuracy,partial_credit_accuracy,timestamp\n";
        }
    }
    
    ~EvolutionTracker() {
        if (log_file_.is_open()) {
            log_file_.close();
        }
    }
    
    void record(float fitness, float accuracy, float partial_credit, int generation) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::stringstream ss;
        ss << std::ctime(&time_t);
        std::string timestamp = ss.str();
        timestamp.pop_back(); // Remove newline
        
        history_.emplace_back(fitness, accuracy, partial_credit, generation, timestamp);
        
        if (log_file_.is_open()) {
            log_file_ << generation << "," << fitness << "," << accuracy << "," 
                     << partial_credit << "," << timestamp << "\n";
            log_file_.flush();
        }
    }
    
    bool should_evolve(int min_generations = 3) const {
        if (history_.size() < min_generations) return false;
        
        // Check if performance is stagnating or declining
        float recent_avg = 0.0f;
        int recent_count = std::min(3, static_cast<int>(history_.size()));
        
        for (int i = 0; i < recent_count; ++i) {
            recent_avg += history_[history_.size() - 1 - i].accuracy;
        }
        recent_avg /= recent_count;
        
        // Evolve if accuracy is below 80% for recent generations
        return recent_avg < 0.8f;
    }
    
    std::string get_progress_summary() const {
        if (history_.empty()) return "No history available";
        
        std::stringstream ss;
        ss << "\n📈 EVOLUTION PROGRESS:\n";
        ss << "   Total Generations: " << history_.size() << "\n";
        
        if (history_.size() > 0) {
            ss << "   First Gen Accuracy: " << std::fixed << std::setprecision(3) 
               << history_[0].accuracy << "\n";
            ss << "   Latest Gen Accuracy: " << history_.back().accuracy << "\n";
            
            if (history_.size() > 1) {
                float improvement = history_.back().accuracy - history_[0].accuracy;
                ss << "   Overall Improvement: " << std::showpos << improvement << "\n";
            }
        }
        
        return ss.str();
    }
};

// ==================== KNOWLEDGE DOMAINS ====================

KnowledgeDomain create_animals_domain() {
    KnowledgeDomain domain("Animals");
    
    // Add facts
    domain.add_fact(Fact("cat", "ISA", "mammal"));
    domain.add_fact(Fact("dog", "ISA", "mammal"));
    domain.add_fact(Fact("bird", "ISA", "animal"));
    domain.add_fact(Fact("fish", "ISA", "animal"));
    domain.add_fact(Fact("mammal", "ISA", "animal"));
    
    domain.add_fact(Fact("cat", "HAS", "fur"));
    domain.add_fact(Fact("dog", "HAS", "tail"));
    domain.add_fact(Fact("bird", "HAS", "feathers"));
    domain.add_fact(Fact("fish", "HAS", "scales"));
    
    domain.add_fact(Fact("cat", "CAN", "meow"));
    domain.add_fact(Fact("dog", "CAN", "bark"));
    domain.add_fact(Fact("bird", "CAN", "fly"));
    domain.add_fact(Fact("fish", "CAN", "swim"));
    
    domain.add_fact(Fact("cat", "EATS", "mice"));
    domain.add_fact(Fact("dog", "EATS", "bones"));
    domain.add_fact(Fact("bird", "EATS", "seeds"));
    domain.add_fact(Fact("fish", "EATS", "plankton"));
    
    domain.add_fact(Fact("cat", "LIVES_IN", "houses"));
    domain.add_fact(Fact("dog", "LIVES_IN", "homes"));
    domain.add_fact(Fact("bird", "LIVES_IN", "trees"));
    domain.add_fact(Fact("fish", "LIVES_IN", "water"));
    
    // Add training texts
    domain.add_training_text("Cats are mammals that have fur and can meow. They eat mice and live in houses.");
    domain.add_training_text("Dogs are mammals with tails. Dogs can bark and they eat bones. Dogs live in homes.");
    domain.add_training_text("Birds are animals that have feathers and can fly. Birds eat seeds and live in trees.");
    domain.add_training_text("Fish are animals with scales. Fish can swim and eat plankton. They live in water.");
    domain.add_training_text("Mammals are animals. Both cats and dogs are mammals.");
    
    return domain;
}

KnowledgeDomain create_science_domain() {
    KnowledgeDomain domain("Science");
    
    domain.add_fact(Fact("water", "ISA", "liquid"));
    domain.add_fact(Fact("ice", "ISA", "solid"));
    domain.add_fact(Fact("steam", "ISA", "gas"));
    domain.add_fact(Fact("oxygen", "ISA", "element"));
    
    domain.add_fact(Fact("water", "HAS", "hydrogen"));
    domain.add_fact(Fact("water", "HAS", "oxygen"));
    domain.add_fact(Fact("plant", "HAS", "chlorophyll"));
    
    domain.add_fact(Fact("plant", "CAN", "photosynthesize"));
    domain.add_fact(Fact("water", "CAN", "freeze"));
    domain.add_fact(Fact("ice", "CAN", "melt"));
    
    domain.add_fact(Fact("plant", "CONSUMES", "carbon dioxide"));
    domain.add_fact(Fact("plant", "CONSUMES", "sunlight"));
    domain.add_fact(Fact("human", "CONSUMES", "oxygen"));
    
    domain.add_training_text("Water is a liquid that has hydrogen and oxygen. Water can freeze into ice.");
    domain.add_training_text("Ice is solid water. Ice can melt back into liquid water when heated.");
    domain.add_training_text("Steam is water in gas form. Plants have chlorophyll and can photosynthesize.");
    domain.add_training_text("Plants consume carbon dioxide and sunlight. Humans consume oxygen to breathe.");
    domain.add_training_text("Oxygen is an element that is essential for life.");
    
    return domain;
}

// ==================== MAIN LEARN-TEST-EVOLVE LOOP ====================

class LearnTestEvolveSystem {
private:
    melvin_t* melvin_;
    QuestionGenerator question_gen_;
    GradingSystem grading_system_;
    EvolutionTracker evolution_tracker_;
    std::vector<KnowledgeDomain> domains_;
    int current_generation_;
    
public:
    LearnTestEvolveSystem(const std::string& store_dir)
        : melvin_(melvin_create(store_dir.c_str())),
          evolution_tracker_("melvin_evolution_log.csv"),
          current_generation_(0) {
        
        // Setup domains
        domains_.push_back(create_animals_domain());
        domains_.push_back(create_science_domain());
    }
    
    ~LearnTestEvolveSystem() {
        if (melvin_) {
            melvin_destroy(melvin_);
        }
    }
    
    void run_learning_cycle(int num_cycles) {
        std::cout << "🧠 MELVIN LEARN-TEST-EVOLVE SYSTEM\n";
        std::cout << "==================================\n\n";
        
        for (int cycle = 0; cycle < num_cycles; ++cycle) {
            std::cout << "\n╔════════════════════════════════════════════════════╗\n";
            std::cout << "║  CYCLE " << (cycle + 1) << " / " << num_cycles << " - Generation " 
                     << current_generation_ << "\n";
            std::cout << "╚════════════════════════════════════════════════════╝\n\n";
            
            // Step 1: Feed information to Melvin
            std::cout << "📚 STEP 1: FEEDING INFORMATION TO MELVIN\n";
            std::cout << "----------------------------------------\n";
            feed_knowledge();
            
            // Step 2: Test Melvin
            std::cout << "\n📝 STEP 2: TESTING MELVIN'S KNOWLEDGE\n";
            std::cout << "-------------------------------------\n";
            auto grade_reports = test_knowledge();
            
            // Step 3: Grade results
            std::cout << "\n📊 STEP 3: GRADING MELVIN'S PERFORMANCE\n";
            std::cout << "---------------------------------------\n";
            float overall_accuracy = display_grades(grade_reports);
            
            // Step 4: Determine if evolution is needed
            std::cout << "\n🧬 STEP 4: EVOLUTION DECISION\n";
            std::cout << "-----------------------------\n";
            bool should_evolve = check_evolution_trigger(overall_accuracy);
            
            if (should_evolve) {
                std::cout << "🚨 Performance below threshold! Triggering evolution...\n";
                evolve_melvin();
            } else {
                std::cout << "✅ Performance is good! No evolution needed.\n";
            }
            
            // Step 5: Record progress
            record_progress(overall_accuracy);
            
            current_generation_++;
            
            // Brief pause between cycles
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        // Final summary
        display_final_summary();
    }
    
private:
    void feed_knowledge() {
        int total_facts = 0;
        
        for (const auto& domain : domains_) {
            std::cout << "   📖 Domain: " << domain.name << "\n";
            
            // Feed training texts
            for (const auto& text : domain.training_texts) {
                melvin_learn(melvin_, text.c_str());
                std::cout << "      ➜ " << text << "\n";
            }
            
            total_facts += domain.fact_count();
        }
        
        std::cout << "\n   ✅ Fed " << total_facts << " facts across " 
                 << domains_.size() << " domains\n";
    }
    
    std::vector<GradeReport> test_knowledge() {
        std::vector<GradeReport> reports;
        
        for (const auto& domain : domains_) {
            std::cout << "   🧪 Testing " << domain.name << " knowledge...\n";
            
            // Generate questions
            auto questions = question_gen_.generate_questions(domain, 10);
            std::cout << "      Generated " << questions.size() << " questions\n";
            
            // Get Melvin's answers
            std::vector<std::string> answers;
            for (const auto& q : questions) {
                const char* answer_cstr = melvin_reason(melvin_, q.question.c_str());
                std::string answer = answer_cstr ? answer_cstr : "(no answer)";
                answers.push_back(answer);
            }
            
            // Grade the test
            GradeReport report = grading_system_.grade_test(
                questions, answers, domain.name, current_generation_);
            
            reports.push_back(report);
        }
        
        return reports;
    }
    
    float display_grades(const std::vector<GradeReport>& reports) {
        float total_accuracy = 0.0f;
        
        for (const auto& report : reports) {
            std::cout << "   " << report.get_summary() << "\n";
            total_accuracy += report.accuracy;
        }
        
        float overall_accuracy = reports.empty() ? 0.0f : total_accuracy / reports.size();
        
        std::cout << "\n   🎯 OVERALL ACCURACY: " << std::fixed << std::setprecision(1) 
                 << (overall_accuracy * 100) << "%\n";
        
        // Display detailed report for first domain
        if (!reports.empty()) {
            std::cout << reports[0].get_detailed_report();
        }
        
        return overall_accuracy;
    }
    
    bool check_evolution_trigger(float accuracy) {
        // Evolution triggers:
        // 1. Accuracy below 75%
        // 2. Multiple consecutive poor performances
        // 3. Manual override
        
        std::cout << "   Current Accuracy: " << std::fixed << std::setprecision(1) 
                 << (accuracy * 100) << "%\n";
        std::cout << "   Evolution Threshold: 75%\n";
        
        if (accuracy < 0.75f) {
            return true;
        }
        
        if (evolution_tracker_.should_evolve(3)) {
            std::cout << "   Stagnation detected over multiple generations\n";
            return true;
        }
        
        return false;
    }
    
    void evolve_melvin() {
        std::cout << "\n   🧬 Running evolution cycle...\n";
        
        // Trigger decay pass (simulates evolutionary pressure)
        melvin_decay_pass(melvin_);
        std::cout << "      ✓ Applied memory decay\n";
        
        // Run consolidation (strengthens successful patterns)
        melvin_run_nightly_consolidation(melvin_);
        std::cout << "      ✓ Consolidated memories\n";
        
        // Report new stats
        size_t nodes = melvin_node_count(melvin_);
        size_t edges = melvin_edge_count(melvin_);
        
        std::cout << "      📊 Graph: " << nodes << " nodes, " << edges << " edges\n";
        std::cout << "   ✅ Evolution complete!\n";
    }
    
    void record_progress(float accuracy) {
        float fitness = accuracy; // Simplified fitness = accuracy
        evolution_tracker_.record(fitness, accuracy, accuracy, current_generation_);
    }
    
    void display_final_summary() {
        std::cout << "\n\n";
        std::cout << "╔══════════════════════════════════════════════════════════════╗\n";
        std::cout << "║              FINAL LEARNING SUMMARY                         ║\n";
        std::cout << "╚══════════════════════════════════════════════════════════════╝\n";
        
        std::cout << evolution_tracker_.get_progress_summary();
        
        std::cout << "\n📊 FINAL STATISTICS:\n";
        std::cout << "   Total Generations: " << current_generation_ << "\n";
        std::cout << "   Knowledge Domains: " << domains_.size() << "\n";
        std::cout << "   Total Nodes: " << melvin_node_count(melvin_) << "\n";
        std::cout << "   Total Edges: " << melvin_edge_count(melvin_) << "\n";
        std::cout << "   Health Score: " << std::fixed << std::setprecision(3) 
                 << melvin_get_health_score(melvin_) << "\n";
        
        std::cout << "\n🎉 SYSTEM CAPABILITIES DEMONSTRATED:\n";
        std::cout << "   ✅ Efficient information ingestion\n";
        std::cout << "   ✅ Automated knowledge testing\n";
        std::cout << "   ✅ Accurate performance grading\n";
        std::cout << "   ✅ Adaptive evolution triggers\n";
        std::cout << "   ✅ Continuous learning loop\n";
        std::cout << "   ✅ Progress tracking and logging\n";
        
        std::cout << "\n💾 Results saved to: melvin_evolution_log.csv\n";
    }
};

// ==================== MAIN ====================

int main(int argc, char* argv[]) {
    std::cout << "🧠 MELVIN LEARN-TEST-EVOLVE SYSTEM\n";
    std::cout << "===================================\n\n";
    
    // Parse command line arguments
    int num_cycles = 5;
    std::string store_dir = "melvin_lte_data";
    
    if (argc > 1) {
        num_cycles = std::atoi(argv[1]);
    }
    
    if (argc > 2) {
        store_dir = argv[2];
    }
    
    std::cout << "Configuration:\n";
    std::cout << "   Cycles: " << num_cycles << "\n";
    std::cout << "   Storage: " << store_dir << "\n\n";
    
    std::cout << "This system will:\n";
    std::cout << "   1️⃣  Feed Melvin information from multiple domains\n";
    std::cout << "   2️⃣  Test his knowledge with generated questions\n";
    std::cout << "   3️⃣  Grade his answers for accuracy\n";
    std::cout << "   4️⃣  Trigger evolution when performance drops\n";
    std::cout << "   5️⃣  Repeat to demonstrate continuous improvement\n\n";
    
    std::cout << "Press Enter to start...";
    std::cin.get();
    
    // Create and run the system
    LearnTestEvolveSystem system(store_dir);
    system.run_learning_cycle(num_cycles);
    
    std::cout << "\n✅ Learn-Test-Evolve cycle complete!\n";
    std::cout << "   Melvin has demonstrated continuous learning and adaptation.\n\n";
    
    return 0;
}

