from __future__ import annotations

import subprocess
import sys

from invoke import task

from LumenBuild.Constants import (
    BUILD_DIR,
    ERROR_CODE,
    ROOT_DIR,
    CLANG_EXTENSIONS,
    TIDY_SUPPRESS_NOISE,
)
from LumenBuild.Utils import (
    Log,
    LogError,
    LogOk,
    LogWarn,
    RequireTool,
    RunParallel,
)
from LumenBuild.Cache import ( FileCache, GetAllFiles, GetFilesToCheck)
from LumenBuild.Tasks.Build import BuildDebug

__all__ = ["Tidy"]

TIDY_CACHE_PATH = BUILD_DIR / ".tidy_cache.json"
TIDY_COMPILE_DB = ROOT_DIR / "compile_commands.json"
TIDY_SEARCH_ROOTS = [ROOT_DIR / "LumenEngine" / "Source"]


def _PrintTidyFailure(res: subprocess.CompletedProcess) -> None:
    for stream in (res.stdout, res.stderr):
        if not stream:
            continue
        filtered = "\n".join(
            line
            for line in stream.splitlines()
            if not any(line.strip().startswith(n) for n in TIDY_SUPPRESS_NOISE)
        )
        if filtered.strip():
            print(filtered, file=sys.stderr)


@task(
    name="tidy",
    help={
        "fix": "Apply suggested fixes (inplace)",
        "checks": "Override checks (e.g. --checks='bugprone-*')",
        "force": "Ignore cache and re-check all files",
    },
)
def Tidy(ctx, fix: bool = False, checks: str = "", force: bool = False) -> None:
    clang_tidy = RequireTool("clang-tidy")

    if not TIDY_COMPILE_DB.exists():
        Log("Generating compile_commands.json with debug configuration")
        BuildDebug(ctx)
    if not TIDY_COMPILE_DB.exists():
        LogError("Failed to generate compile_commands.json")
        sys.exit(ERROR_CODE)

    all_files: list[str] = GetAllFiles(TIDY_SEARCH_ROOTS, CLANG_EXTENSIONS)

    if not all_files:
        LogWarn("No C/C++ source files found to tidy.")
        return

    cache = FileCache(TIDY_CACHE_PATH)

    if force:
        cache.Wipe()
        files_to_check = all_files
        Log("Force mode: ignoring cache, checking all files")
    else:
        cache.InvalidateIfMetaChanged("clang_tidy", clang_tidy)
        cache.InvalidateIfFileMetaChanged("compile_db", TIDY_COMPILE_DB)

        files_to_check = GetFilesToCheck(cache, all_files)

    if not files_to_check:
        LogOk("Clang-Tidy: nothing to check (all files up to date).")
        return

    Log(f"Checking {len(files_to_check)} file(s) with clang-tidy")

    compile_db_str = str(TIDY_COMPILE_DB.resolve())

    commands: list[list[str]] = []
    for file_path in files_to_check:
        cmd: list[str] = [clang_tidy, f"-p={compile_db_str}", file_path]
        if checks:
            cmd.append(f"--checks={checks}")
        if fix:
            cmd.append("--fix")
        commands.append(cmd)

    results = RunParallel(commands, on_failure=_PrintTidyFailure)

    failed_count = 0
    for cmd, res in zip(commands, results):
        file_path = cmd[2]
        if res.returncode == 0:
            cache.MarkOk(file_path)
        else:
            failed_count += 1

    cache.Save()

    if failed_count:
        LogError(f"Clang-Tidy step failed for {failed_count} file(s)")
        sys.exit(ERROR_CODE)

    LogOk("Clang-Tidy checks passed.")
