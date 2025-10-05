#!/usr/bin/env python3
"""
UCA Experiment Runner
Systematic testing and tuning of mathematical formulas
"""

import os
import sys
import subprocess
import time
import csv
import json
import numpy as np
import matplotlib.pyplot as plt
from pathlib import Path
import argparse

class UCAExperimentRunner:
    def __init__(self, base_dir="."):
        self.base_dir = Path(base_dir)
        self.results_dir = self.base_dir / "experiment_results"
        self.results_dir.mkdir(exist_ok=True)
        
        # Default parameters (good starting priors)
        self.default_params = {
            "degree_epsilon": 1e-6,
            "recency_tau": 300.0,  # 5 minutes
            "multi_hop_gamma": 0.93,
            "diversity_lambda": 0.30,
            "temporal_continuity_t": 5.0,
            "contradiction_beta": 1.5,
            "rel_prior_temporal": 1.20,
            "rel_prior_generalization": 1.10,
            "rel_prior_exact": 1.00,
            "rel_prior_leap": 0.85,
            "conf_beta_path": 3.0,
            "conf_beta_length": 0.15,
            "conf_beta_similarity": 0.5,
            "conf_beta_contradiction": 1.0,
            "min_conf_for_emit": 0.15
        }
        
    def run_experiment(self, name, env_vars=None, duration=60):
        """Run a single experiment with given environment variables"""
        print(f"🧪 Running experiment: {name}")
        
        # Set up environment
        env = os.environ.copy()
        if env_vars:
            env.update(env_vars)
        
        # Create test prompts
        prompts_file = self.results_dir / f"{name}_prompts.txt"
        with open(prompts_file, 'w') as f:
            f.write("what are cats\n")
            f.write("what happened then\n")
            f.write("cats are mammals\n")
            f.write("exact match\n")
            f.write("leap of logic\n")
            f.write("what happened first then next\n")
            f.write("cats are reptiles\n")
            f.write("what is 2 + 3\n")
            f.write("sequence: 1, 3, 5, 7\n")
        
        # Run melvin_scheduler
        cmd = ["./melvin_scheduler"]
        try:
            with open(prompts_file, 'r') as f:
                process = subprocess.Popen(
                    cmd, 
                    stdin=f, 
                    stdout=subprocess.PIPE, 
                    stderr=subprocess.PIPE,
                    env=env,
                    cwd=self.base_dir
                )
            
            # Let it run for specified duration
            time.sleep(duration)
            process.terminate()
            process.wait(timeout=5)
            
        except subprocess.TimeoutExpired:
            process.kill()
        except Exception as e:
            print(f"Error running experiment {name}: {e}")
            return None
        
        # Collect results
        metrics_file = self.base_dir / "uca_metrics.csv"
        if metrics_file.exists():
            return self.analyze_metrics(metrics_file, name)
        else:
            print(f"No metrics file found for {name}")
            return None
    
    def analyze_metrics(self, metrics_file, experiment_name):
        """Analyze CSV metrics and return summary statistics"""
        try:
            with open(metrics_file, 'r') as f:
                reader = csv.DictReader(f)
                rows = list(reader)
            
            if not rows:
                return None
            
            # Calculate statistics
            confidences = [float(row['conf']) for row in rows]
            path_scores = [float(row['path_score']) for row in rows]
            diversities = [float(row['div_pen']) for row in rows]
            degree_norms = [float(row['deg_norm']) for row in rows]
            
            stats = {
                'experiment': experiment_name,
                'num_samples': len(rows),
                'avg_confidence': np.mean(confidences),
                'std_confidence': np.std(confidences),
                'avg_path_score': np.mean(path_scores),
                'avg_diversity': np.mean(diversities),
                'avg_degree_norm': np.mean(degree_norms),
                'min_confidence': np.min(confidences),
                'max_confidence': np.max(confidences)
            }
            
            return stats
            
        except Exception as e:
            print(f"Error analyzing metrics for {experiment_name}: {e}")
            return None
    
    def run_baseline_experiment(self):
        """Run baseline experiment with all formulas enabled"""
        return self.run_experiment("baseline", duration=30)
    
    def run_ablation_experiments(self):
        """Run ablation experiments - disable one formula at a time"""
        ablations = {
            "no_degree": {"UCA_NO_DEGREE": "1"},
            "no_relpr": {"UCA_NO_RELPR": "1"},
            "no_contra": {"UCA_NO_CONTRA": "1"},
            "no_cont": {"UCA_NO_CONT": "1"},
            "no_div": {"UCA_NO_DIV": "1"},
            "use_td": {"UCA_USE_TD": "1"}
        }
        
        results = {}
        for name, env_vars in ablations.items():
            result = self.run_experiment(f"ablation_{name}", env_vars, duration=20)
            if result:
                results[name] = result
        
        return results
    
    def run_learning_curve_experiment(self):
        """Run learning curve experiment - same prompts repeated"""
        print("📈 Running learning curve experiment...")
        
        # Create repeated prompts
        prompts_file = self.results_dir / "learning_curve_prompts.txt"
        with open(prompts_file, 'w') as f:
            for i in range(10):  # Repeat same prompts 10 times
                f.write("what are cats\n")
                f.write("what is 2 + 3\n")
                f.write("sequence: 1, 3, 5, 7\n")
        
        # Run with learning enabled
        env = os.environ.copy()
        env["UCA_LEARNING_ENABLED"] = "1"
        
        result = self.run_experiment("learning_curve", env, duration=60)
        return result
    
    def run_stress_test(self):
        """Run stress test with noise prompts"""
        print("💥 Running stress test...")
        
        # Create noise prompts
        prompts_file = self.results_dir / "stress_prompts.txt"
        with open(prompts_file, 'w') as f:
            f.write("asdfghjkl\n")
            f.write("random nonsense text\n")
            f.write("123456789\n")
            f.write("!@#$%^&*()\n")
            f.write("what are cats\n")  # One good prompt
        
        result = self.run_experiment("stress_test", duration=30)
        return result
    
    def generate_report(self, baseline, ablations, learning, stress):
        """Generate comprehensive experiment report"""
        report = {
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "baseline": baseline,
            "ablations": ablations,
            "learning_curve": learning,
            "stress_test": stress,
            "recommendations": []
        }
        
        # Analyze results and generate recommendations
        if baseline and ablations:
            baseline_conf = baseline.get('avg_confidence', 0)
            
            for ablation_name, ablation_result in ablations.items():
                ablation_conf = ablation_result.get('avg_confidence', 0)
                impact = baseline_conf - ablation_conf
                
                if abs(impact) > 0.1:  # Significant impact
                    if impact > 0:
                        report["recommendations"].append(
                            f"Keep {ablation_name} formula - improves confidence by {impact:.3f}"
                        )
                    else:
                        report["recommendations"].append(
                            f"Consider removing {ablation_name} formula - reduces confidence by {abs(impact):.3f}"
                        )
        
        # Save report
        report_file = self.results_dir / "experiment_report.json"
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"📊 Report saved to {report_file}")
        return report
    
    def create_visualizations(self, results):
        """Create visualization plots"""
        try:
            import matplotlib.pyplot as plt
            
            # Confidence comparison plot
            if 'ablations' in results and results['ablations']:
                fig, ax = plt.subplots(figsize=(10, 6))
                
                experiments = ['baseline'] + list(results['ablations'].keys())
                confidences = [results.get('baseline', {}).get('avg_confidence', 0)]
                confidences.extend([results['ablations'][k].get('avg_confidence', 0) 
                                   for k in results['ablations'].keys()])
                
                bars = ax.bar(experiments, confidences)
                ax.set_ylabel('Average Confidence')
                ax.set_title('Formula Ablation Impact on Confidence')
                ax.tick_params(axis='x', rotation=45)
                
                # Color bars based on performance
                for i, (bar, conf) in enumerate(zip(bars, confidences)):
                    if i == 0:  # Baseline
                        bar.set_color('blue')
                    elif conf < confidences[0]:  # Worse than baseline
                        bar.set_color('red')
                    else:  # Better than baseline
                        bar.set_color('green')
                
                plt.tight_layout()
                plt.savefig(self.results_dir / 'confidence_comparison.png', dpi=150)
                plt.close()
                
                print("📈 Visualization saved to experiment_results/confidence_comparison.png")
                
        except ImportError:
            print("Matplotlib not available - skipping visualizations")
        except Exception as e:
            print(f"Error creating visualizations: {e}")
    
    def run_full_experiment_suite(self):
        """Run complete experiment suite"""
        print("🚀 Starting UCA Full Experiment Suite")
        print("=" * 50)
        
        # Run all experiments
        baseline = self.run_baseline_experiment()
        ablations = self.run_ablation_experiments()
        learning = self.run_learning_curve_experiment()
        stress = self.run_stress_test()
        
        # Generate report
        results = {
            'baseline': baseline,
            'ablations': ablations,
            'learning_curve': learning,
            'stress_test': stress
        }
        
        report = self.generate_report(baseline, ablations, learning, stress)
        self.create_visualizations(results)
        
        # Print summary
        print("\n📋 EXPERIMENT SUMMARY")
        print("=" * 30)
        
        if baseline:
            print(f"Baseline confidence: {baseline['avg_confidence']:.3f} ± {baseline['std_confidence']:.3f}")
        
        if ablations:
            print("\nAblation impacts:")
            baseline_conf = baseline.get('avg_confidence', 0) if baseline else 0
            for name, result in ablations.items():
                impact = baseline_conf - result.get('avg_confidence', 0)
                print(f"  {name}: {impact:+.3f} confidence change")
        
        if report['recommendations']:
            print("\n💡 Recommendations:")
            for rec in report['recommendations']:
                print(f"  • {rec}")
        
        print(f"\n✅ Full experiment suite completed!")
        print(f"📁 Results saved in: {self.results_dir}")

def main():
    parser = argparse.ArgumentParser(description='UCA Experiment Runner')
    parser.add_argument('--quick', action='store_true', help='Run quick 2-minute tests')
    parser.add_argument('--ablation-only', action='store_true', help='Run only ablation tests')
    parser.add_argument('--learning-only', action='store_true', help='Run only learning curve test')
    
    args = parser.parse_args()
    
    runner = UCAExperimentRunner()
    
    if args.quick:
        print("⚡ Running quick tests...")
        baseline = runner.run_baseline_experiment()
        ablations = runner.run_ablation_experiments()
        runner.generate_report(baseline, ablations, None, None)
        
    elif args.ablation_only:
        print("🔬 Running ablation tests only...")
        baseline = runner.run_baseline_experiment()
        ablations = runner.run_ablation_experiments()
        runner.generate_report(baseline, ablations, None, None)
        
    elif args.learning_only:
        print("📈 Running learning curve test only...")
        learning = runner.run_learning_curve_experiment()
        runner.generate_report(None, {}, learning, None)
        
    else:
        runner.run_full_experiment_suite()

if __name__ == "__main__":
    main()
