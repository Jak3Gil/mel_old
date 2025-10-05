#!/usr/bin/env python3
"""
MELVIN LLM-STYLE ANALYTICS COMPANION (Lite Version)
==================================================

Reads CSV logs from verification runs, analyzes metrics, flags guardrail violations,
and ranks runs by overall "health score". Works without matplotlib for basic analysis.

Usage:
    python3 analysis/verify_results_lite.py
    python3 analysis/verify_results_lite.py --input-dir verification_results/
"""

import os
import sys
import pandas as pd
import numpy as np
from pathlib import Path
import argparse
import json
from datetime import datetime
import warnings
warnings.filterwarnings('ignore')

class MelvinAnalyticsLite:
    def __init__(self, input_dir="verification_results"):
        self.input_dir = Path(input_dir)
        self.results = {}
        self.guardrails = {
            'attn_entropy_min': 0.08,
            'attn_entropy_max': 0.35,
            'diversity_min': 0.45,
            'diversity_max': 0.85,
            'top2_margin_min': 0.2,
            'max_fanout': 16,
            'latency_p95_max_ms': 50,
            'embedding_coherence_min': 0.55
        }
        
    def load_csv_data(self):
        """Load all CSV files from the input directory"""
        print(f"🔍 Loading CSV data from {self.input_dir}")
        
        csv_files = list(self.input_dir.glob("*.csv"))
        if not csv_files:
            print(f"❌ No CSV files found in {self.input_dir}")
            print(f"   Creating sample data for demonstration...")
            self.create_sample_data()
            return True
            
        all_data = []
        for csv_file in csv_files:
            try:
                df = pd.read_csv(csv_file)
                df['source_file'] = csv_file.name
                all_data.append(df)
                print(f"   ✅ Loaded {csv_file.name}: {len(df)} rows")
            except Exception as e:
                print(f"   ❌ Error loading {csv_file.name}: {e}")
                
        if all_data:
            self.df = pd.concat(all_data, ignore_index=True)
            print(f"📊 Total data loaded: {len(self.df)} rows")
            return True
        return False
    
    def create_sample_data(self):
        """Create sample data for demonstration"""
        print("   📝 Creating sample verification data...")
        
        # Create sample data
        np.random.seed(42)
        n_runs = 50
        
        data = {
            'run_id': [f"run_{i:03d}" for i in range(n_runs)],
            'mode': np.random.choice(['classic', 'hybrid'], n_runs),
            'seed': np.random.choice([17, 42, 123], n_runs),
            'attention_entropy': np.random.normal(0.2, 0.05, n_runs),
            'top2_margin': np.random.uniform(0.1, 0.4, n_runs),
            'output_diversity': np.random.uniform(0.3, 0.9, n_runs),
            'accuracy': np.random.uniform(0.7, 0.95, n_runs),
            'latency_ms': np.random.exponential(30, n_runs),
            'mem_mb': np.random.uniform(50, 200, n_runs),
            'context_overlap': np.random.uniform(0.2, 0.8, n_runs),
            'fanout': np.random.choice([8, 12, 16], n_runs)
        }
        
        self.df = pd.DataFrame(data)
        
        # Save sample data
        self.input_dir.mkdir(exist_ok=True)
        sample_file = self.input_dir / "sample_results.csv"
        self.df.to_csv(sample_file, index=False)
        print(f"   ✅ Sample data saved to {sample_file}")
    
    def calculate_health_score(self, row):
        """Calculate overall health score for a run"""
        score = 0.0
        max_score = 0.0
        
        # Attention entropy (25% weight)
        if self.guardrails['attn_entropy_min'] <= row['attention_entropy'] <= self.guardrails['attn_entropy_max']:
            score += 25.0
        max_score += 25.0
        
        # Output diversity (25% weight)
        if self.guardrails['diversity_min'] <= row['output_diversity'] <= self.guardrails['diversity_max']:
            score += 25.0
        max_score += 25.0
        
        # Top2 margin (20% weight)
        if row['top2_margin'] >= self.guardrails['top2_margin_min']:
            score += 20.0
        max_score += 20.0
        
        # Latency (15% weight)
        if row['latency_ms'] <= self.guardrails['latency_p95_max_ms']:
            score += 15.0
        max_score += 15.0
        
        # Fanout (15% weight)
        if row['fanout'] <= self.guardrails['max_fanout']:
            score += 15.0
        max_score += 15.0
        
        return (score / max_score) * 100 if max_score > 0 else 0.0
    
    def analyze_results(self):
        """Analyze the loaded results"""
        if not hasattr(self, 'df') or self.df.empty:
            print("❌ No data to analyze")
            return
            
        print("\n📊 ANALYSIS RESULTS")
        print("==================")
        
        # Calculate health scores
        self.df['health_score'] = self.df.apply(self.calculate_health_score, axis=1)
        
        # Overall statistics
        print(f"📈 Overall Statistics:")
        print(f"   Total runs: {len(self.df)}")
        print(f"   Average health score: {self.df['health_score'].mean():.1f}%")
        print(f"   Best health score: {self.df['health_score'].max():.1f}%")
        print(f"   Worst health score: {self.df['health_score'].min():.1f}%")
        
        # Profile comparison
        if 'mode' in self.df.columns:
            print(f"\n🎯 Profile Comparison:")
            profile_stats = self.df.groupby('mode').agg({
                'health_score': ['mean', 'std', 'count'],
                'accuracy': 'mean',
                'attention_entropy': 'mean',
                'output_diversity': 'mean',
                'latency_ms': 'mean'
            }).round(3)
            print(profile_stats)
        
        # Guardrail violations
        print(f"\n⚠️  Guardrail Violations:")
        violations = 0
        violation_details = []
        
        for _, row in self.df.iterrows():
            row_violations = []
            if not (self.guardrails['attn_entropy_min'] <= row['attention_entropy'] <= self.guardrails['attn_entropy_max']):
                row_violations.append(f"attn_entropy={row['attention_entropy']:.3f}")
            if not (self.guardrails['diversity_min'] <= row['output_diversity'] <= self.guardrails['diversity_max']):
                row_violations.append(f"diversity={row['output_diversity']:.3f}")
            if row['top2_margin'] < self.guardrails['top2_margin_min']:
                row_violations.append(f"top2_margin={row['top2_margin']:.3f}")
            if row['latency_ms'] > self.guardrails['latency_p95_max_ms']:
                row_violations.append(f"latency={row['latency_ms']:.0f}ms")
            if row['fanout'] > self.guardrails['max_fanout']:
                row_violations.append(f"fanout={row['fanout']}")
                
            if row_violations:
                violations += 1
                violation_details.append((row['run_id'], row_violations))
        
        # Show violations
        for i, (run_id, violations) in enumerate(violation_details[:5]):
            print(f"   Run {run_id}: {', '.join(violations)}")
        
        if len(violation_details) > 5:
            print(f"   ... and {len(violation_details) - 5} more violations")
        print(f"   Total violations: {len(violation_details)}/{len(self.df)} runs")
        
        # Best runs
        print(f"\n🏆 Top 5 Runs by Health Score:")
        top_runs = self.df.nlargest(5, 'health_score')[['run_id', 'mode', 'health_score', 'accuracy', 'attention_entropy', 'output_diversity', 'latency_ms']]
        for _, run in top_runs.iterrows():
            print(f"   {run['run_id']} ({run['mode']}): {run['health_score']:.1f}% - acc={run['accuracy']:.3f}, attn={run['attention_entropy']:.3f}, div={run['output_diversity']:.3f}")
        
        # Tuning recommendations
        self.generate_tuning_recommendations()
    
    def generate_tuning_recommendations(self):
        """Generate tuning recommendations based on analysis"""
        print(f"\n🔧 TUNING RECOMMENDATIONS")
        print("=========================")
        
        # Analyze patterns
        avg_entropy = self.df['attention_entropy'].mean()
        avg_diversity = self.df['output_diversity'].mean()
        avg_latency = self.df['latency_ms'].mean()
        avg_fanout = self.df['fanout'].mean()
        
        recommendations = []
        
        # Attention entropy analysis
        if avg_entropy < 0.15:
            recommendations.append("⚠️  Low attention entropy - increase temperature by +0.1, increase fanout by +2")
        elif avg_entropy > 0.30:
            recommendations.append("⚠️  High attention entropy - decrease temperature by -0.1, decrease fanout by -2")
        else:
            recommendations.append("✅ Attention entropy in optimal range")
        
        # Output diversity analysis
        if avg_diversity < 0.6:
            recommendations.append("⚠️  Low output diversity - increase temperature by +0.1, increase repetition penalty")
        elif avg_diversity > 0.85:
            recommendations.append("⚠️  High output diversity - decrease temperature by -0.1, increase top-p threshold")
        else:
            recommendations.append("✅ Output diversity in optimal range")
        
        # Latency analysis
        if avg_latency > 50:
            recommendations.append("⚠️  High latency - decrease fanout by -4, enable attention dropout")
        else:
            recommendations.append("✅ Latency within acceptable range")
        
        # Fanout analysis
        if avg_fanout > 14:
            recommendations.append("⚠️  High fanout - consider reducing for better performance")
        elif avg_fanout < 10:
            recommendations.append("⚠️  Low fanout - consider increasing for better exploration")
        else:
            recommendations.append("✅ Fanout in balanced range")
        
        for rec in recommendations:
            print(f"   {rec}")
    
    def generate_report(self):
        """Generate a comprehensive analysis report"""
        if not hasattr(self, 'df') or self.df.empty:
            print("❌ No data to report")
            return
            
        report = {
            'timestamp': datetime.now().isoformat(),
            'total_runs': len(self.df),
            'average_health_score': float(self.df['health_score'].mean()),
            'best_health_score': float(self.df['health_score'].max()),
            'worst_health_score': float(self.df['health_score'].min()),
            'guardrail_violations': int((self.df['health_score'] < 100).sum()),
            'guardrails': self.guardrails,
            'summary_stats': {
                'avg_attention_entropy': float(self.df['attention_entropy'].mean()),
                'avg_output_diversity': float(self.df['output_diversity'].mean()),
                'avg_accuracy': float(self.df['accuracy'].mean()),
                'avg_latency_ms': float(self.df['latency_ms'].mean()),
                'avg_fanout': float(self.df['fanout'].mean())
            }
        }
        
        if 'mode' in self.df.columns:
            profile_stats = self.df.groupby('mode').agg({
                'health_score': ['mean', 'std'],
                'accuracy': 'mean',
                'attention_entropy': 'mean',
                'output_diversity': 'mean',
                'latency_ms': 'mean'
            })
            # Convert MultiIndex columns to string keys for JSON serialization
            profile_dict = {}
            for mode in profile_stats.index:
                profile_dict[str(mode)] = {}
                for col in profile_stats.columns:
                    profile_dict[str(mode)][str(col)] = float(profile_stats.loc[mode, col])
            report['profile_comparison'] = profile_dict
        
        # Save report
        report_file = self.input_dir / "analysis_report.json"
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"\n📋 Analysis report saved to: {report_file}")
        return report

def main():
    parser = argparse.ArgumentParser(description='MELVIN LLM-Style Analytics Companion (Lite)')
    parser.add_argument('--input-dir', default='verification_results', 
                       help='Directory containing CSV files')
    parser.add_argument('--profile', help='Filter by specific profile')
    
    args = parser.parse_args()
    
    print("🧠 MELVIN LLM-STYLE ANALYTICS COMPANION (LITE)")
    print("===============================================")
    
    # Initialize analytics
    analytics = MelvinAnalyticsLite(args.input_dir)
    
    # Load data
    if not analytics.load_csv_data():
        sys.exit(1)
    
    # Filter by profile if specified
    if args.profile and 'mode' in analytics.df.columns:
        analytics.df = analytics.df[analytics.df['mode'] == args.profile]
        print(f"🔍 Filtered to profile: {args.profile}")
    
    # Analyze results
    analytics.analyze_results()
    
    # Generate report
    analytics.generate_report()
    
    print("\n✅ Analytics complete!")
    print("\n📊 For visualization plots, install matplotlib and run:")
    print("   pip install matplotlib seaborn")
    print("   python3 analysis/verify_results.py --plot-all")

if __name__ == "__main__":
    main()
