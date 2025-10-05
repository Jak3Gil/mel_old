#!/bin/bash

# MELVIN DATASET TEST SCRIPT
# Generated for commonsense_qa

echo "🧠 Running commonsense_qa through Melvin's pipeline"
echo "================================================="

# Create test directory
mkdir -p huggingface_data/melvin_tests

# Test with different profiles
echo "🔍 Testing with Conservative profile..."
make run-verification-profile profile=Conservative seed=42 > huggingface_data/melvin_tests/conservative_test.log

echo "🔍 Testing with Balanced profile..."
make run-verification-profile profile=Balanced seed=42 > huggingface_data/melvin_tests/balanced_test.log

echo "🔍 Testing with Creative profile..."
make run-verification-profile profile=Creative seed=42 > huggingface_data/melvin_tests/creative_test.log

echo "📊 Generating analytics..."
python3 analysis/verify_results_lite.py --input-dir huggingface_data/melvin_tests --plot-all

echo "✅ Dataset testing completed!"
echo "Check huggingface_data/melvin_tests/ for results"
