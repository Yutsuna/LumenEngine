from __future__ import annotations

import json
import os
from collections.abc import Mapping
from dataclasses import dataclass
from pathlib import Path

from Library import ConfigurationError, Workspace

__all__ = ["CompilationDatabase", "CompileEntry"]


@dataclass(frozen=True)
class CompileEntry:
    command: str
    output: str  # normalized object file path, "" when the database does not say

    def portable( self, workspace: Workspace ) -> str:
        """Command without the checkout location, so cached keys survive a different CI checkout path."""
        return self.command.replace(os.fspath(workspace.root), "<ROOT>")


class CompilationDatabase:
    FILE_NAME = "compile_commands.json"

    def __init__( self, entries: Mapping[str, CompileEntry] ) -> None:
        self._entries = entries

    @classmethod
    def load( cls, build: Path ) -> CompilationDatabase:
        path = build / cls.FILE_NAME
        try:
            records = json.loads(path.read_text(encoding="utf-8"))
        except OSError as exception:
            raise ConfigurationError("Missing compilation database", f"Need: {path}. Run `meson setup {build.name}` first.") from exception
        except ValueError as exception:
            raise ConfigurationError("Invalid compilation database", f"{path}: {exception}") from exception

        entries: dict[str, CompileEntry] = {}
        for record in records:
            directory = record.get("directory", build)
            key = Workspace.normalize(os.path.join(directory, record["file"]))
            if key in entries:  # clang-tidy also uses the first matching entry
                continue
            output = record.get("output")
            entries[key] = CompileEntry(
                record.get("command") or " ".join(record.get("arguments", ())),
                Workspace.normalize(os.path.join(directory, output)) if output else "",
            )
        return cls(entries)

    def get( self, path: Path ) -> CompileEntry | None:
        return self._entries.get(Workspace.normalize(path))
