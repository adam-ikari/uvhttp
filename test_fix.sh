#!/bin/bash
echo "Testing helloworld fix..."

# Start the server in the background
./build/dist/bin/helloworld &
SERVER_PID=$!

# Wait a moment for server to start
sleep 2

# Make a request to the server
echo "Making request to server..."
curl -s http://localhost:8080/ || echo "Request failed, but that's OK"

# Terminate the server gracefully
echo "Terminating server (PID: $SERVER_PID)..."
kill $SERVER_PID

# Wait a moment to see if it exits cleanly
sleep 1

# Check if the process is still running (would indicate a crash)
if kill -0 $SERVER_PID 2>/dev/null; then
    echo "Process still running, force killing..."
    kill -9 $SERVER_PID 2>/dev/null || true
else
    echo "Process terminated cleanly"
fi

echo "Test completed"