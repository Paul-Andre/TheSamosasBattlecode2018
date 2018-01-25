#!/bin/bash
set -e

make
valgrind ./build/agent
