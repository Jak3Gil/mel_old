#!/usr/bin/env python3
"""
Energy Trajectory Visualization
[Hopfield-Diffusion Upgrade - Visualization]
"""

import sys
import re
import matplotlib.pyplot as plt

def extract_energy_trajectory(logfile):
    """Extract energy values from log file"""
    steps = []
    energies = []
    
    with open(logfile, 'r') as f:
        for line in f:
            # Try multiple patterns
            patterns = [
                r"Step\s+(\d+).*Energy\s*[=:]\s*([-0-9.eE]+)",
                r"\[Diffusion\s+t=(\d+)\].*Energy=([-0-9.eE]+)",
                r"(\d+)\s+\|\s+([-0-9.eE]+)\s+\|"  # Table format
            ]
            
            for pattern in patterns:
                m = re.search(pattern, line)
                if m:
                    steps.append(int(m.group(1)))
                    energies.append(float(m.group(2)))
                    break
    
    return steps, energies

def plot_energy_curve(steps, energies, title="Hopfield-Diffusion Energy Descent"):
    """Plot energy trajectory"""
    plt.figure(figsize=(10, 6))
    
    plt.plot(steps, energies, 'b-', linewidth=2, label='Energy')
    plt.axhline(y=energies[-1] if energies else 0, 
                color='r', linestyle='--', alpha=0.5, label='Final Energy')
    
    plt.xlabel('Diffusion Step', fontsize=12)
    plt.ylabel('System Energy', fontsize=12)
    plt.title(title, fontsize=14, fontweight='bold')
    plt.grid(True, alpha=0.3)
    plt.legend()
    
    # Add convergence annotation
    if len(energies) > 1:
        convergence_rate = abs(energies[-1] - energies[0])
        plt.text(0.02, 0.98, 
                f'Initial: {energies[0]:.4f}\nFinal: {energies[-1]:.4f}\nΔE: {convergence_rate:.4f}',
                transform=plt.gca().transAxes,
                verticalalignment='top',
                bbox=dict(boxstyle='round', facecolor='wheat', alpha=0.5))
    
    plt.tight_layout()
    return plt

def main():
    if len(sys.argv) < 2:
        print("Usage: python3 plot_energy.py <logfile>")
        print("Example: python3 plot_energy.py hopfield_log.txt")
        sys.exit(1)
    
    logfile = sys.argv[1]
    
    print(f"\n📊 Analyzing energy trajectory from: {logfile}\n")
    
    steps, energies = extract_energy_trajectory(logfile)
    
    if not energies:
        print("❌ No energy values found in log file!")
        print("   Make sure the log contains energy information")
        sys.exit(1)
    
    print(f"✅ Found {len(energies)} energy measurements")
    print(f"   Steps: {min(steps)} to {max(steps)}")
    print(f"   Energy range: {min(energies):.6f} to {max(energies):.6f}")
    print(f"   Total descent: {abs(energies[-1] - energies[0]):.6f}")
    
    # Check convergence
    if len(energies) >= 10:
        last_deltas = [abs(energies[i] - energies[i-1]) for i in range(-10, 0)]
        mean_delta = sum(last_deltas) / len(last_deltas)
        print(f"   Mean |ΔE| (last 10): {mean_delta:.6f}")
        
        if mean_delta < 1e-4:
            print(f"   ✅ Converged (threshold: 1e-4)")
        elif mean_delta < 1e-3:
            print(f"   ✅ Nearly converged (threshold: 1e-3)")
        else:
            print(f"   ⚠️  Still descending (consider more steps)")
    
    print(f"\n📈 Generating plot...")
    
    plt = plot_energy_curve(steps, energies)
    
    # Save
    output_file = logfile.replace('.txt', '_energy.png')
    plt.savefig(output_file, dpi=150)
    print(f"   ✅ Saved to: {output_file}")
    
    # Show
    print(f"\n   Opening plot...")
    plt.show()
    
    print(f"\n✅ Done!\n")

if __name__ == "__main__":
    main()




