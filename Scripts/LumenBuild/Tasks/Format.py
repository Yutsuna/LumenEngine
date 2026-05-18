from __future__ import annotations

import sys

from invoke import task

from LumenBuild.Constants import (
    BUILD_DIR,
    CLANG_EXTENSIONS,
    ERROR_CODE,
    ROOT_DIR,
)
from LumenBuild.Utils import Log, LogError, LogOk, LogWarn, RequireTool, RunParallel
from LumenBuild.Cache import (FileCache, GetAllFiles, GetFilesToCheck)

__all__ = ["Format"]


FORMAT_CACHE_PATH = BUILD_DIR / ".format_cache.json"
FORMAT_SEARCH_ROOTS = [
    ROOT_DIR / "LumenEngine" / "Source",
    ROOT_DIR / "LumenEngine" / "Shaders",
    ROOT_DIR / "Examples",
]

@task(
    name="format",
    help={
        "check": "Check formatting without writing files",
        "force": "Ignore cache and check all files",
    },
)
def Format(ctx, check: bool = False, force: bool = False) -> None:
    clang_format = RequireTool("clang-format")

    all_files: list[str] = GetAllFiles(FORMAT_SEARCH_ROOTS, CLANG_EXTENSIONS)

    if not all_files:
        LogWarn("No C/C++ source files found to format.")
        return

    cache = FileCache(FORMAT_CACHE_PATH)

    if check or force:
        files_to_check = all_files
        if force:
            cache.Wipe()
            Log("Force mode: ignoring cache, checking all files")

    else:
        cache.InvalidateIfMetaChanged("clang_format", clang_format)
        files_to_check = GetFilesToCheck(cache, all_files)

    if not files_to_check:
        LogOk("Format: nothing to do (all files up to date).")
        return

    mode = "check" if check else "write"
    Log(f"clang-format ({mode}) on {len(files_to_check)} file(s)")

    commands: list[list[str]] = []
    for file_path in files_to_check:
        if check:
            commands.append([clang_format, "--dry-run", "--Werror", file_path])
        else:
            commands.append([clang_format, "-i", file_path])

    results = RunParallel(commands)

    failed_count = 0
    for cmd, res in zip(commands, results):
        file_path = cmd[-1]
        if res.returncode == 0:
            if not check:
                cache.MarkOk(file_path)
        else:
            failed_count += 1

    if not check:
        cache.Save()

    if failed_count:
        LogError(f"Formatting step failed for {failed_count} file(s)")
        sys.exit(ERROR_CODE)

    if check:
        LogOk("Formatting check passed.")
    else:
        LogOk("Formatting applied.")
