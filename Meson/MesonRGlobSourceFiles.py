"""
DESCRIPTION:
    RGlob all sources files in a given directory

USAGE:
    MesonRGlobSourceFiles.py <directory>
"""

import sys
from pathlib import Path


def Main() -> int:

    for SourceFile in Path(sys.argv[1]).rglob("*"):
        if SourceFile.suffix in (".c", ".cpp"):
            print(SourceFile.as_posix())

    return 0


if __name__ == "__main__":
    raise SystemExit(Main())
