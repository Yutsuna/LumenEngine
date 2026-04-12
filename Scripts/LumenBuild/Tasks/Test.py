from __future__ import annotations

from invoke import task

from LumenBuild import CMake as FCMake
from LumenBuild.Constants import BUILD_TYPE_DEBUG, EAnsiColor as C
from LumenBuild.Utils import Log


@task(
    name="test",
    help={
        "build_type": "Build type to run tests from (default: Debug)",
        "filter": "CTest regex filter  (e.g. --filter Core)",
        "no_parallel": "Disable parallel test execution",
    },
)
def Test(
    ctx,
    build_type: str = BUILD_TYPE_DEBUG,
    filter: str = "",
    no_parallel: bool = False,
):
    Log(f"{C.BOLD}{build_type}{C.RESET}  —  LumenEngine", prefix="TEST ", color=C.CYAN)

    FCMake.Test(
        build_type,
        filter_pattern=filter or None,
        parallel=not no_parallel,
    )
