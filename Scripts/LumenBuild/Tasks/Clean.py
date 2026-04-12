from invoke import task

from LumenBuild import CMake as FCMake
from LumenBuild.Constants import BUILD_TYPE_DEBUG, BUILD_TYPE_RELEASE
from LumenBuild.Constants import EAnsiColor as C
from LumenBuild.Utils import Log


@task(
    name="clean",
    help={
        "debug": "Remove only the Debug build directory",
        "release": "Remove only the Release build directory",
        "sanitizer": "Remove a sanitizer-specific Debug build  (e.g. --sanitizer asan)",
    },
)
def Clean(ctx, debug: bool = False, release: bool = False, sanitizer: str = "") -> None:

    Log("Removing build artefacts", prefix="CLEAN", color=C.RED)

    if sanitizer and not (debug or release):
        debug = True

    if debug:
        FCMake.Clean(BUILD_TYPE_DEBUG, sanitizer=sanitizer or None)
        return

    if release:
        FCMake.Clean(BUILD_TYPE_RELEASE)
        return

    FCMake.Clean(None)
