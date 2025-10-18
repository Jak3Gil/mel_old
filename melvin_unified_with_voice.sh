#!/bin/bash
# Run Melvin Unified with Voice Output
# Melvin will speak its thoughts out loud!

echo "🎙️ Starting Melvin Unified with Voice Output..."
echo "Melvin will speak everything it sees!"
echo ""

# Run melvin_unified and pipe output through voice processor
./melvin_unified 2>&1 | while IFS= read -r line; do
    # Print the line
    echo "$line"
    
    # If it's a thought line, speak it
    if [[ "$line" =~ "💭 Melvin: " ]]; then
        # Extract just the thought text
        thought=$(echo "$line" | sed 's/.*💭 Melvin: //')
        # Speak it in the background so it doesn't block
        say "$thought" &
    fi
done

