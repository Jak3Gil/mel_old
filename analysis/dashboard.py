#!/usr/bin/env python3
"""
MELVIN REGRESSION DASHBOARD
==========================

Streamlit dashboard for monitoring Melvin's performance across commits and runs.
Provides real-time visualization of health metrics, guardrail violations, and
domain-specific performance trends.

Usage:
    streamlit run analysis/dashboard.py
"""

import streamlit as st
import pandas as pd
import numpy as np
import plotly.express as px
import plotly.graph_objects as go
from plotly.subplots import make_subplots
import json
from pathlib import Path
import os
from datetime import datetime, timedelta
import warnings
warnings.filterwarnings('ignore')

# Page configuration
st.set_page_config(
    page_title="Melvin Regression Dashboard",
    page_icon="🧠",
    layout="wide",
    initial_sidebar_state="expanded"
)

class MelvinDashboard:
    def __init__(self):
        self.data_dir = Path("verification_results")
        self.hf_data_dir = Path("huggingface_data/melvin_tests")
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
    
    def load_data(self, data_source="all"):
        """Load CSV data from specified sources"""
        all_data = []
        
        if data_source in ["all", "verification"]:
            # Load verification results
            if self.data_dir.exists():
                for csv_file in self.data_dir.glob("*.csv"):
                    try:
                        df = pd.read_csv(csv_file)
                        df['data_source'] = 'verification'
                        df['file_name'] = csv_file.name
                        all_data.append(df)
                    except Exception as e:
                        st.warning(f"Error loading {csv_file}: {e}")
        
        if data_source in ["all", "huggingface"]:
            # Load HuggingFace test results
            if self.hf_data_dir.exists():
                for csv_file in self.hf_data_dir.glob("*.csv"):
                    try:
                        df = pd.read_csv(csv_file)
                        df['data_source'] = 'huggingface'
                        df['file_name'] = csv_file.name
                        all_data.append(df)
                    except Exception as e:
                        st.warning(f"Error loading {csv_file}: {e}")
        
        if all_data:
            combined_df = pd.concat(all_data, ignore_index=True)
            
            # Add timestamp if not present
            if 'timestamp' not in combined_df.columns:
                combined_df['timestamp'] = datetime.now()
            
            # Calculate health scores
            combined_df['health_score'] = combined_df.apply(self.calculate_health_score, axis=1)
            
            return combined_df
        return pd.DataFrame()
    
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
    
    def render_sidebar(self, df):
        """Render the sidebar with filters and controls"""
        st.sidebar.header("🎛️ Dashboard Controls")
        
        # Data source filter
        if not df.empty:
            data_sources = df['data_source'].unique().tolist()
            selected_sources = st.sidebar.multiselect(
                "Data Sources",
                data_sources,
                default=data_sources
            )
            
            # Profile filter
            if 'mode' in df.columns:
                profiles = df['mode'].unique().tolist()
                selected_profiles = st.sidebar.multiselect(
                    "Profiles",
                    profiles,
                    default=profiles
                )
            else:
                selected_profiles = None
            
            # Domain filter
            if 'dataset_source' in df.columns:
                domains = df['dataset_source'].unique().tolist()
                selected_domains = st.sidebar.multiselect(
                    "Domains",
                    domains,
                    default=domains
                )
            else:
                selected_domains = None
            
            # Time range filter
            if 'timestamp' in df.columns:
                min_date = df['timestamp'].min()
                max_date = df['timestamp'].max()
                
                if pd.notna(min_date) and pd.notna(max_date):
                    date_range = st.sidebar.date_input(
                        "Date Range",
                        value=(min_date.date(), max_date.date()),
                        min_value=min_date.date(),
                        max_value=max_date.date()
                    )
                else:
                    date_range = None
            else:
                date_range = None
            
            return selected_sources, selected_profiles, selected_domains, date_range
        return None, None, None, None
    
    def apply_filters(self, df, sources, profiles, domains, date_range):
        """Apply filters to the dataframe"""
        filtered_df = df.copy()
        
        if sources:
            filtered_df = filtered_df[filtered_df['data_source'].isin(sources)]
        
        if profiles:
            filtered_df = filtered_df[filtered_df['mode'].isin(profiles)]
        
        if domains:
            filtered_df = filtered_df[filtered_df['dataset_source'].isin(domains)]
        
        if date_range and len(date_range) == 2:
            start_date, end_date = date_range
            filtered_df = filtered_df[
                (filtered_df['timestamp'].dt.date >= start_date) &
                (filtered_df['timestamp'].dt.date <= end_date)
            ]
        
        return filtered_df
    
    def render_overview_metrics(self, df):
        """Render overview metrics cards"""
        if df.empty:
            st.warning("No data available for overview metrics")
            return
        
        col1, col2, col3, col4 = st.columns(4)
        
        with col1:
            avg_health = df['health_score'].mean()
            st.metric(
                "Average Health Score",
                f"{avg_health:.1f}%",
                delta=f"{avg_health - 70:.1f}%" if avg_health != 70 else None
            )
        
        with col2:
            total_runs = len(df)
            st.metric("Total Runs", total_runs)
        
        with col3:
            violations = (df['health_score'] < 100).sum()
            violation_rate = (violations / total_runs) * 100 if total_runs > 0 else 0
            st.metric(
                "Guardrail Violations",
                f"{violations}/{total_runs}",
                delta=f"{violation_rate:.1f}%"
            )
        
        with col4:
            avg_accuracy = df['accuracy'].mean()
            st.metric(
                "Average Accuracy",
                f"{avg_accuracy:.3f}",
                delta=f"{avg_accuracy - 0.8:.3f}" if avg_accuracy != 0.8 else None
            )
    
    def render_health_trends(self, df):
        """Render health score trends over time"""
        if df.empty or 'timestamp' not in df.columns:
            st.warning("No timestamp data available for trends")
            return
        
        st.subheader("📈 Health Score Trends")
        
        # Group by timestamp and calculate metrics
        df_sorted = df.sort_values('timestamp')
        
        # Create trend chart
        fig = go.Figure()
        
        # Add health score trend
        fig.add_trace(go.Scatter(
            x=df_sorted['timestamp'],
            y=df_sorted['health_score'],
            mode='markers+lines',
            name='Health Score',
            line=dict(color='blue', width=2),
            marker=dict(size=6)
        ))
        
        # Add guardrail lines
        fig.add_hline(y=70, line_dash="dash", line_color="orange", 
                     annotation_text="Target (70%)")
        fig.add_hline(y=100, line_dash="dash", line_color="green", 
                     annotation_text="Perfect (100%)")
        
        fig.update_layout(
            title="Health Score Over Time",
            xaxis_title="Time",
            yaxis_title="Health Score (%)",
            hovermode='x unified'
        )
        
        st.plotly_chart(fig, use_container_width=True)
    
    def render_domain_analysis(self, df):
        """Render domain-specific analysis"""
        if df.empty or 'dataset_source' not in df.columns:
            st.warning("No domain data available")
            return
        
        st.subheader("🎯 Domain Performance Analysis")
        
        # Domain comparison
        domain_stats = df.groupby('dataset_source').agg({
            'health_score': ['mean', 'std', 'count'],
            'accuracy': 'mean',
            'attention_entropy': 'mean',
            'output_diversity': 'mean',
            'latency_ms': 'mean'
        }).round(3)
        
        st.dataframe(domain_stats)
        
        # Domain performance chart
        fig = px.bar(
            df, 
            x='dataset_source', 
            y='health_score',
            color='dataset_source',
            title='Health Score by Domain',
            labels={'health_score': 'Health Score (%)', 'dataset_source': 'Domain'}
        )
        st.plotly_chart(fig, use_container_width=True)
        
        # Profile performance across domains
        if 'mode' in df.columns:
            fig = px.box(
                df,
                x='dataset_source',
                y='health_score',
                color='mode',
                title='Health Score Distribution by Domain and Profile'
            )
            st.plotly_chart(fig, use_container_width=True)
    
    def render_guardrail_analysis(self, df):
        """Render guardrail violation analysis"""
        if df.empty:
            st.warning("No data available for guardrail analysis")
            return
        
        st.subheader("⚠️ Guardrail Violations")
        
        # Calculate violations
        violations = []
        for _, row in df.iterrows():
            row_violations = []
            if not (self.guardrails['attn_entropy_min'] <= row['attention_entropy'] <= self.guardrails['attn_entropy_max']):
                row_violations.append("attention_entropy")
            if not (self.guardrails['diversity_min'] <= row['output_diversity'] <= self.guardrails['diversity_max']):
                row_violations.append("output_diversity")
            if row['top2_margin'] < self.guardrails['top2_margin_min']:
                row_violations.append("top2_margin")
            if row['latency_ms'] > self.guardrails['latency_p95_max_ms']:
                row_violations.append("latency")
            if row['fanout'] > self.guardrails['max_fanout']:
                row_violations.append("fanout")
            
            violations.extend(row_violations)
        
        # Violation counts
        violation_counts = pd.Series(violations).value_counts()
        
        if not violation_counts.empty:
            fig = px.pie(
                values=violation_counts.values,
                names=violation_counts.index,
                title='Guardrail Violations by Type'
            )
            st.plotly_chart(fig, use_container_width=True)
            
            # Violation details
            st.subheader("Violation Details")
            violation_df = pd.DataFrame({
                'Violation Type': violation_counts.index,
                'Count': violation_counts.values,
                'Percentage': (violation_counts.values / violation_counts.sum() * 100).round(1)
            })
            st.dataframe(violation_df)
        else:
            st.success("🎉 No guardrail violations detected!")
    
    def render_correlation_analysis(self, df):
        """Render correlation analysis"""
        if df.empty:
            st.warning("No data available for correlation analysis")
            return
        
        st.subheader("🔗 Metric Correlations")
        
        # Select numeric columns
        numeric_cols = df.select_dtypes(include=[np.number]).columns.tolist()
        
        if len(numeric_cols) > 1:
            # Correlation matrix
            corr_matrix = df[numeric_cols].corr()
            
            fig = px.imshow(
                corr_matrix,
                text_auto=True,
                aspect="auto",
                title="Correlation Matrix of Metrics",
                color_continuous_scale="RdBu"
            )
            st.plotly_chart(fig, use_container_width=True)
            
            # Pairplot for key metrics
            key_metrics = ['health_score', 'accuracy', 'attention_entropy', 'output_diversity', 'latency_ms']
            available_metrics = [col for col in key_metrics if col in df.columns]
            
            if len(available_metrics) >= 2:
                fig = px.scatter_matrix(
                    df[available_metrics],
                    title="Scatter Matrix of Key Metrics"
                )
                st.plotly_chart(fig, use_container_width=True)
    
    def run(self):
        """Run the dashboard"""
        st.title("🧠 Melvin Regression Dashboard")
        st.markdown("Real-time monitoring of Melvin's performance across commits and validation runs")
        
        # Load data
        with st.spinner("Loading data..."):
            df = self.load_data()
        
        if df.empty:
            st.error("No data found. Please ensure CSV files exist in verification_results/ or huggingface_data/melvin_tests/")
            return
        
        # Render sidebar and apply filters
        filters = self.render_sidebar(df)
        if filters[0] is not None:  # Check if sidebar rendered successfully
            sources, profiles, domains, date_range = filters
            df = self.apply_filters(df, sources, profiles, domains, date_range)
        
        if df.empty:
            st.warning("No data matches the selected filters")
            return
        
        # Render dashboard sections
        self.render_overview_metrics(df)
        
        # Create tabs for different analyses
        tab1, tab2, tab3, tab4 = st.tabs(["📈 Trends", "🎯 Domains", "⚠️ Guardrails", "🔗 Correlations"])
        
        with tab1:
            self.render_health_trends(df)
        
        with tab2:
            self.render_domain_analysis(df)
        
        with tab3:
            self.render_guardrail_analysis(df)
        
        with tab4:
            self.render_correlation_analysis(df)
        
        # Footer
        st.markdown("---")
        st.markdown("**Melvin Regression Dashboard** - Built with Streamlit")

def main():
    dashboard = MelvinDashboard()
    dashboard.run()

if __name__ == "__main__":
    main()
