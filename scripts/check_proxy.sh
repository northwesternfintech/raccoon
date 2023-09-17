#!/bin/bash

if docker ps | grep -q raccoon-proxy; then
    echo "Raccoon proxy container is already running."
elif docker ps -a | grep -q raccoon-proxy; then
    echo "Starting the existing 'raccoon-proxy' container..."
    docker start raccoon-proxy
    echo "'raccoon-proxy' container started."
    sleep 3
else
    echo "Starting raccoon proxy container..."
    docker run -d --name raccoon-proxy -p 8675:8675 stevenewald/raccoon-proxy:latest
    echo "Raccoon-proxy container started."
    sleep 3
fi
