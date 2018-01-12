#!/bin/bash

# Prepare.
if [[ ! -d build ]]; then
  mkdir build
fi

# Build.
PLATFORM="$(uname -s)"
case "${PLATFORM}" in
  Linux*)
    LIBRARIES="-lbattlecode-linux -lutil -ldl -lrt -pthread -lgcc_s -lc -lm"
    ;;
  Darwin*)
    LIBRARIES="-lbattlecode-darwin -lSystem -lresolv -lc -lm"
    ;;
  *)
    echo "Unsupported platform '${PLATFORM}'"
    exit 1
    ;;
esac
LIBRARIES="${LIBRARIES} -L../battlecode/c/lib"
INCLUDES="-I../battlecode/c/include -Iinclude"
g++ --std=c++14 agent/main.cpp src/* ${INCLUDES} ${LIBRARIES} -o build/main

# Run.
./build/main
