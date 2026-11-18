from __future__ import annotations

import json
import os
from pathlib import Path
from types import TracebackType

from .Workspace import Workspace

__all__ = ["GLOBAL_CACHE_DIR", "GLOBAL_CACHE_ENVIRONMENT", "ResultCache"]

GLOBAL_CACHE_DIR = Path(".cache") / "Lint"
GLOBAL_CACHE_ENVIRONMENT = "LINT_CACHE_DIR"


class ResultCache:
    """Remembers which files are VALID for a given key. Failures are never stored: they are always re-analyzed."""

    VERSION = 1

    def __init__( self, path: Path, workspace: Workspace, enabled: bool = True ) -> None:
        self._path = path
        self._workspace = workspace
        self._enabled = enabled
        self._entries: dict[str, str] = {}
        self._dirty = False
        self._load()

    @classmethod
    def open( cls, workspace: Workspace, name: str, directory: Path | None, enabled: bool ) -> ResultCache:
        folder = directory or Path(os.environ.get(GLOBAL_CACHE_ENVIRONMENT, workspace.root / GLOBAL_CACHE_DIR))
        return cls(folder / f"{name}.json", workspace, enabled)

    def __enter__( self ) -> ResultCache:
        return self

    def __exit__( self, kind: type[BaseException] | None, error: BaseException | None, trace: TracebackType | None ) -> None:
        self.save()

    def is_valid( self, path: Path, key: str ) -> bool:
        return self._enabled and self._entries.get(self._workspace.relative(path)) == key

    def mark_valid( self, path: Path, key: str ) -> None:
        self._entries[self._workspace.relative(path)] = key
        self._dirty = True

    def forget( self, path: Path ) -> None:
        if self._entries.pop(self._workspace.relative(path), None) is not None:
            self._dirty = True

    def save( self ) -> None:
        if not self._dirty:
            return
        entries = {name: key for name, key in self._entries.items() if (self._workspace.root / name).exists()}
        self._path.parent.mkdir(parents=True, exist_ok=True)
        temporary = self._path.with_suffix(f".{os.getpid()}.tmp")
        temporary.write_text(
            json.dumps({"version": self.VERSION, "entries": entries}, sort_keys=True, separators=(",", ":")),
            encoding="utf-8",
        )
        os.replace(temporary, self._path)
        self._dirty = False

    def _load( self ) -> None:
        try:
            data = json.loads(self._path.read_text(encoding="utf-8"))
        except (OSError, ValueError):
            return
        if isinstance(data, dict) and data.get("version") == self.VERSION and isinstance(data.get("entries"), dict):
            self._entries = data["entries"]
