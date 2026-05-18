"""
lumen/utils.py
==============
Shared utilities: logging, CPU detection, subprocess helpers.
"""

from __future__ import annotations

import multiprocessing
import shutil
import subprocess
import sys
import time
from pathlib import Path
from typing import Sequence
import concurrent.futures
from typing import Callable

from LumenBuild.Constants import ERROR_CODE, SUCCESS_CODE, EAnsiColor as C

# ---------------------------------------------------------------------------
# CPU / parallelism
# ---------------------------------------------------------------------------


def GetParallelJobs() -> int:
    return multiprocessing.cpu_count()


# ---------------------------------------------------------------------------
# Logging
# ---------------------------------------------------------------------------


def Log(message: str, *, prefix: str = "Lumen", color: str = C.CYAN) -> None:
    tag = f"{color}{C.BOLD}[{prefix}]{C.RESET}"
    print(f"{tag} {message}", flush=True)


def LogOk(message: str) -> None:
    Log(message, prefix="  OK  ", color=C.GREEN)


def LogWarn(message: str) -> None:
    Log(message, prefix=" WARN ", color=C.YELLOW)


def LogError(message: str) -> None:
    Log(message, prefix=" ERR  ", color=C.RED)


def LogStep(message: str) -> None:
    Log(message, prefix="  >>  ", color=C.BLUE)


def LogDim(message: str) -> None:
    print(f"{C.GREY}{message}{C.RESET}", flush=True)


# ---------------------------------------------------------------------------
# Tool detection
# ---------------------------------------------------------------------------


def RequireTool(name: str) -> str:
    path = shutil.which(name)
    if path is None:
        LogError(f"Required tool not found: {C.BOLD}{name}{C.RESET}")
        LogError("Please install it and make sure it is on your PATH.")
        sys.exit(ERROR_CODE)
    return path


def FindTool(name: str) -> str | None:
    return shutil.which(name)


# ---------------------------------------------------------------------------
# Subprocess
# ---------------------------------------------------------------------------


def Run(
    cmd: Sequence[str],
    *,
    cwd: Path | None = None,
    env: dict | None = None,
    check: bool = True,
    capture: bool = False,
) -> subprocess.CompletedProcess:
    display = " ".join(str(a) for a in cmd)
    LogDim(f"$ {display}")

    t0 = time.monotonic()
    result = subprocess.run(
        [str(a) for a in cmd],
        cwd=str(cwd) if cwd else None,
        env=env,
        capture_output=capture,
        text=True,
    )
    elapsed = time.monotonic() - t0

    if result.returncode != 0 and check:
        LogError(f"Command failed (exit {result.returncode}) after {elapsed:.1f}s")
        LogError(f"  {display}")
        if capture and result.stderr:
            print(result.stderr, file=sys.stderr)
        sys.exit(result.returncode)

    return result


def RunParallel(
    commands: list[Sequence[str]],
    *,
    cwd: Path | None = None,
    max_workers: int | None = None,
    on_failure: Callable[[subprocess.CompletedProcess], None] | None = None,
) -> list[subprocess.CompletedProcess]:

    workers = max_workers or GetParallelJobs()
    results: list[subprocess.CompletedProcess] = []

    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as pool:
        futures = {
            pool.submit(Run, cmd, cwd=cwd, check=False, capture=True): cmd
            for cmd in commands
        }
        for fut in concurrent.futures.as_completed(futures):
            cmd = futures[fut]
            try:
                res = fut.result()
                results.append(res)
                if res.returncode != SUCCESS_CODE:
                    LogError(f"Failed: {' '.join(str(a) for a in cmd)}")
                    if on_failure:
                        on_failure(res)
                    else:
                        if res.stdout:
                            print(res.stdout, file=sys.stderr)
                        if res.stderr:
                            print(res.stderr, file=sys.stderr)
            except Exception as exc:
                LogError(f"Exception for {cmd}: {exc}")

    return results
