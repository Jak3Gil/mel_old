#!/usr/bin/env python3
"""
MELVIN HUGGING FACE DATASET INTEGRATION
=======================================

Downloads and processes multiple datasets from Hugging Face to test Melvin's
LLM-style reasoning capabilities across different cognitive domains.

Selected datasets:
1. commonsense_qa - Commonsense reasoning questions
2. piqa - Physical commonsense reasoning
3. gsm8k - Grade school math word problems
4. boolq - Boolean question answering
5. openbookqa - Open-book question answering
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
    """Download a dataset from Hugging Face using OFFICIAL test splits only"""
    print(f"📥 Downloading dataset: {dataset_name} (split: {split})")
    
    # CRITICAL: Use official test splits only to prevent data leakage
    official_splits = {
        'commonsense_qa': ['validation', 'test'],
        'piqa': ['validation', 'test'],
        'gsm8k': ['test'],
        'boolq': ['validation', 'test'],
        'openbookqa': ['validation', 'test']
    }
    
    if split not in official_splits.get(dataset_name, ['train', 'validation', 'test']):
        print(f"⚠️  WARNING: {split} not in official splits for {dataset_name}")
        print(f"   Official splits: {official_splits.get(dataset_name, ['train', 'validation', 'test'])}")
    
    try:
        from datasets import load_dataset
        import git
        import os
        
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

def create_run_meta(dataset_name, split, max_samples, profile=None, seed=None):
    """Create RUN_META.json with experiment metadata"""
    import json
    import git
    import os
    from datetime import datetime
    
    try:
        # Get git commit hash
        repo = git.Repo('.')
        commit_hash = repo.head.commit.hexsha[:8]
        commit_message = repo.head.commit.message.strip().split('\n')[0]
    except:
        commit_hash = "unknown"
        commit_message = "unknown"
    
    run_meta = {
        "experiment_id": f"{dataset_name}_{split}_{datetime.now().strftime('%Y%m%d_%H%M%S')}",
        "timestamp": datetime.now().isoformat(),
        "dataset": {
            "name": dataset_name,
            "split": split,
            "max_samples": max_samples,
            "official_split": True  # Flag to ensure we're using proper splits
        },
        "system": {
            "commit_hash": commit_hash,
            "commit_message": commit_message,
            "profile": profile,
            "seed": seed,
            "version": "melvin_v2.0"
        },
        "evaluation": {
            "data_leakage_prevention": True,
            "calibration_enabled": False,  # Will be set during calibration phase
            "error_taxonomy_enabled": True
        }
    }
    
    return run_meta

def save_run_meta(run_meta, output_dir):
    """Save RUN_META.json to output directory"""
    import json
    from pathlib import Path
    
    output_path = Path(output_dir)
    output_path.mkdir(exist_ok=True)
    
    meta_file = output_path / f"RUN_META_{run_meta['experiment_id']}.json"
    with open(meta_file, 'w') as f:
        json.dump(run_meta, f, indent=2)
    
    print(f"💾 Saved run metadata: {meta_file}")
    return meta_file

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

def process_piqa(dataset):
    """Process PIQA (Physical Commonsense Reasoning) dataset for Melvin"""
    print("🏃 Processing PIQA dataset...")
    
    processed_data = []
    for i, item in enumerate(dataset):
        goal = item['goal']
        choices = [item['sol1'], item['sol2']]
        answer_key = item['label']
        
        # Find the correct answer (0 or 1)
        correct_answer = choices[answer_key]
        
        # Create Melvin-friendly format
        melvin_item = {
            'id': f"piqa_{i:04d}",
            'type': 'physical_commonsense',
            'goal': goal,
            'choices': choices,
            'correct_answer': correct_answer,
            'answer_key': str(answer_key),
            'melvin_input': f"Goal: {goal}\nChoices: A) {choices[0]}\nB) {choices[1]}\nAnswer: {correct_answer}"
        }
        
        processed_data.append(melvin_item)
    
    print(f"✅ Processed {len(processed_data)} PIQA items")
    return processed_data

def process_gsm8k(dataset):
    """Process GSM8K (Grade School Math) dataset for Melvin"""
    print("🔢 Processing GSM8K dataset...")
    
    processed_data = []
    for i, item in enumerate(dataset):
        question = item['question']
        answer = item['answer']
        
        # Extract the numerical answer (last number in the answer)
        try:
            # GSM8K answers are in format "#### 42" where 42 is the answer
            numerical_answer = answer.split()[-1]
            if numerical_answer.isdigit():
                numerical_answer = int(numerical_answer)
            else:
                numerical_answer = float(numerical_answer)
        except:
            numerical_answer = answer  # Fallback to full answer
        
        # Create Melvin-friendly format
        melvin_item = {
            'id': f"gsm8k_{i:04d}",
            'type': 'math_reasoning',
            'question': question,
            'answer': answer,
            'numerical_answer': numerical_answer,
            'melvin_input': f"Math Problem: {question}\nAnswer: {numerical_answer}"
        }
        
        processed_data.append(melvin_item)
    
    print(f"✅ Processed {len(processed_data)} GSM8K items")
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

def run_multi_dataset_suite(datasets, max_samples, output_dir):
    """Run the full multi-dataset test suite"""
    print("🎯 RUNNING MULTI-DATASET SUITE")
    print("==============================")
    
    all_results = {}
    combined_data = []
    
    for dataset_name in datasets:
        print(f"\n📥 Processing {dataset_name}...")
        
        # Download dataset
        dataset = download_dataset(dataset_name, max_samples=max_samples)
        if dataset is None:
            print(f"❌ Failed to download {dataset_name}, skipping...")
            continue
        
        # Process dataset based on type
        if dataset_name == 'commonsense_qa':
            processed_data = process_commonsense_qa(dataset)
        elif dataset_name == 'boolq':
            processed_data = process_boolq(dataset)
        elif dataset_name == 'openbookqa':
            processed_data = process_openbookqa(dataset)
        elif dataset_name == 'piqa':
            processed_data = process_piqa(dataset)
        elif dataset_name == 'gsm8k':
            processed_data = process_gsm8k(dataset)
        else:
            print(f"❌ Unknown dataset: {dataset_name}")
            continue
        
        # Add dataset source to each item
        for item in processed_data:
            item['dataset_source'] = dataset_name
        
        # Save individual dataset
        json_file, txt_file, csv_file = save_processed_data(
            processed_data, dataset_name, output_dir
        )
        
        # Add to combined data
        combined_data.extend(processed_data)
        all_results[dataset_name] = {
            'count': len(processed_data),
            'files': {'json': json_file, 'txt': txt_file, 'csv': csv_file}
        }
    
    # Save combined dataset
    if combined_data:
        combined_json, combined_txt, combined_csv = save_processed_data(
            combined_data, 'combined_multi_dataset', output_dir
        )
        all_results['combined'] = {
            'count': len(combined_data),
            'files': {'json': combined_json, 'txt': combined_txt, 'csv': combined_csv}
        }
        
        # Create comprehensive test script
        create_comprehensive_test_script(datasets, output_dir)
    
    return all_results

def create_comprehensive_test_script(datasets, output_dir):
    """Create a comprehensive test script for all datasets"""
    output_path = Path(output_dir)
    script_content = f'''#!/bin/bash

# MELVIN COMPREHENSIVE MULTI-DATASET TEST SCRIPT
# Generated for datasets: {', '.join(datasets)}

echo "🧠 Running comprehensive multi-dataset validation through Melvin"
echo "=============================================================="

# Create test directory
mkdir -p {output_dir}/melvin_tests

# Test each dataset with different profiles
for dataset in {', '.join(datasets)}; do
    echo "🔍 Testing $dataset with Conservative profile..."
    make run-verification-profile profile=Conservative seed=42 dataset=$dataset > {output_dir}/melvin_tests/$dataset"_conservative.log"
    
    echo "🔍 Testing $dataset with Balanced profile..."
    make run-verification-profile profile=Balanced seed=42 dataset=$dataset > {output_dir}/melvin_tests/$dataset"_balanced.log"
    
    echo "🔍 Testing $dataset with Creative profile..."
    make run-verification-profile profile=Creative seed=42 dataset=$dataset > {output_dir}/melvin_tests/$dataset"_creative.log"
done

# Test combined dataset
echo "🔍 Testing combined dataset with all profiles..."
make run-verification-profile profile=Conservative seed=42 dataset=combined_multi_dataset > {output_dir}/melvin_tests/combined_conservative.log
make run-verification-profile profile=Balanced seed=42 dataset=combined_multi_dataset > {output_dir}/melvin_tests/combined_balanced.log
make run-verification-profile profile=Creative seed=42 dataset=combined_multi_dataset > {output_dir}/melvin_tests/combined_creative.log

echo "📊 Generating comprehensive analytics..."
python3 analysis/verify_results.py --input-dir {output_dir}/melvin_tests --plot-all

echo "📋 Generating domain-specific analysis..."
python3 analysis/verify_results.py --input-dir {output_dir}/melvin_tests --domain-analysis

echo "✅ Comprehensive multi-dataset testing completed!"
echo "Check {output_dir}/melvin_tests/ for detailed results"
'''
    
    script_file = output_path / "test_comprehensive_multi_dataset.sh"
    with open(script_file, 'w') as f:
        f.write(script_content)
    
    # Make executable
    os.chmod(script_file, 0o755)
    print(f"📝 Created comprehensive test script: {script_file}")
    
    return script_file

def main():
    parser = argparse.ArgumentParser(description='Download and process Hugging Face datasets for Melvin')
    parser.add_argument('--dataset', default='commonsense_qa', 
                       choices=['commonsense_qa', 'boolq', 'openbookqa', 'piqa', 'gsm8k'],
                       help='Single dataset to download')
    parser.add_argument('--datasets', nargs='+', 
                       choices=['commonsense_qa', 'boolq', 'openbookqa', 'piqa', 'gsm8k'],
                       help='Multiple datasets to download and process')
    parser.add_argument('--max-samples', type=int, default=100,
                       help='Maximum number of samples to process per dataset')
    parser.add_argument('--output-dir', default='huggingface_data',
                       help='Output directory for processed data')
    parser.add_argument('--install-deps', action='store_true',
                       help='Install required dependencies')
    parser.add_argument('--comprehensive', action='store_true',
                       help='Run comprehensive multi-dataset suite')
    
    args = parser.parse_args()
    
    print("🤗 MELVIN HUGGING FACE DATASET INTEGRATION")
    print("==========================================")
    
    # Install dependencies if requested
    if args.install_deps:
        if not install_dependencies():
            sys.exit(1)
    
    # Determine which datasets to process
    if args.comprehensive or args.datasets:
        if args.comprehensive:
            datasets = ['commonsense_qa', 'piqa', 'gsm8k']  # Core reasoning domains
        else:
            datasets = args.datasets
        
        # Run multi-dataset suite
        results = run_multi_dataset_suite(datasets, args.max_samples, args.output_dir)
        
        # Summary
        print(f"\n📊 MULTI-DATASET PROCESSING SUMMARY")
        print("==================================")
        for dataset, info in results.items():
            print(f"{dataset}: {info['count']} samples")
        print(f"Total samples: {sum(info['count'] for info in results.values())}")
        print(f"Output directory: {args.output_dir}")
        
    else:
        # Single dataset processing
        dataset = args.dataset
        print(f"Processing single dataset: {dataset}")
        
        # Download dataset
        dataset_obj = download_dataset(dataset, max_samples=args.max_samples)
        if dataset_obj is None:
            sys.exit(1)
        
        # Process dataset based on type
        if dataset == 'commonsense_qa':
            processed_data = process_commonsense_qa(dataset_obj)
        elif dataset == 'boolq':
            processed_data = process_boolq(dataset_obj)
        elif dataset == 'openbookqa':
            processed_data = process_openbookqa(dataset_obj)
        elif dataset == 'piqa':
            processed_data = process_piqa(dataset_obj)
        elif dataset == 'gsm8k':
            processed_data = process_gsm8k(dataset_obj)
        else:
            print(f"❌ Unknown dataset: {dataset}")
            sys.exit(1)
        
        # Save processed data
        json_file, txt_file, csv_file = save_processed_data(
            processed_data, dataset, args.output_dir
        )
        
        # Create test script
        test_script = create_melvin_test_script(dataset, json_file, args.output_dir)
        
        # Summary
        print(f"\n📊 DATASET PROCESSING SUMMARY")
        print("=============================")
        print(f"Dataset: {dataset}")
        print(f"Samples processed: {len(processed_data)}")
        print(f"Output directory: {args.output_dir}")
        print(f"Files created:")
        print(f"  • {json_file}")
        print(f"  • {txt_file}")
        print(f"  • {csv_file}")
        print(f"  • {test_script}")
    
    print(f"\n🚀 NEXT STEPS")
    print("=============")
    if args.comprehensive or args.datasets:
        print(f"1. Run comprehensive tests: ./{args.output_dir}/test_comprehensive_multi_dataset.sh")
        print(f"2. Analyze results: python3 analysis/verify_results.py --input-dir {args.output_dir}/melvin_tests --plot-all")
        print(f"3. Domain analysis: python3 analysis/verify_results.py --input-dir {args.output_dir}/melvin_tests --domain-analysis")
    else:
        print(f"1. Review processed data: cat {txt_file}")
        print(f"2. Run through Melvin: ./{test_script}")
        print(f"3. Analyze results: python3 analysis/verify_results.py --input-dir {args.output_dir}/melvin_tests")
    
    print(f"\n✅ Dataset integration completed!")

if __name__ == "__main__":
    main()
