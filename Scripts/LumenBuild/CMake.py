from __future__ import annotations

from pathlib import Path
import shutil
import sys
from typing import Iterable

from LumenBuild.Constants import (
    ERROR_CODE,
    BUILD_DIR,
    BUILD_TYPE_DEBUG,
    ROOT_DIR,
    SANITIZER_ALIASES,
    SANITIZER_FLAGS,
)
from LumenBuild.Utils import (
    GetParallelJobs,
    Log,
    LogOk,
    LogStep,
    LogWarn,
    RequireTool,
    Run,
)


def _LinkCompileCommands(build_dir: Path) -> None:
    source = build_dir / "compile_commands.json"
    target = ROOT_DIR / "compile_commands.json"

    if not source.exists():
        return

    if target.is_symlink() and target.resolve() == source.resolve():
        return

    if target.exists() or target.is_symlink():
        target.unlink()

    target.symlink_to(source)
    LogStep(f"compile_commands.json -> {source}")


def _ResolveSanitizer(name: str) -> str:
    key = name.lower()
    return SANITIZER_ALIASES.get(key, key)


def _SanitizerFlag(name: str) -> str:
    key = _ResolveSanitizer(name)
    flag = SANITIZER_FLAGS.get(key)
    if flag is None:
        known = ", ".join(SANITIZER_FLAGS)
        Log(
            f"Unknown sanitizer '{name}'. Known: {known}",
            prefix=" ERR ",
            color="\033[91m",
        )
        sys.exit(ERROR_CODE)
    return flag


def BuildDir(build_type: str, sanitizer: str | None = None) -> Path:
    suffix = f"-{sanitizer}" if sanitizer else ""
    return BUILD_DIR / f"{build_type}{suffix}"


def Configure(
    build_type: str,
    *,
    sanitizer: str | None = None,
    extra_flags: Iterable[str] = (),
    enable_testing: bool = False,
) -> Path:
    cmake = RequireTool("cmake")
    jobs = GetParallelJobs()
    bdir = BuildDir(build_type, sanitizer)

    bdir.mkdir(parents=True, exist_ok=True)

    cmd: list[str] = [
        cmake,
        "-S",
        str(ROOT_DIR),
        "-B",
        str(bdir),
        f"-DCMAKE_BUILD_TYPE={build_type}",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON",
    ]

    if sanitizer:
        cmd.append(_SanitizerFlag(sanitizer))

    if enable_testing:
        cmd.append("-DLUMEN_ENABLE_TESTING=ON")

    cmd.extend(extra_flags)

    ninja = shutil.which("ninja")
    if ninja:
        cmd += ["-G", "Ninja"]
        LogStep(f"Generator : Ninja  ({jobs} workers)")
    else:
        LogStep(f"Generator : Unix Makefiles  ({jobs} workers)")

    LogStep("Configure : " + build_type + (f"  [{sanitizer}]" if sanitizer else ""))

    Run(cmd, cwd=ROOT_DIR)
    _LinkCompileCommands(bdir)
    return bdir


# ---------------------------------------------------------------------------
# Build
# ---------------------------------------------------------------------------


def Build(
    build_type: str,
    *,
    sanitizer: str | None = None,
    target: str | None = None,
    enable_testing: bool = False,
    extra_flags: Iterable[str] = (),
) -> None:
    jobs = GetParallelJobs()

    bdir = Configure(
        build_type,
        sanitizer=sanitizer,
        extra_flags=extra_flags,
        enable_testing=enable_testing,
    )

    cmake = RequireTool("cmake")

    cmd: list[str] = [
        cmake,
        "--build",
        str(bdir),
        "--parallel",
        str(jobs),
    ]

    if target:
        cmd += ["--target", target]

    LogStep("Build     : " + build_type + (f"  [{sanitizer}]" if sanitizer else ""))
    LogStep(f"Jobs      : {jobs} parallel")

    Run(cmd, cwd=bdir)
    LogOk(f"Build complete  →  {bdir}")


# ---------------------------------------------------------------------------
# Test
# ---------------------------------------------------------------------------


def Test(
    build_type: str = BUILD_TYPE_DEBUG,
    *,
    filter_pattern: str | None = None,
    parallel: bool = True,
) -> None:
    ctest = RequireTool("ctest")
    jobs = GetParallelJobs()
    bdir = BuildDir(build_type)

    if not bdir.exists():
        LogWarn(f"Build directory not found: {bdir}")
        LogWarn("Run  'lumen build debug --testing'  first.")
        sys.exit(ERROR_CODE)

    cmd: list[str] = [
        ctest,
        "--test-dir",
        str(bdir),
        "--output-on-failure",
    ]

    if parallel:
        cmd += ["--parallel", str(jobs)]

    if filter_pattern:
        cmd += ["-R", filter_pattern]

    LogStep(f"CTest     : {build_type}  ({jobs} parallel workers)")

    Run(cmd, cwd=bdir)
    LogOk("All tests passed.")


# ---------------------------------------------------------------------------
# Clean
# ---------------------------------------------------------------------------


def Clean(build_type: str | None, *, sanitizer: str | None = None) -> None:
    if build_type:
        target = BuildDir(build_type, sanitizer)
        if target.exists():
            LogStep(f"Remove    : {target}")
            shutil.rmtree(target)
            LogOk(f"Removed  {target}")
        else:
            LogWarn(f"Nothing to clean: {target}")
    else:
        if BUILD_DIR.exists():
            LogStep(f"Remove    : {BUILD_DIR}  (all configurations)")
            shutil.rmtree(BUILD_DIR)
            LogOk(f"Removed  {BUILD_DIR}")
        else:
            LogWarn("Build directory does not exist — already clean.")
