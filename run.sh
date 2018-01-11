#!/bin/bash

# Prepare.
if [[ ! -d build ]]; then
  mkdir build
fi
cd build

# Build.
cmake ..
make

# Run.
./agent
