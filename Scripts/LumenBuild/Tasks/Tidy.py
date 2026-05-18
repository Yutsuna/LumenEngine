from __future__ import annotations

import sys

from invoke import task

from LumenBuild.Constants import (
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
from LumenBuild.Tasks.Build import BuildDebug

__all__ = ["Tidy"]


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
        "force": "Ignore cache and check all files",
    },
)
def Tidy(ctx, fix: bool = False, checks: str = "", force: bool = False) -> None:
    clang_tidy = RequireTool("clang-tidy")
    compile_db = ROOT_DIR / "compile_commands.json"
    search_root = ROOT_DIR / "LumenEngine" / "Source"

    if not compile_db.exists():
        Log("Generating compile_commands.json with debug configuration")
        BuildDebug(ctx)
    if not compile_db.exists():
        LogError("Failed to generate compile_commands.json")
        sys.exit(ERROR_CODE)

    files: list[str] = []
    for pattern in CLANG_EXTENSIONS:
        files.extend(str(path) for path in search_root.rglob(pattern))

    files = sorted(set(files))

    if not files:
        LogWarn("No C/C++ source files found to tidy.")
        return

    compile_db_str = str(compile_db.resolve())
    build_path_str = str((ROOT_DIR / "Build" / "Debug").resolve())

    commands: list[list[str]] = []
    for file_path in files:
        cmd: list[str] = [
            clang_tidy,
            f"-p={compile_db_str}",
            file_path,
        ]
        if checks:
            cmd.append(f"--checks={checks}")
        if fix:
            cmd.append("--fix")
        commands.append(cmd)

    results = RunParallel(commands, on_failure=_PrintTidyFailure)
    failed = [r for r in results if r.returncode != 0]
    if failed:
        LogError(f"Clang-Tidy step failed for {len(failed)} file(s)")
        sys.exit(ERROR_CODE)

    LogOk("Clang-Tidy checks passed.")
