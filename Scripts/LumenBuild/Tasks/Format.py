from __future__ import annotations

import sys

from invoke import task

from LumenBuild.Constants import (
    CLANG_EXTENSIONS,
    ERROR_CODE,
    ROOT_DIR,
)
from LumenBuild.Utils import Log, LogError, LogOk, LogWarn, RequireTool, RunParallel


@task(
    name="format",
    help={
        "check": "Check formatting without writing files",
    },
)
def Format(ctx, check: bool = False) -> None:
    clang_format = RequireTool("clang-format")

    search_roots = [
        ROOT_DIR / "LumenEngine" / "Source",
        ROOT_DIR / "LumenEngine" / "Shaders",
        ROOT_DIR / "Examples",
    ]

    files: list[str] = []
    for root in search_roots:
        if not root.exists():
            continue
        for pattern in CLANG_FORMAT_EXTENSIONS:
            files.extend(str(path) for path in root.rglob(pattern) if path.is_file())

    files = sorted(set(files))

    if not files:
        LogWarn("No C/C++ source files found to format.")
        return

    mode = "check" if check else "write"
    Log(f"clang-format ({mode}) on {len(files)} files", prefix="FMT  ")

    commands = []
    for file_path in files:
        if check:
            commands.append([clang_format, "--dry-run", "--Werror", file_path])
        else:
            commands.append([clang_format, "-i", file_path])

    results = RunParallel(commands, cwd=ROOT_DIR)
    failed = [result for result in results if result.returncode != 0]
    if failed:
        LogError(f"Formatting step failed for {len(failed)} file(s)")
        sys.exit(ERROR_CODE)

    if check:
        LogOk("Formatting check passed.")
    else:
        LogOk("Formatting applied.")
