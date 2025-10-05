#!/usr/bin/env python3
"""
MELVIN LLM-STYLE ANALYTICS COMPANION
====================================

Reads CSV logs from verification runs, produces plots, flags guardrail violations,
and ranks runs by overall "health score".

Usage:
    python3 analysis/verify_results.py
    python3 analysis/verify_results.py --input-dir verification_results/
    python3 analysis/verify_results.py --profile Conservative --plot-all
"""

import os
import sys
import pandas as pd
import numpy as np
import matplotlib.pyplot as plt
import seaborn as sns
from pathlib import Path
import argparse
import json
from datetime import datetime
import warnings
warnings.filterwarnings('ignore')

class MelvinAnalytics:
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
        self.domain_weights = {
            'commonsense_qa': {'reasoning': 0.4, 'accuracy': 0.3, 'diversity': 0.3},
            'piqa': {'reasoning': 0.5, 'accuracy': 0.3, 'diversity': 0.2},
            'gsm8k': {'reasoning': 0.6, 'accuracy': 0.4, 'diversity': 0.0},
            'boolq': {'reasoning': 0.3, 'accuracy': 0.5, 'diversity': 0.2},
            'openbookqa': {'reasoning': 0.4, 'accuracy': 0.4, 'diversity': 0.2}
        }
        
    def load_csv_data(self):
        """Load all CSV files from the input directory"""
        print(f"🔍 Loading CSV data from {self.input_dir}")
        
        csv_files = list(self.input_dir.glob("*.csv"))
        if not csv_files:
            print(f"❌ No CSV files found in {self.input_dir}")
            return False
            
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
        for _, row in self.df.iterrows():
            row_violations = []
            if not (self.guardrails['attn_entropy_min'] <= row['attention_entropy'] <= self.guardrails['attn_entropy_max']):
                row_violations.append(f"attn_entropy={row['attention_entropy']:.3f}")
            if not (self.guardrails['diversity_min'] <= row['output_diversity'] <= self.guardrails['diversity_max']):
                row_violations.append(f"diversity={row['output_diversity']:.3f}")
            if row['top2_margin'] < self.guardrails['top2_margin_min']:
                row_violations.append(f"top2_margin={row['top2_margin']:.3f}")
            if row['latency_ms'] > self.guardrails['latency_p95_max_ms']:
                row_violations.append(f"latency={row['latency_ms']}ms")
            if row['fanout'] > self.guardrails['max_fanout']:
                row_violations.append(f"fanout={row['fanout']}")
                
            if row_violations:
                violations += 1
                if violations <= 5:  # Show first 5 violations
                    print(f"   Run {row['run_id']}: {', '.join(row_violations)}")
        
        if violations > 5:
            print(f"   ... and {violations - 5} more violations")
        print(f"   Total violations: {violations}/{len(self.df)} runs")
        
        # Best runs
        print(f"\n🏆 Top 5 Runs by Health Score:")
        top_runs = self.df.nlargest(5, 'health_score')[['run_id', 'mode', 'health_score', 'accuracy', 'attention_entropy', 'output_diversity', 'latency_ms']]
        for _, run in top_runs.iterrows():
            print(f"   {run['run_id']} ({run['mode']}): {run['health_score']:.1f}% - acc={run['accuracy']:.3f}, attn={run['attention_entropy']:.3f}, div={run['output_diversity']:.3f}")
    
    def analyze_domains(self):
        """Analyze performance across different reasoning domains"""
        if not hasattr(self, 'df') or self.df.empty:
            print("❌ No data to analyze")
            return
            
        print("\n🎯 DOMAIN-SPECIFIC ANALYSIS")
        print("==========================")
        
        # Check if we have dataset source information
        if 'dataset_source' not in self.df.columns:
            print("⚠️  No dataset source information found - running basic analysis")
            return
        
        # Domain-specific metrics
        for domain in self.df['dataset_source'].unique():
            domain_data = self.df[self.df['dataset_source'] == domain]
            print(f"\n📊 {domain.upper()} Domain:")
            print(f"   Runs: {len(domain_data)}")
            print(f"   Avg Health Score: {domain_data['health_score'].mean():.1f}%")
            print(f"   Avg Accuracy: {domain_data['accuracy'].mean():.3f}")
            print(f"   Avg Attention Entropy: {domain_data['attention_entropy'].mean():.3f}")
            print(f"   Avg Output Diversity: {domain_data['output_diversity'].mean():.3f}")
            
            # Best profile for this domain
            if 'mode' in domain_data.columns:
                best_profile = domain_data.groupby('mode')['health_score'].mean().idxmax()
                best_score = domain_data.groupby('mode')['health_score'].mean().max()
                print(f"   Best Profile: {best_profile} ({best_score:.1f}%)")
        
        # Cross-domain comparison
        print(f"\n🔄 Cross-Domain Performance:")
        domain_comparison = self.df.groupby('dataset_source').agg({
            'health_score': ['mean', 'std'],
            'accuracy': 'mean',
            'attention_entropy': 'mean',
            'output_diversity': 'mean',
            'latency_ms': 'mean'
        }).round(3)
        print(domain_comparison)
    
    def calculate_domain_health_score(self, row):
        """Calculate domain-specific health score"""
        if 'dataset_source' not in row or row['dataset_source'] not in self.domain_weights:
            return self.calculate_health_score(row)  # Fallback to general score
        
        weights = self.domain_weights[row['dataset_source']]
        score = 0.0
        max_score = 0.0
        
        # Reasoning component (attention entropy + top2 margin)
        reasoning_score = 0.0
        if self.guardrails['attn_entropy_min'] <= row['attention_entropy'] <= self.guardrails['attn_entropy_max']:
            reasoning_score += 0.5
        if row['top2_margin'] >= self.guardrails['top2_margin_min']:
            reasoning_score += 0.5
        score += reasoning_score * weights['reasoning'] * 100
        max_score += weights['reasoning'] * 100
        
        # Accuracy component
        accuracy_score = row['accuracy']
        score += accuracy_score * weights['accuracy'] * 100
        max_score += weights['accuracy'] * 100
        
        # Diversity component
        if weights['diversity'] > 0:
            diversity_score = 0.0
            if self.guardrails['diversity_min'] <= row['output_diversity'] <= self.guardrails['diversity_max']:
                diversity_score = 1.0
            score += diversity_score * weights['diversity'] * 100
            max_score += weights['diversity'] * 100
        
        # Performance penalties
        if row['latency_ms'] > self.guardrails['latency_p95_max_ms']:
            score *= 0.9  # 10% penalty for slow performance
        if row['fanout'] > self.guardrails['max_fanout']:
            score *= 0.95  # 5% penalty for high fanout
        
        return (score / max_score) * 100 if max_score > 0 else 0.0
    
    def create_plots(self, plot_all=False):
        """Create visualization plots"""
        if not hasattr(self, 'df') or self.df.empty:
            print("❌ No data to plot")
            return
            
        print("\n📊 Creating visualization plots...")
        
        # Set up the plotting style
        plt.style.use('seaborn-v0_8')
        sns.set_palette("husl")
        
        # Create output directory for plots
        plot_dir = self.input_dir / "plots"
        plot_dir.mkdir(exist_ok=True)
        
        # 1. Accuracy vs Attention Entropy
        plt.figure(figsize=(10, 6))
        if 'mode' in self.df.columns:
            for mode in self.df['mode'].unique():
                mode_data = self.df[self.df['mode'] == mode]
                plt.scatter(mode_data['attention_entropy'], mode_data['accuracy'], 
                           label=mode, alpha=0.7, s=50)
        else:
            plt.scatter(self.df['attention_entropy'], self.df['accuracy'], alpha=0.7, s=50)
        
        plt.xlabel('Attention Entropy')
        plt.ylabel('Accuracy')
        plt.title('Accuracy vs Attention Entropy')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        # Add guardrail lines
        plt.axvline(self.guardrails['attn_entropy_min'], color='red', linestyle='--', alpha=0.5, label='Guardrail Min')
        plt.axvline(self.guardrails['attn_entropy_max'], color='red', linestyle='--', alpha=0.5, label='Guardrail Max')
        
        plt.tight_layout()
        plt.savefig(plot_dir / 'accuracy_vs_attention_entropy.png', dpi=300, bbox_inches='tight')
        plt.close()
        print(f"   ✅ Saved: accuracy_vs_attention_entropy.png")
        
        # 2. Output Diversity vs Temperature (if available)
        if 'output_temperature' in self.df.columns:
            plt.figure(figsize=(10, 6))
            if 'mode' in self.df.columns:
                for mode in self.df['mode'].unique():
                    mode_data = self.df[self.df['mode'] == mode]
                    plt.scatter(mode_data['output_temperature'], mode_data['output_diversity'], 
                               label=mode, alpha=0.7, s=50)
            else:
                plt.scatter(self.df['output_temperature'], self.df['output_diversity'], alpha=0.7, s=50)
            
            plt.xlabel('Output Temperature')
            plt.ylabel('Output Diversity')
            plt.title('Output Diversity vs Temperature')
            plt.legend()
            plt.grid(True, alpha=0.3)
            
            # Add guardrail lines
            plt.axhline(self.guardrails['diversity_min'], color='red', linestyle='--', alpha=0.5, label='Guardrail Min')
            plt.axhline(self.guardrails['diversity_max'], color='red', linestyle='--', alpha=0.5, label='Guardrail Max')
            
            plt.tight_layout()
            plt.savefig(plot_dir / 'diversity_vs_temperature.png', dpi=300, bbox_inches='tight')
            plt.close()
            print(f"   ✅ Saved: diversity_vs_temperature.png")
        
        # 3. Latency Histogram
        plt.figure(figsize=(10, 6))
        if 'mode' in self.df.columns:
            for mode in self.df['mode'].unique():
                mode_data = self.df[self.df['mode'] == mode]
                plt.hist(mode_data['latency_ms'], alpha=0.7, label=mode, bins=20)
        else:
            plt.hist(self.df['latency_ms'], alpha=0.7, bins=20)
        
        plt.xlabel('Latency (ms)')
        plt.ylabel('Frequency')
        plt.title('Latency Distribution')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        # Add guardrail line
        plt.axvline(self.guardrails['latency_p95_max_ms'], color='red', linestyle='--', alpha=0.5, label='Guardrail Max')
        
        plt.tight_layout()
        plt.savefig(plot_dir / 'latency_histogram.png', dpi=300, bbox_inches='tight')
        plt.close()
        print(f"   ✅ Saved: latency_histogram.png")
        
        # 4. Health Score Distribution
        plt.figure(figsize=(10, 6))
        if 'mode' in self.df.columns:
            for mode in self.df['mode'].unique():
                mode_data = self.df[self.df['mode'] == mode]
                plt.hist(mode_data['health_score'], alpha=0.7, label=mode, bins=20)
        else:
            plt.hist(self.df['health_score'], alpha=0.7, bins=20)
        
        plt.xlabel('Health Score (%)')
        plt.ylabel('Frequency')
        plt.title('Health Score Distribution')
        plt.legend()
        plt.grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(plot_dir / 'health_score_distribution.png', dpi=300, bbox_inches='tight')
        plt.close()
        print(f"   ✅ Saved: health_score_distribution.png")
        
        if plot_all:
            # 5. Correlation Heatmap
            plt.figure(figsize=(12, 8))
            numeric_cols = self.df.select_dtypes(include=[np.number]).columns
            correlation_matrix = self.df[numeric_cols].corr()
            
            sns.heatmap(correlation_matrix, annot=True, cmap='coolwarm', center=0, 
                       square=True, fmt='.2f')
            plt.title('Correlation Matrix of Metrics')
            plt.tight_layout()
            plt.savefig(plot_dir / 'correlation_heatmap.png', dpi=300, bbox_inches='tight')
            plt.close()
            print(f"   ✅ Saved: correlation_heatmap.png")
            
            # 6. Multi-metric scatter plot
            fig, axes = plt.subplots(2, 2, figsize=(15, 12))
            
            # Accuracy vs Health Score
            axes[0,0].scatter(self.df['health_score'], self.df['accuracy'], alpha=0.7)
            axes[0,0].set_xlabel('Health Score (%)')
            axes[0,0].set_ylabel('Accuracy')
            axes[0,0].set_title('Accuracy vs Health Score')
            axes[0,0].grid(True, alpha=0.3)
            
            # Attention Entropy vs Output Diversity
            axes[0,1].scatter(self.df['attention_entropy'], self.df['output_diversity'], alpha=0.7)
            axes[0,1].set_xlabel('Attention Entropy')
            axes[0,1].set_ylabel('Output Diversity')
            axes[0,1].set_title('Attention Entropy vs Output Diversity')
            axes[0,1].grid(True, alpha=0.3)
            
            # Latency vs Fanout
            axes[1,0].scatter(self.df['fanout'], self.df['latency_ms'], alpha=0.7)
            axes[1,0].set_xlabel('Fanout')
            axes[1,0].set_ylabel('Latency (ms)')
            axes[1,0].set_title('Latency vs Fanout')
            axes[1,0].grid(True, alpha=0.3)
            
            # Top2 Margin vs Health Score
            axes[1,1].scatter(self.df['top2_margin'], self.df['health_score'], alpha=0.7)
            axes[1,1].set_xlabel('Top2 Margin')
            axes[1,1].set_ylabel('Health Score (%)')
            axes[1,1].set_title('Top2 Margin vs Health Score')
            axes[1,1].grid(True, alpha=0.3)
            
            plt.tight_layout()
            plt.savefig(plot_dir / 'multi_metric_scatter.png', dpi=300, bbox_inches='tight')
            plt.close()
            print(f"   ✅ Saved: multi_metric_scatter.png")
        
        print(f"\n📊 All plots saved to: {plot_dir}")
    
    def create_domain_plots(self):
        """Create domain-specific visualization plots"""
        if not hasattr(self, 'df') or self.df.empty or 'dataset_source' not in self.df.columns:
            print("❌ No domain data to plot")
            return
            
        print("\n🎯 Creating domain-specific plots...")
        
        # Set up the plotting style
        plt.style.use('seaborn-v0_8')
        sns.set_palette("husl")
        
        # Create output directory for plots
        plot_dir = self.input_dir / "plots"
        plot_dir.mkdir(exist_ok=True)
        
        # 1. Domain Performance Comparison
        plt.figure(figsize=(12, 8))
        
        # Create subplots for different metrics
        fig, axes = plt.subplots(2, 2, figsize=(15, 12))
        
        # Health Score by Domain
        domain_health = self.df.groupby('dataset_source')['health_score'].agg(['mean', 'std']).reset_index()
        axes[0,0].bar(domain_health['dataset_source'], domain_health['mean'], 
                     yerr=domain_health['std'], capsize=5, alpha=0.7)
        axes[0,0].set_title('Health Score by Domain')
        axes[0,0].set_ylabel('Health Score (%)')
        axes[0,0].tick_params(axis='x', rotation=45)
        
        # Accuracy by Domain
        domain_acc = self.df.groupby('dataset_source')['accuracy'].agg(['mean', 'std']).reset_index()
        axes[0,1].bar(domain_acc['dataset_source'], domain_acc['mean'], 
                     yerr=domain_acc['std'], capsize=5, alpha=0.7)
        axes[0,1].set_title('Accuracy by Domain')
        axes[0,1].set_ylabel('Accuracy')
        axes[0,1].tick_params(axis='x', rotation=45)
        
        # Attention Entropy by Domain
        domain_attn = self.df.groupby('dataset_source')['attention_entropy'].agg(['mean', 'std']).reset_index()
        axes[1,0].bar(domain_attn['dataset_source'], domain_attn['mean'], 
                     yerr=domain_attn['std'], capsize=5, alpha=0.7)
        axes[1,0].set_title('Attention Entropy by Domain')
        axes[1,0].set_ylabel('Attention Entropy')
        axes[1,0].tick_params(axis='x', rotation=45)
        
        # Output Diversity by Domain
        domain_div = self.df.groupby('dataset_source')['output_diversity'].agg(['mean', 'std']).reset_index()
        axes[1,1].bar(domain_div['dataset_source'], domain_div['mean'], 
                     yerr=domain_div['std'], capsize=5, alpha=0.7)
        axes[1,1].set_title('Output Diversity by Domain')
        axes[1,1].set_ylabel('Output Diversity')
        axes[1,1].tick_params(axis='x', rotation=45)
        
        plt.tight_layout()
        plt.savefig(plot_dir / 'domain_performance_comparison.png', dpi=300, bbox_inches='tight')
        plt.close()
        print(f"   ✅ Saved: domain_performance_comparison.png")
        
        # 2. Profile Performance Across Domains
        if 'mode' in self.df.columns:
            plt.figure(figsize=(14, 8))
            
            # Create a heatmap of profile performance across domains
            pivot_data = self.df.groupby(['dataset_source', 'mode'])['health_score'].mean().unstack()
            
            sns.heatmap(pivot_data, annot=True, fmt='.1f', cmap='RdYlGn', 
                       center=70, square=True, cbar_kws={'label': 'Health Score (%)'})
            plt.title('Profile Performance Across Domains')
            plt.xlabel('Profile')
            plt.ylabel('Domain')
            plt.tight_layout()
            plt.savefig(plot_dir / 'profile_domain_heatmap.png', dpi=300, bbox_inches='tight')
            plt.close()
            print(f"   ✅ Saved: profile_domain_heatmap.png")
        
        # 3. Domain-specific scatter plots
        fig, axes = plt.subplots(1, 3, figsize=(18, 6))
        
        # Accuracy vs Attention Entropy by Domain
        for domain in self.df['dataset_source'].unique():
            domain_data = self.df[self.df['dataset_source'] == domain]
            axes[0].scatter(domain_data['attention_entropy'], domain_data['accuracy'], 
                           label=domain, alpha=0.7, s=50)
        axes[0].set_xlabel('Attention Entropy')
        axes[0].set_ylabel('Accuracy')
        axes[0].set_title('Accuracy vs Attention Entropy by Domain')
        axes[0].legend()
        axes[0].grid(True, alpha=0.3)
        
        # Health Score vs Latency by Domain
        for domain in self.df['dataset_source'].unique():
            domain_data = self.df[self.df['dataset_source'] == domain]
            axes[1].scatter(domain_data['latency_ms'], domain_data['health_score'], 
                           label=domain, alpha=0.7, s=50)
        axes[1].set_xlabel('Latency (ms)')
        axes[1].set_ylabel('Health Score (%)')
        axes[1].set_title('Health Score vs Latency by Domain')
        axes[1].legend()
        axes[1].grid(True, alpha=0.3)
        
        # Output Diversity vs Top2 Margin by Domain
        for domain in self.df['dataset_source'].unique():
            domain_data = self.df[self.df['dataset_source'] == domain]
            axes[2].scatter(domain_data['top2_margin'], domain_data['output_diversity'], 
                           label=domain, alpha=0.7, s=50)
        axes[2].set_xlabel('Top2 Margin')
        axes[2].set_ylabel('Output Diversity')
        axes[2].set_title('Output Diversity vs Top2 Margin by Domain')
        axes[2].legend()
        axes[2].grid(True, alpha=0.3)
        
        plt.tight_layout()
        plt.savefig(plot_dir / 'domain_scatter_analysis.png', dpi=300, bbox_inches='tight')
        plt.close()
        print(f"   ✅ Saved: domain_scatter_analysis.png")
        
        print(f"\n🎯 Domain plots saved to: {plot_dir}")
    
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
            'guardrails': self.guardrails
        }
        
        if 'mode' in self.df.columns:
            report['profile_comparison'] = self.df.groupby('mode').agg({
                'health_score': ['mean', 'std'],
                'accuracy': 'mean',
                'attention_entropy': 'mean',
                'output_diversity': 'mean',
                'latency_ms': 'mean'
            }).to_dict()
        
        # Save report
        report_file = self.input_dir / "analysis_report.json"
        with open(report_file, 'w') as f:
            json.dump(report, f, indent=2)
        
        print(f"\n📋 Analysis report saved to: {report_file}")
        return report

def main():
    parser = argparse.ArgumentParser(description='MELVIN LLM-Style Analytics Companion')
    parser.add_argument('--input-dir', default='verification_results', 
                       help='Directory containing CSV files')
    parser.add_argument('--profile', help='Filter by specific profile')
    parser.add_argument('--domain', help='Filter by specific domain/dataset')
    parser.add_argument('--plot-all', action='store_true', 
                       help='Generate all plots including correlation heatmap')
    parser.add_argument('--domain-analysis', action='store_true',
                       help='Run domain-specific analysis')
    parser.add_argument('--no-plots', action='store_true', 
                       help='Skip plot generation')
    parser.add_argument('--no-domain-plots', action='store_true',
                       help='Skip domain-specific plots')
    
    args = parser.parse_args()
    
    print("🧠 MELVIN LLM-STYLE ANALYTICS COMPANION")
    print("=======================================")
    
    # Initialize analytics
    analytics = MelvinAnalytics(args.input_dir)
    
    # Load data
    if not analytics.load_csv_data():
        sys.exit(1)
    
    # Filter by profile if specified
    if args.profile and 'mode' in analytics.df.columns:
        analytics.df = analytics.df[analytics.df['mode'] == args.profile]
        print(f"🔍 Filtered to profile: {args.profile}")
    
    # Filter by domain if specified
    if args.domain and 'dataset_source' in analytics.df.columns:
        analytics.df = analytics.df[analytics.df['dataset_source'] == args.domain]
        print(f"🔍 Filtered to domain: {args.domain}")
    
    # Calculate domain-specific health scores if we have domain data
    if 'dataset_source' in analytics.df.columns:
        analytics.df['domain_health_score'] = analytics.df.apply(analytics.calculate_domain_health_score, axis=1)
        print("📊 Calculated domain-specific health scores")
    
    # Analyze results
    analytics.analyze_results()
    
    # Run domain-specific analysis if requested or if we have domain data
    if args.domain_analysis or 'dataset_source' in analytics.df.columns:
        analytics.analyze_domains()
    
    # Generate plots
    if not args.no_plots:
        analytics.create_plots(plot_all=args.plot_all)
    
    # Generate domain-specific plots
    if not args.no_domain_plots and 'dataset_source' in analytics.df.columns:
        analytics.create_domain_plots()
    
    # Generate report
    analytics.generate_report()
    
    print("\n✅ Analytics complete!")

if __name__ == "__main__":
    main()
