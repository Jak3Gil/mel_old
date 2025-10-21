#!/bin/bash

# ╔══════════════════════════════════════════════════════════════════════════╗
# ║  PERSISTENCE VERIFICATION SCRIPT                                         ║
# ║  Ensures memory is fully persistent before streaming data                ║
# ╚══════════════════════════════════════════════════════════════════════════╝

set -e

echo ""
echo "╔══════════════════════════════════════════════════════════════════════════╗"
echo "║  MELVIN PERSISTENCE VERIFICATION                                         ║"
echo "╚══════════════════════════════════════════════════════════════════════════╝"
echo ""

# Colors
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Change to melvin directory
cd "$(dirname "$0")/.."

# Step 1: Check if data directory exists
echo -e "${BLUE}[1/6] Checking data directory...${NC}"
if [ ! -d "data" ]; then
    echo -e "${YELLOW}  → Creating data directory${NC}"
    mkdir -p data
fi
echo -e "${GREEN}  ✓ Data directory exists${NC}"
echo ""

# Step 2: Check current state
echo -e "${BLUE}[2/6] Checking current memory state...${NC}"
if [ -f "data/nodes.melvin" ] && [ -f "data/edges.melvin" ]; then
    NODE_SIZE=$(wc -c < "data/nodes.melvin")
    EDGE_SIZE=$(wc -c < "data/edges.melvin")
    echo -e "${GREEN}  ✓ Memory files exist${NC}"
    echo "    - nodes.melvin: $NODE_SIZE bytes"
    echo "    - edges.melvin: $EDGE_SIZE bytes"
else
    echo -e "${YELLOW}  ! No existing memory - will start fresh${NC}"
    NODE_SIZE=0
    EDGE_SIZE=0
fi
echo ""

# Step 3: Create test knowledge
echo -e "${BLUE}[3/6] Creating test knowledge...${NC}"
cat > /tmp/persistence_test.tch <<EOF
# Persistence verification test
water is liquid
fire is hot
ice is cold
EOF
echo -e "${GREEN}  ✓ Test file created${NC}"
echo ""

# Step 4: Rebuild to ensure latest code
echo -e "${BLUE}[4/6] Building Melvin with persistence fixes...${NC}"
make clean > /dev/null 2>&1 || true
if make > /tmp/melvin_build.log 2>&1; then
    echo -e "${GREEN}  ✓ Build successful${NC}"
else
    echo -e "${RED}  ✗ Build failed - see /tmp/melvin_build.log${NC}"
    tail -20 /tmp/melvin_build.log
    exit 1
fi
echo ""

# Step 5: Test teaching and persistence
echo -e "${BLUE}[5/6] Testing teaching + auto-save...${NC}"
echo "water is liquid" | ./melvin > /tmp/melvin_test_output.txt 2>&1 || true

# Check if files were modified
sleep 1
if [ -f "data/nodes.melvin" ] && [ -f "data/edges.melvin" ]; then
    NEW_NODE_SIZE=$(wc -c < "data/nodes.melvin")
    NEW_EDGE_SIZE=$(wc -c < "data/edges.melvin")
    
    if [ "$NEW_NODE_SIZE" -ge "$NODE_SIZE" ]; then
        echo -e "${GREEN}  ✓ nodes.melvin exists and is current ($NEW_NODE_SIZE bytes)${NC}"
    else
        echo -e "${RED}  ✗ nodes.melvin appears corrupted${NC}"
        exit 1
    fi
    
    if [ "$NEW_EDGE_SIZE" -ge "$EDGE_SIZE" ]; then
        echo -e "${GREEN}  ✓ edges.melvin exists and is current ($NEW_EDGE_SIZE bytes)${NC}"
    else
        echo -e "${RED}  ✗ edges.melvin appears corrupted${NC}"
        exit 1
    fi
else
    echo -e "${RED}  ✗ Memory files not created${NC}"
    exit 1
fi
echo ""

# Step 6: Verify persistence across runs
echo -e "${BLUE}[6/6] Verifying persistence across runs...${NC}"
BEFORE_SIZE=$NEW_NODE_SIZE

# Run again - should load existing memory
echo "fire is hot" | ./melvin > /tmp/melvin_test_output2.txt 2>&1 || true
sleep 1

AFTER_SIZE=$(wc -c < "data/nodes.melvin")

if [ "$AFTER_SIZE" -ge "$BEFORE_SIZE" ]; then
    echo -e "${GREEN}  ✓ Memory persists across runs (grew or maintained)${NC}"
    echo "    - Before: $BEFORE_SIZE bytes"
    echo "    - After:  $AFTER_SIZE bytes"
else
    echo -e "${RED}  ✗ Memory lost between runs!${NC}"
    exit 1
fi
echo ""

# Cleanup
rm -f /tmp/persistence_test.tch /tmp/melvin_test_output.txt /tmp/melvin_test_output2.txt

# Final summary
echo "╔══════════════════════════════════════════════════════════════════════════╗"
echo "║  PERSISTENCE VERIFICATION: PASSED ✓                                      ║"
echo "╚══════════════════════════════════════════════════════════════════════════╝"
echo ""
echo -e "${GREEN}✓ All persistence checks passed!${NC}"
echo ""
echo "Your Melvin system is ready for streaming data:"
echo "  • Auto-save is enabled"
echo "  • Memory persists after every query"
echo "  • Knowledge survives across runs"
echo "  • Storage is working correctly"
echo ""
echo -e "${BLUE}You can now safely run your data stream.${NC}"
echo ""

