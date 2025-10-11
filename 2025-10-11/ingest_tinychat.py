#!/usr/bin/env python3
"""
Download TinyChat dataset and prepare for Melvin ingestion
"""

import sys
import json
from pathlib import Path

def download_tinychat(output_file="tinychat_data.txt", limit=10000):
    """Download TinyChat dataset from HuggingFace."""
    try:
        from datasets import load_dataset
        print(f"📥 Downloading TinyChat dataset (limit: {limit})...")
        
        dataset = load_dataset("starhopp3r/TinyChat", split="train", streaming=True)
        
        count = 0
        with open(output_file, 'w', encoding='utf-8') as f:
            for item in dataset:
                if count >= limit:
                    break
                
                text = item['text']
                # Extract conversation turns
                text = text.replace('[INST]', '').replace('[/INST]', '')
                f.write(text.strip() + '\n')
                
                count += 1
                if count % 1000 == 0:
                    print(f"  Downloaded {count}/{limit} conversations...")
        
        print(f"✅ Downloaded {count} conversations to {output_file}")
        return True
        
    except ImportError:
        print("❌ 'datasets' library not found. Installing...")
        import subprocess
        subprocess.check_call([sys.executable, "-m", "pip", "install", "datasets"])
        print("✅ Installed. Please run again.")
        return False
    except Exception as e:
        print(f"❌ Error downloading: {e}")
        return False

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()
    parser.add_argument("--limit", type=int, default=10000, help="Number of conversations to download")
    parser.add_argument("--output", default="tinychat_data.txt", help="Output file")
    args = parser.parse_args()
    
    download_tinychat(args.output, args.limit)

