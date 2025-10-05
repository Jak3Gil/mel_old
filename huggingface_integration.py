#!/usr/bin/env python3
"""
MELVIN HUGGING FACE DATASET INTEGRATION
=======================================

Downloads and processes a dataset from Hugging Face to test Melvin's
LLM-style reasoning capabilities with real data.

Selected datasets:
1. commonsense_qa - Commonsense reasoning questions
2. boolq - Boolean question answering
3. openbookqa - Open-book question answering
"""

import os
import sys
import json
import subprocess
from pathlib import Path
import argparse

def install_dependencies():
    """Install required Python packages"""
    print("📦 Installing dependencies...")
    try:
        subprocess.check_call([sys.executable, "-m", "pip", "install", "datasets", "transformers"])
        print("✅ Dependencies installed successfully")
        return True
    except subprocess.CalledProcessError as e:
        print(f"❌ Failed to install dependencies: {e}")
        return False

def download_dataset(dataset_name, split="train", max_samples=100):
    """Download a dataset from Hugging Face"""
    print(f"📥 Downloading dataset: {dataset_name}")
    
    try:
        from datasets import load_dataset
        
        # Load the dataset
        dataset = load_dataset(dataset_name, split=split)
        
        # Limit samples for testing
        if len(dataset) > max_samples:
            dataset = dataset.select(range(max_samples))
        
        print(f"✅ Downloaded {len(dataset)} samples from {dataset_name}")
        return dataset
        
    except Exception as e:
        print(f"❌ Failed to download dataset: {e}")
        return None

def process_commonsense_qa(dataset):
    """Process CommonsenseQA dataset for Melvin"""
    print("🧠 Processing CommonsenseQA dataset...")
    
    processed_data = []
    for i, item in enumerate(dataset):
        # Extract question and choices
        question = item['question']
        choices = item['choices']['text']
        answer_key = item['answerKey']
        
        # Find the correct answer
        correct_answer = choices[ord(answer_key) - ord('A')]
        
        # Create Melvin-friendly format
        melvin_item = {
            'id': f"csqa_{i:04d}",
            'type': 'commonsense_reasoning',
            'question': question,
            'choices': choices,
            'correct_answer': correct_answer,
            'answer_key': answer_key,
            'melvin_input': f"Question: {question}\nChoices: {', '.join(choices)}\nAnswer: {correct_answer}"
        }
        
        processed_data.append(melvin_item)
    
    print(f"✅ Processed {len(processed_data)} CommonsenseQA items")
    return processed_data

def process_boolq(dataset):
    """Process BoolQ dataset for Melvin"""
    print("❓ Processing BoolQ dataset...")
    
    processed_data = []
    for i, item in enumerate(dataset):
        question = item['question']
        passage = item['passage']
        answer = item['answer']
        
        # Create Melvin-friendly format
        melvin_item = {
            'id': f"boolq_{i:04d}",
            'type': 'boolean_qa',
            'question': question,
            'passage': passage,
            'answer': answer,
            'melvin_input': f"Passage: {passage}\nQuestion: {question}\nAnswer: {'Yes' if answer else 'No'}"
        }
        
        processed_data.append(melvin_item)
    
    print(f"✅ Processed {len(processed_data)} BoolQ items")
    return processed_data

def process_openbookqa(dataset):
    """Process OpenBookQA dataset for Melvin"""
    print("📚 Processing OpenBookQA dataset...")
    
    processed_data = []
    for i, item in enumerate(dataset):
        question = item['question_stem']
        choices = item['choices']['text']
        answer_key = item['answerKey']
        
        # Find the correct answer
        correct_answer = choices[ord(answer_key) - ord('A')]
        
        # Create Melvin-friendly format
        melvin_item = {
            'id': f"obqa_{i:04d}",
            'type': 'openbook_qa',
            'question': question,
            'choices': choices,
            'correct_answer': correct_answer,
            'answer_key': answer_key,
            'melvin_input': f"Question: {question}\nChoices: {', '.join(choices)}\nAnswer: {correct_answer}"
        }
        
        processed_data.append(melvin_item)
    
    print(f"✅ Processed {len(processed_data)} OpenBookQA items")
    return processed_data

def save_processed_data(data, dataset_name, output_dir):
    """Save processed data in multiple formats"""
    output_path = Path(output_dir)
    output_path.mkdir(exist_ok=True)
    
    # Save as JSON
    json_file = output_path / f"{dataset_name}_processed.json"
    with open(json_file, 'w') as f:
        json.dump(data, f, indent=2)
    print(f"💾 Saved JSON: {json_file}")
    
    # Save as text for Melvin input
    txt_file = output_path / f"{dataset_name}_melvin_input.txt"
    with open(txt_file, 'w') as f:
        for item in data:
            f.write(item['melvin_input'] + "\n\n")
    print(f"💾 Saved text: {txt_file}")
    
    # Save as CSV for analytics
    csv_file = output_path / f"{dataset_name}_melvin_input.csv"
    import pandas as pd
    df = pd.DataFrame(data)
    df.to_csv(csv_file, index=False)
    print(f"💾 Saved CSV: {csv_file}")
    
    return json_file, txt_file, csv_file

def create_melvin_test_script(dataset_name, data_file, output_dir):
    """Create a test script to run data through Melvin"""
    output_path = Path(output_dir)
    script_content = f'''#!/bin/bash

# MELVIN DATASET TEST SCRIPT
# Generated for {dataset_name}

echo "🧠 Running {dataset_name} through Melvin's pipeline"
echo "================================================="

# Create test directory
mkdir -p {output_dir}/melvin_tests

# Test with different profiles
echo "🔍 Testing with Conservative profile..."
make run-verification-profile profile=Conservative seed=42 > {output_dir}/melvin_tests/conservative_test.log

echo "🔍 Testing with Balanced profile..."
make run-verification-profile profile=Balanced seed=42 > {output_dir}/melvin_tests/balanced_test.log

echo "🔍 Testing with Creative profile..."
make run-verification-profile profile=Creative seed=42 > {output_dir}/melvin_tests/creative_test.log

echo "📊 Generating analytics..."
python3 analysis/verify_results_lite.py --input-dir {output_dir}/melvin_tests --plot-all

echo "✅ Dataset testing completed!"
echo "Check {output_dir}/melvin_tests/ for results"
'''
    
    script_file = output_path / f"test_{dataset_name}_with_melvin.sh"
    with open(script_file, 'w') as f:
        f.write(script_content)
    
    # Make executable
    os.chmod(script_file, 0o755)
    print(f"📝 Created test script: {script_file}")
    
    return script_file

def main():
    parser = argparse.ArgumentParser(description='Download and process Hugging Face datasets for Melvin')
    parser.add_argument('--dataset', default='commonsense_qa', 
                       choices=['commonsense_qa', 'boolq', 'openbookqa'],
                       help='Dataset to download')
    parser.add_argument('--max-samples', type=int, default=100,
                       help='Maximum number of samples to process')
    parser.add_argument('--output-dir', default='huggingface_data',
                       help='Output directory for processed data')
    parser.add_argument('--install-deps', action='store_true',
                       help='Install required dependencies')
    
    args = parser.parse_args()
    
    print("🤗 MELVIN HUGGING FACE DATASET INTEGRATION")
    print("==========================================")
    
    # Install dependencies if requested
    if args.install_deps:
        if not install_dependencies():
            sys.exit(1)
    
    # Download dataset
    dataset = download_dataset(args.dataset, max_samples=args.max_samples)
    if dataset is None:
        sys.exit(1)
    
    # Process dataset based on type
    if args.dataset == 'commonsense_qa':
        processed_data = process_commonsense_qa(dataset)
    elif args.dataset == 'boolq':
        processed_data = process_boolq(dataset)
    elif args.dataset == 'openbookqa':
        processed_data = process_openbookqa(dataset)
    else:
        print(f"❌ Unknown dataset: {args.dataset}")
        sys.exit(1)
    
    # Save processed data
    json_file, txt_file, csv_file = save_processed_data(
        processed_data, args.dataset, args.output_dir
    )
    
    # Create test script
    test_script = create_melvin_test_script(args.dataset, json_file, args.output_dir)
    
    # Summary
    print(f"\n📊 DATASET PROCESSING SUMMARY")
    print("=============================")
    print(f"Dataset: {args.dataset}")
    print(f"Samples processed: {len(processed_data)}")
    print(f"Output directory: {args.output_dir}")
    print(f"Files created:")
    print(f"  • {json_file}")
    print(f"  • {txt_file}")
    print(f"  • {csv_file}")
    print(f"  • {test_script}")
    
    print(f"\n🚀 NEXT STEPS")
    print("=============")
    print(f"1. Review processed data: cat {txt_file}")
    print(f"2. Run through Melvin: ./{test_script}")
    print(f"3. Analyze results: python3 analysis/verify_results_lite.py --input-dir {args.output_dir}/melvin_tests")
    
    print(f"\n✅ Dataset integration completed!")

if __name__ == "__main__":
    main()
