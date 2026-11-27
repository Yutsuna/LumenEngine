#!/usr/bin/env bash
set -euo pipefail

readonly NATIVE_FILE="Meson/Native.ini"

function InvokeMeson()
{
    local BuildDir="$1"
    local BuildType="$2"
    local Sanitize="$3"
    local EnableBenchmarking=$([ "$BuildType" = "release" ] && echo true || echo false)

    local MesonArgs=(
        setup "$BuildDir"
        "--buildtype=$BuildType"
        "-Denable_testing=true"
	"-Denable_benchmarking=$EnableBenchmarking"
        --reconfigure
        "--native-file=$NATIVE_FILE"
    )

    if [[ -n "$Sanitize" ]]; then
        MesonArgs+=("-Db_sanitize=$Sanitize")
    fi

    meson "${MesonArgs[@]}" &>/dev/null &
}

Pids=()

InvokeMeson "Build"         "debug"   "address,undefined"
Pids+=("$!")

InvokeMeson "Build-TSAN"    "debug"   "thread"
Pids+=("$!")

InvokeMeson "Build-Release" "release" ""
Pids+=("$!")

Status=0
for Pid in "${Pids[@]}"; do
    wait "$Pid" || Status=1
done

exit "$Status"
