from __future__ import annotations

import os
from dataclasses import dataclass
from pathlib import Path
from typing import ClassVar

__all__ = ["Workspace"]


@dataclass(frozen=True)
class Workspace:
    """The repository checkout every script operates on, and the path conventions around it."""

    root: Path

    ENVIRONMENT: ClassVar[tuple[str, ...]] = ("GITHUB_REPOSITORY_SOURCE", "GITHUB_WORKSPACE")

    @classmethod
    def locate( cls, explicit: Path | None = None ) -> Workspace:
        if explicit is not None:
            return cls(explicit.resolve())
        for variable in cls.ENVIRONMENT:
            value = os.environ.get(variable)
            if value:
                return cls(Path(value).resolve())
        return cls(Path(__file__).resolve().parents[2])

    def absolute( self, path: str | Path, base: Path | None = None ) -> Path:
        return Path(os.path.abspath((base or self.root) / path))

    def relative( self, path: str | Path ) -> str:
        try:
            return os.path.relpath(path, self.root).replace(os.sep, "/")
        except ValueError:
            return os.fspath(path)

    @staticmethod
    def normalize( path: str | Path ) -> str:
        return os.path.normcase(os.path.normpath(os.fspath(path)))
