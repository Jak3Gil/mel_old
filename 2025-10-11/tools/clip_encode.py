#!/usr/bin/env python3
"""
MELVIN CLIP ENCODER

Encodes images to CLIP embeddings for cross-modal reasoning.
Can be replaced with mock mode for testing without CLIP installation.

Usage:
  python clip_encode.py image.jpg                    # Output to stdout
  python clip_encode.py image.jpg --output emb.json  # Output to file
  python clip_encode.py --mock dog.jpg               # Mock mode

Requirements (for real CLIP):
  pip install torch torchvision clip-by-openai pillow
"""

import sys
import json
import hashlib
import argparse
from pathlib import Path

# Try to import CLIP (optional)
try:
    import torch
    import clip
    from PIL import Image
    CLIP_AVAILABLE = True
except ImportError:
    CLIP_AVAILABLE = False

def generate_mock_embedding(image_path: str, dim: int = 512) -> list:
    """Generate deterministic mock embedding based on filename."""
    # Hash filename to get deterministic seed
    seed = int(hashlib.md5(image_path.encode()).hexdigest(), 16) % (2**32)
    
    # Generate pseudo-random embedding
    import random
    random.seed(seed)
    
    embedding = [random.gauss(0, 1) for _ in range(dim)]
    
    # Normalize to unit vector
    norm = sum(x*x for x in embedding) ** 0.5
    embedding = [x / norm for x in embedding]
    
    return embedding

def encode_with_clip(image_path: str, model_name: str = "ViT-B/32") -> list:
    """Encode image using real CLIP model."""
    if not CLIP_AVAILABLE:
        raise RuntimeError("CLIP not available. Install with: pip install clip-by-openai")
    
    # Load model
    device = "cuda" if torch.cuda.is_available() else "cpu"
    model, preprocess = clip.load(model_name, device=device)
    
    # Load and preprocess image
    image = Image.open(image_path)
    image_input = preprocess(image).unsqueeze(0).to(device)
    
    # Get embedding
    with torch.no_grad():
        image_features = model.encode_image(image_input)
        image_features /= image_features.norm(dim=-1, keepdim=True)
    
    # Convert to list
    embedding = image_features.cpu().numpy()[0].tolist()
    
    return embedding

def main():
    parser = argparse.ArgumentParser(description="Encode images to CLIP embeddings")
    parser.add_argument("image_path", help="Path to image file")
    parser.add_argument("--output", "-o", help="Output JSON file (default: stdout)")
    parser.add_argument("--mock", action="store_true", help="Use mock encoding (no CLIP needed)")
    parser.add_argument("--model", default="ViT-B/32", help="CLIP model name")
    parser.add_argument("--dim", type=int, default=512, help="Embedding dimension (for mock)")
    
    args = parser.parse_args()
    
    # Check if image exists
    if not Path(args.image_path).exists():
        print(f"Error: Image not found: {args.image_path}", file=sys.stderr)
        sys.exit(1)
    
    # Encode image
    try:
        if args.mock or not CLIP_AVAILABLE:
            if not args.mock and not CLIP_AVAILABLE:
                print("Warning: CLIP not available, using mock encoding", file=sys.stderr)
            embedding = generate_mock_embedding(args.image_path, args.dim)
            mode = "mock"
        else:
            embedding = encode_with_clip(args.image_path, args.model)
            mode = "clip"
    except Exception as e:
        print(f"Error encoding image: {e}", file=sys.stderr)
        sys.exit(1)
    
    # Create output
    output = {
        "image_path": args.image_path,
        "embedding": embedding,
        "dim": len(embedding),
        "mode": mode
    }
    
    # Write output
    if args.output:
        with open(args.output, 'w') as f:
            json.dump(output, f, indent=2)
        print(f"Embedding written to {args.output}", file=sys.stderr)
    else:
        print(json.dumps(output))

if __name__ == "__main__":
    main()

