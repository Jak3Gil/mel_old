#!/bin/bash

echo ""
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║  👁️  SEE WHAT MELVIN SEES                                ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo ""
echo "Choose visualization mode:"
echo ""
echo "  1) Test Visualization (synthetic scenes, no camera needed)"
echo "  2) Live Camera (use your webcam)"
echo "  3) Read guide"
echo "  4) Exit"
echo ""
read -p "Enter choice [1-4]: " choice

case $choice in
    1)
        echo ""
        echo "▶️  Starting test visualization..."
        echo "   (Press 'q' to quit, 'p' to pause)"
        echo ""
        python3 visualize_melvin.py
        ;;
    2)
        echo ""
        echo "▶️  Starting camera visualization..."
        echo "   (Press 'q' to quit, 'p' to pause, 's' to save)"
        echo ""
        python3 visualize_camera.py
        ;;
    3)
        echo ""
        cat VISUALIZATION_GUIDE.md | head -100
        echo ""
        echo "Full guide: VISUALIZATION_GUIDE.md"
        echo ""
        ;;
    4)
        echo "Goodbye! 👋"
        exit 0
        ;;
    *)
        echo "Invalid choice. Please run again."
        exit 1
        ;;
esac

echo ""
echo "✅ Done!"


