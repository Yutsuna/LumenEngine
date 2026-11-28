"""
DESCRIPTION:
    Copy Windows shared libraries next to an executable after a Meson build.

USAGE:
    MesonStageRuntimeDlls.py <stamp-output> <dll> [<dll> ...]
"""

from __future__ import annotations

import shutil
import sys
from pathlib import Path


def Error(Message: str, ErrorCode: int) -> int:
    print(Message, file=sys.stderr)
    return ErrorCode


def Main() -> int:

    if len(sys.argv) < 3:
        return Error("USAGE: MesonStageRuntimeDlls.py <stamp> <dll> [<dll> ...]", 2)

    StampOutput = Path(sys.argv[1])
    Destination = StampOutput.parent

    for RawSource in sys.argv[2:]:
        SourcePath = Path(RawSource)
        Target     = Destination / SourcePath.name

        if SourcePath.resolve() != Target.resolve():
            shutil.copy2(SourcePath, Target)

    StampOutput.touch()
    return 0


if __name__ == "__main__":
    raise SystemExit(Main())
