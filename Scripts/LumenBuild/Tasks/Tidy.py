from __future__ import annotations

import hashlib
import json
import sys
from pathlib import Path
from typing import Sequence

from invoke import task

from LumenBuild.Constants import (
    CLANG_EXTENSIONS,
    ERROR_CODE,
    ROOT_DIR,
    TIDY_CACHE_FILE,
    TIDY_HEADER_FILTER,
)
from LumenBuild.Utils import (
    Log,
    LogError,
    LogOk,
    LogWarn,
    RequireTool,
    RunParallel,
)
from LumenBuild.Tasks.Build import BuildDebug

__all__ = ["Tidy"]


def _GetFileHash(path: Path, config_hash: bytes) -> str:
    hasher = hashlib.sha1()
    hasher.update(config_hash)
    hasher.update(path.read_bytes())
    return hasher.hexdigest()


def _LoadCache() -> dict[str, str]:
    if TIDY_CACHE_FILE.exists():
        try:
            return json.loads(TIDY_CACHE_FILE.read_text())
        except Exception:
            return {}
    return {}


def _GetSourceFiles(
    force: bool, config_hash: bytes
) -> tuple[list[Path], dict[str, str], dict[str, str]]:
    search_root = ROOT_DIR / "LumenEngine" / "Source"
    all_files: list[Path] = []

    for pattern in CLANG_EXTENSIONS:
        all_files.extend(search_root.rglob(pattern))

    all_files = sorted({f.resolve() for f in all_files if f.is_file()})

    cache = _LoadCache()
    to_process: list[Path] = []
    current_hashes: dict[str, str] = {}

    for path in all_files:
        p_str = str(path)
        h = _GetFileHash(path, config_hash)
        current_hashes[p_str] = h

        if force or cache.get(p_str) != h:
            to_process.append(path)

    return to_process, current_hashes, cache


def _UpdateCache(
    results: list, current_hashes: dict[str, str], old_cache: dict[str, str]
) -> int:
    new_cache = old_cache.copy()
    failed_count = 0

    for res in results:
        file_path = res.args[-1]
        if res.returncode == 0:
            new_cache[file_path] = current_hashes[file_path]
        else:
            failed_count += 1
            if file_path in new_cache:
                del new_cache[file_path]

    TIDY_CACHE_FILE.parent.mkdir(parents=True, exist_ok=True)
    TIDY_CACHE_FILE.write_text(json.dumps(new_cache, indent=2))
    return failed_count


@task(
    name="tidy",
    help={
        "fix": "Apply suggested fixes (inplace)",
        "checks": "Override checks (e.g. --checks='bugprone-*')",
        "force": "Ignore cache and check all files",
    },
)
def Tidy(ctx, fix: bool = False, checks: str = "", force: bool = False) -> None:
    clang_tidy = RequireTool("clang-tidy")
    compile_db = ROOT_DIR / "compile_commands.json"

    if not compile_db.exists():
        LogWarn(
            "compile_commands.json not found. Running 'build.debug' to generate it..."
        )
        BuildDebug(ctx)

        if not compile_db.exists():
            LogError("Failed to generate compile_commands.json.")
            sys.exit(ERROR_CODE)

    config_file = ROOT_DIR / ".clang-tidy"
    config_hash = config_file.read_bytes() if config_file.exists() else b""
    to_process, current_hashes, cache = _GetSourceFiles(force, config_hash)

    if not to_process:
        LogOk("No changes detected. Everything is clean (cached).")
        return

    Log(
        f"clang-tidy: {len(to_process)} files to check",
        prefix="TIDY ",
    )

    base_cmd = [
        clang_tidy,
        "-p",
        str(ROOT_DIR),
        f"--header-filter={TIDY_HEADER_FILTER}",
        "--extra-arg=-Wno-unknown-warning-option",
    ]
    if not fix:
        base_cmd.append("--quiet")
    else:
        base_cmd.extend(["--fix-errors", "--format-style=file"])

    if checks:
        base_cmd.append(f"--checks={checks}")

    commands: list[Sequence[str]] = [base_cmd + [str(p)] for p in to_process]
    results = RunParallel(commands, cwd=ROOT_DIR)

    failed_count = _UpdateCache(results, current_hashes, {} if force else cache)

    if failed_count > 0:
        LogError(f"Clang-tidy found issues in {failed_count} file(s).")
        sys.exit(ERROR_CODE)

    LogOk("All files passed clang-tidy.")
