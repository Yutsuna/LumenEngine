#!/bin/bash

set -e

readonly ROOT_DIR="$(pwd)"
readonly TSAN_CONF="$ROOT_DIR/Debug/TSanSuppression.conf"
readonly BINARY_PATH="Examples/TriangleExample/TriangleExample_d"
readonly TIMEOUT_VAL="5s"

readonly SANITIZERS_NAMES=("asan" "tsan" "ubsan")

readonly ERROR_CODE=84

function Error()
{
    echo "[Error] $1"
    exit $ERROR_CODE
}

function Success()
{
    echo "[Success] $1"
}

function RunSanitizerChecks()
{
    echo "-------------------------------------------------------"
    echo "Running Build and Test for: $Sanitizer"
    echo "-------------------------------------------------------"

    echo "[Step 1] Building with $Sanitizer..."
    lumen build.debug --sanitizer "$Sanitizer"

    local Target="./Build/Debug-$Sanitizer/$BINARY_PATH"

    export TSAN_OPTIONS=""
    
    if [ "$Sanitizer" == "tsan" ]; then
        echo "[Config] Applying TSan suppressions..."
        export TSAN_OPTIONS="suppressions=$TSAN_CONF print_suppressions=1"
    fi

    if [ -f "$Target" ]; then
        echo "[Step 2] Executing $Target (Timeout: $TIMEOUT_VAL)..."
        timeout --preserve-status "$TIMEOUT_VAL" "$Target"
        
        Success "Sanitizer $Sanitizer checks passed!"
    else
        Error "Binary not found at $Target"
    fi
}

if [ -z "$IN_NIX_SHELL" ] && [ -z "$IN_NIX_DEVELOP" ]; then
    Error "This script must be run inside Lumen's Nix environement"
fi

for Sanitizer in "${SANITIZERS_NAMES[@]}"; do
    RunSanitizerChecks
done

echo "-------------------------------------------------------"
echo "All sanitizer checks passed successfully!"
echo "-------------------------------------------------------"
