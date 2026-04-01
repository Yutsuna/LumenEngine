#!/bin/bash

readonly TargetDir="LumenEngine"
readonly BuildDir="Build"
readonly Threads=$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)

echo "Launching clang-tidy on $TargetDir ($Threads parallel processes)..."

find "$TargetDir" -type f \( -name "*.cpp" -o -name "*.hpp" -o -name "*.inl" \) -print0 | \
    xargs -0 -P "$Threads" -I {} clang-tidy -p "$BuildDir" --checks='*' {}

echo "Analysis complete."
