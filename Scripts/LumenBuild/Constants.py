from pathlib import Path

# ---------------------------------------------------------------------------
# Exit codes
# ---------------------------------------------------------------------------

ERROR_CODE = 84
SUCCESS_CODE = 0

# ---------------------------------------------------------------------------
# Paths
# ---------------------------------------------------------------------------

ROOT_DIR: Path = Path(__file__).resolve().parents[2]
BUILD_DIR: Path = ROOT_DIR / "Build"
SOURCE_DIR: Path = ROOT_DIR / "LumenEngine" / "Source"

# ---------------------------------------------------------------------------
# CMake presets / build types
# ---------------------------------------------------------------------------

BUILD_TYPE_DEBUG = "Debug"
BUILD_TYPE_RELEASE = "Release"
BUILD_TYPE_RELDBG = "RelWithDebInfo"

# ---------------------------------------------------------------------------
# Sanitizers  (cmake -D flags)
# ---------------------------------------------------------------------------

SANITIZER_FLAGS: dict[str, str] = {
    "asan": "-DLUMEN_ENABLE_ASAN=ON",
    "ubsan": "-DLUMEN_ENABLE_UBSAN=ON",
    "tsan": "-DLUMEN_ENABLE_TSAN=ON",
}

SANITIZER_ALIASES: dict[str, str] = {
    "address": "asan",
    "undefined": "ubsan",
    "thread": "tsan",
}

# ---------------------------------------------------------------------------
# Clang utilities
# ---------------------------------------------------------------------------

CLANG_EXTENSIONS = ("*.cpp", "*.hpp", "*.inl", "*.frag", "*.vert", "*.comp")
TIDY_CACHE_FILE = BUILD_DIR / ".lumen_tidy_cache.json"

# ---------------------------------------------------------------------------
# Colours
# ---------------------------------------------------------------------------


class EAnsiColor:
    RESET = "\033[0m"
    BOLD = "\033[1m"
    RED = "\033[91m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    BLUE = "\033[94m"
    CYAN = "\033[96m"
    GREY = "\033[90m"
