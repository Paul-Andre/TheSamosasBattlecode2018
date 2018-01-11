#!/bin/bash

# Prepare.
if [[ ! -d build ]]; then
  mkdir build
fi

# Build.
LIBRARIES="-lutil -ldl -lrt -lpthread -lgcc_s -lc -lm -L/battlecode-c/lib/ -lbattlecode"
INCLUDES="-I/battlecode-c/include -Iinclude"
g++ --std=c++14 agent/main.cpp src/* $INCLUDES $LIBRARIES -o build/main

# Run.
./build/main
