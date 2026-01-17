#!/bin/bash
# Generate version_history.txt for the auto-update system
# This script should be run during deployment to create the version history file
# that allows clients to know how many commits behind they are.

set -e

OUTPUT_FILE="${1:-version_history.txt}"
MAX_ENTRIES="${2:-50}"

echo "Generating version history (last $MAX_ENTRIES commits)..."

# Generate the version history file with recent git short hashes
git log --format="%h" -n "$MAX_ENTRIES" > "$OUTPUT_FILE"

ENTRY_COUNT=$(wc -l < "$OUTPUT_FILE")
echo "Generated $OUTPUT_FILE with $ENTRY_COUNT entries"

# Show the first few entries
echo "Recent versions:"
head -5 "$OUTPUT_FILE" | while read -r hash; do
    # Get commit message for each hash
    msg=$(git log --format="%s" -n 1 "$hash" 2>/dev/null || echo "unknown")
    echo "  $hash - $msg"
done

echo "..."
echo "Done!"
