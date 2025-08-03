#!/bin/bash

PIDS=()
SYMBOL_GROUPS=("AAPL" "MSFT" "GOOG" "AMZN" "INTC")
BASE_PORT=5000
SCRIPT="server.py"

for i in "${!SYMBOL_GROUPS[@]}"; do
    SYMBOLS="${SYMBOL_GROUPS[$i]}"
    PORT=$((BASE_PORT + i))
    echo "Starting feed for [$SYMBOLS] on port $PORT..."
    python3 "$SCRIPT" --port $PORT --symbols $SYMBOLS &
    PIDS+=($!)
done

# Handle Ctrl+C or kill
trap "echo '🛑 Stopping all servers...'; kill ${PIDS[@]}; exit 0" SIGINT SIGTERM

# Wait for all background processes
wait
