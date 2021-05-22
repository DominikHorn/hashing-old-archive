#!/bin/bash

# Ensure output directory is there
mkdir -p out
rm -rf out/*

# Build and open result folder
python plot_throughput.py && open out/
