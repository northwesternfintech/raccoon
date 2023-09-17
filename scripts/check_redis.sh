#!/bin/bash

if docker ps | grep -q raccoon-redis-server; then
    echo "Redis container is already running."
elif docker ps -a | grep -q raccoon-redis-server; then
    echo "Starting the existing 'raccoon-redis-server' container..."
    docker start raccoon-redis-server
    echo "'raccoon-redis-server' container started."
    sleep 3
else
    echo "Starting Redis container..."
    docker run -d --name raccoon-redis-server -p 6379:6379 redis:latest
    echo "Redis container started."
    sleep 3
fi
