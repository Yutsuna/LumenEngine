#!/usr/bin/env bash

set -euo pipefail

function FindProjectRoot()
{
    local CurrentDir="$PWD"

    while [[ "$CurrentDir" != "/" ]]; do
        if [[ -f "${CurrentDir}/flake.nix" && -f "${CurrentDir}/VERSION.md" ]]; then
            echo "${CurrentDir}"
            return 0
        fi
        CurrentDir="$(dirname "${CurrentDir}")"
    done

    echo "$PWD"
}

readonly ProjectRoot="$(FindProjectRoot)"
readonly BuildBaseDir="${ProjectRoot}/Build"
readonly ExampleRelativePath="Examples/BaseExample/BaseExample"
readonly NinjaGenerator="Ninja"

readonly ColorReset="\033[0m"
readonly ColorGreen="\033[0;32m"
readonly ColorBlue="\033[0;34m"
readonly ColorRed="\033[0;31m"
readonly ColorYellow="\033[1;33m"

readonly LogInfo="${ColorBlue}[INFO]${ColorReset} "
readonly LogSuccess="${ColorGreen}[SUCCESS]${ColorReset} "
readonly LogError="${ColorRed}[ERROR]${ColorReset} "
readonly LogWarn="${ColorYellow}[WARN]${ColorReset} "

readonly ErrorCode=84

function PrintLog()
{
    local -r Prefix="$1"
    local -r Message="$2"
    printf "%b%s\n" "${Prefix}" "${Message}"
}

function ShowHelp()
{
    cat << EOF
LumenEngine Build System

Usage: lumen [Command] [Mode]
       lumen [Mode]

Commands:
  build   <Mode>  Configure and compile the project (Default)
  run     <Mode>  Execute the BaseExample (build if needed)
  clean           Remove all build artifacts
  rebuild <Mode>  Full cleanup followed by a fresh build

Modes:
  debug, release, relwithdebinfo
  - Default mode: debug
  - Inside nix shell (IN_NIX_SHELL): default mode is release
  - Shorthand: 'lumen release' is equivalent to 'lumen build release'
EOF
}

function GetDefaultBuildMode()
{
    if [[ -n "${IN_NIX_SHELL:-}" ]]; then
        echo "release"
    else
        echo "debug"
    fi
}

function GetBuildMode()
{
    local -r RawInput="${1:-Debug}"
    case "${RawInput,,}" in
        release) echo "Release" ;;
        relwithdebinfo|relwithdeb) echo "RelWithDebInfo" ;;
        debug) echo "Debug" ;;
        *)
            PrintLog "${LogWarn}" "Unknown mode '${RawInput}', falling back to Debug"
            echo "Debug"
            ;;
    esac
}

function IsModeToken()
{
    local -r RawInput="${1:-}"
    case "${RawInput,,}" in
        release|relwithdebinfo|relwithdeb|debug) return 0 ;;
        *) return 1 ;;
    esac
}

function GetProcessorCount()
{
    getconf _NPROCESSORS_ONLN 2>/dev/null || nproc 2>/dev/null || echo 4
}

function InvokeCmake()
{
    local -r TargetMode="$1"
    local -r TargetDir="${BuildBaseDir}/${TargetMode}"
    local -r Jobs="$(GetProcessorCount)"

    command -v cmake >/dev/null 2>&1 || { PrintLog "${LogError}" "CMake not found."; exit $ErrorCode; }
    command -v ninja >/dev/null 2>&1 || { PrintLog "${LogError}" "Ninja not found."; exit $ErrorCode; }

    if [[ ! -f "${TargetDir}/build.ninja" ]]; then
        PrintLog "${LogInfo}" "Configuring CMake in ${TargetMode} mode..."
        cmake -S "${ProjectRoot}" -B "${TargetDir}" \
              -G "${NinjaGenerator}" \
              -DCMAKE_BUILD_TYPE="${TargetMode}" \
              -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
    fi

    PrintLog "${LogInfo}" "Building (${Jobs} threads)..."
    cmake --build "${TargetDir}" --parallel "${Jobs}"

    if [[ -f "${TargetDir}/compile_commands.json" ]]; then
        ln -sf "${TargetDir}/compile_commands.json" "${ProjectRoot}/compile_commands.json"
    fi
}

function ExecuteBinary()
{
    local -r TargetMode="$1"
    local -r BinaryPath="${BuildBaseDir}/${TargetMode}/${ExampleRelativePath}"

    if [[ -x "${BinaryPath}" ]]; then
        PrintLog "${LogInfo}" "Executing: ${BinaryPath}"
        (cd "$(dirname "${BinaryPath}")" && "./$(basename "${BinaryPath}")")
    else
        PrintLog "${LogError}" "Executable not found or not runnable: ${BinaryPath}"
        exit $ErrorCode
    fi
}

ResolvedCmd="build"
ResolvedMode="$(GetDefaultBuildMode)"

if [[ -n "${1:-}" ]]; then
    case "${1,,}" in
        build|run|clean|rebuild|help|--help|-h)
            ResolvedCmd="$1"
            if [[ -n "${2:-}" ]]; then
                ResolvedMode="$2"
            fi
            ;;
        *)
            if IsModeToken "$1"; then
                ResolvedCmd="build"
                ResolvedMode="$1"
            else
                ResolvedCmd="$1"
                if [[ -n "${2:-}" ]]; then
                    ResolvedMode="$2"
                fi
            fi
            ;;
    esac
fi

readonly InputCmd="${ResolvedCmd}"
readonly InputMode="$(GetBuildMode "${ResolvedMode}")"

case "${InputCmd,,}" in
    build)
        InvokeCmake "${InputMode}"
        PrintLog "${LogSuccess}" "Build completed."
        ;;

    run)
        InvokeCmake "${InputMode}"
        ExecuteBinary "${InputMode}"
        ;;

    clean)
        PrintLog "${LogWarn}" "Removing build artifacts..."
        rm -rf "${BuildBaseDir}"
        rm -f "${ProjectRoot}/compile_commands.json"
        PrintLog "${LogSuccess}" "Clean completed."
        ;;

    rebuild)
        InvokeCmake "${InputMode}"
        PrintLog "${LogSuccess}" "Rebuild completed."
        ;;

    help|--help|-h)
        ShowHelp
        ;;

    *)
        PrintLog "${LogError}" "Unknown command: ${InputCmd}"
        ShowHelp
        exit $ErrorCode
        ;;
esac
