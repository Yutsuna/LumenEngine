from __future__ import annotations

import hashlib
import json
from pathlib import Path

from LumenBuild.Utils import Log

__all__ = [
    "FileCache",
    "GetAllFiles",
    "GetFilesToCheck",
]

CACHE_VERSION = 1
CACHE_CHUNK = 1 << 16


def _SHA256(path: Path) -> str:
    h = hashlib.sha256()
    with path.open("rb") as f:
        while chunk := f.read(CACHE_CHUNK):
            h.update(chunk)
    return h.hexdigest()


class FileCache:

    def __init__(self, path: Path) -> None:
        self._path = path
        self.Data: dict[str, object] = {"version": CACHE_VERSION, "meta": {}, "files": {}}
        self.Load()

    def Load(self) -> None:
        if not self._path.exists():
            return
        try:
            raw = json.loads(self._path.read_text())
            if raw.get("version") == CACHE_VERSION:
                self.Data = raw
        except (json.JSONDecodeError, OSError):
            pass

    def Save(self) -> None:
        self._path.parent.mkdir(parents=True, exist_ok=True)
        self._path.write_text(json.dumps(self.Data, indent=2))

    def InvalidateIfMetaChanged(self, key: str, current_value: str) -> bool:
        if self.Data["meta"].get(key) != current_value:
            self.Data["files"] = {}
            self.Data["meta"][key] = current_value
            return True
        return False

    def InvalidateIfFileMetaChanged(self, key: str, path: Path) -> bool:
        return self.InvalidateIfMetaChanged(key, _SHA256(path))

    def Wipe(self) -> None:
        self.Data["files"] = {}

    def NeedsCheck(self, file_path: str) -> bool:
        path = Path(file_path)
        if not path.exists():
            return False
        return self.Data["files"].get(file_path) != _SHA256(path)

    def MarkOk(self, file_path: str) -> None:
        path = Path(file_path)
        if path.exists():
            self.Data["files"][file_path] = _SHA256(path)


def GetAllFiles(root_paths: list[Path], patterns: tuple[str, ...])->list[str]:
    return sorted(
        set(
            str(path)
            for root in root_paths
            if root.exists()
            for pattern in patterns
            for path in root.rglob(pattern)
            if path.is_file()
        )
    )

def GetFilesToCheck(cache: FileCache, all_files: list[str]) -> list[str]:
    files_to_check: list[str] = [f for f in all_files if cache.NeedsCheck(f)]
    skipped = len(all_files) - len(files_to_check)
    if skipped:
        Log(f"Cache: skipping {skipped} unchanged file(s)")

    return files_to_check
