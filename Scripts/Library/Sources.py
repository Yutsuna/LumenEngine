from __future__ import annotations

import os
from collections.abc import Iterable, Sequence
from fnmatch import fnmatch
from pathlib import Path

from .Workspace import Workspace

__all__ = ["GLOBAL_EXTENSIONS", "GLOBAL_HEADER_EXTENSIONS", "SourceTree"]

GLOBAL_EXTENSIONS: tuple[str, ...] = (".hpp", ".inl", ".cpp")
GLOBAL_HEADER_EXTENSIONS: tuple[str, ...] = (".hpp", ".inl", ".h")


class SourceTree:
    def __init__(
        self,
        workspace: Workspace,
        directory: Path,
        extensions: tuple[str, ...] = GLOBAL_EXTENSIONS,
        excludes: Iterable[str] = (),
    ) -> None:
        self._workspace = workspace
        self._directory = directory
        self._extensions = extensions
        self._excludes = tuple(excludes)

    def discover( self, requested: Sequence[Path] = () ) -> list[Path]:
        """Sorted files under the tree; when `requested` is given, only those."""
        wanted = {os.fspath(self._workspace.absolute(path)) for path in requested}
        found: list[Path] = []
        for current, directories, names in os.walk(self._directory):
            directories[:] = [name for name in directories if not name.startswith(".")]
            for name in names:
                path = Path(current, name)
                if name.endswith(self._extensions) and not self._excluded(path) and (not requested or os.fspath(path) in wanted):
                    found.append(path)
        return sorted(found)

    def _excluded( self, path: Path ) -> bool:
        if not self._excludes:
            return False
        relative = self._workspace.relative(path)
        return any(fnmatch(relative, pattern) for pattern in self._excludes)
