from __future__ import annotations

import re
from dataclasses import dataclass
from pathlib import Path

from Library import Workspace

__all__ = ["HeaderFilter"]


@dataclass(frozen=True)
class HeaderFilter:
    """
    clang-tidy `--header-filter` restricting diagnostics to the project sources.

    Headers are reported as `/abs/root/Source/..`, or `../Source/..` when the build uses relative include
    paths (meson does): both forms must match. The engine is LLVM's POSIX regex: no `(?:...)` groups.
    """

    workspace: Workspace
    directory: Path

    @property
    def pattern( self ) -> str:
        separator = r"[/\\]"
        root = re.escape(str(self.workspace.root))
        source = re.escape(self.workspace.relative(self.directory))
        return "^(" + root + separator + r"|(\.{1,2}" + separator + ")*)" + source + separator + ".*"
