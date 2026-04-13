from __future__ import annotations

from invoke import task

from LumenBuild import CMake as FCMake
from LumenBuild.Constants import (
    BUILD_TYPE_DEBUG,
    BUILD_TYPE_RELEASE,
    SANITIZER_FLAGS,
    EAnsiColor as C,
)
from LumenBuild.Utils import Log


@task(
    name="debug",
    help={
        "sanitizer": "Sanitizer to enable: asan, ubsan, tsan  (default: none)",
        "target": "CMake target to build  (default: all)",
        "testing": "Enable test targets (LUMEN_ENABLE_TESTING=ON)",
    },
    aliases=("d",),
)
def BuildDebug(
    ctx,
    sanitizer: str = "",
    target: str = "",
    testing: bool = False,
):
    _Header("Debug", sanitizer or None)

    FCMake.Build(
        BUILD_TYPE_DEBUG,
        sanitizer=sanitizer or None,
        target=target or None,
        enable_testing=testing,
    )


@task(
    name="release",
    help={
        "target": "CMake target to build  (default: all)",
        "lto": "Enable Link-Time Optimisation",
        "testing": "Enable test targets",
    },
    aliases=("r",),
)
def BuildRelease(
    ctx,
    target: str = "",
    lto: bool = False,
    testing: bool = False,
):
    _Header("Release", None)

    extra: list[str] = []
    if lto:
        extra.append("-DLUMEN_ENABLE_LTO=ON")

    FCMake.Build(
        BUILD_TYPE_RELEASE,
        target=target or None,
        extra_flags=extra,
        enable_testing=testing,
    )


@task(name="help")
def BuildHelp(ctx):
    sanitizers = ", ".join(SANITIZER_FLAGS)
    print(f"""
{C.BOLD}{C.CYAN}Lumen Build System — build subcommands{C.RESET}

  {C.BOLD}lumen build debug{C.RESET}
      Configure + build in Debug mode.

  {C.BOLD}lumen build debug --sanitizer <name>{C.RESET}
      Build with a sanitizer enabled.
      Available: {C.YELLOW}{sanitizers}{C.RESET}

      Examples:
        lumen build debug --sanitizer asan
        lumen build debug --sanitizer ubsan
        lumen build debug --sanitizer tsan

  {C.BOLD}lumen build debug --testing{C.RESET}
      Also compile test targets (requires GTest).

  {C.BOLD}lumen build debug --target <Target>{C.RESET}
      Build only the specified CMake target.

  {C.BOLD}lumen build release{C.RESET}
      Configure + build in Release mode (-O3).

  {C.BOLD}lumen build release --lto{C.RESET}
      Release build with Link-Time Optimisation.

  {C.BOLD}lumen build help{C.RESET}
      Show this message.
""")


def _Header(build_type: str, sanitizer: str | None) -> None:
    san_str = f" {C.YELLOW}[{sanitizer}]{C.RESET}" if sanitizer else ""
    Log(
        f"{C.BOLD}{build_type}{C.RESET}{san_str}  —  LumenEngine",
        prefix="BUILD",
        color=C.BLUE,
    )
