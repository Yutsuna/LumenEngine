from __future__ import annotations

from collections.abc import Mapping
from pathlib import Path

from Library import Executable, Workspace

__all__ = ["NinjaDependencies"]


class NinjaDependencies:
    """Every file each object file was built from, read from ninja's dependency log in a single call."""

    EXECUTABLES = ("ninja", "samu", "ninja-build")
    SEPARATOR = ": #deps"

    def __init__( self, includes: Mapping[str, tuple[str, ...]] ) -> None:
        self._includes = includes

    @classmethod
    def load( cls, build: Path ) -> NinjaDependencies:
        ninja = Executable.find(*cls.EXECUTABLES)
        if ninja is None:
            return cls({})
        try:
            result = ninja.capture("-t", "deps", cwd=build)
        except OSError:
            return cls({})
        return cls(cls._parse(result.stdout, build) if result.returncode == 0 else {})

    def __bool__( self ) -> bool:
        return bool(self._includes)

    def of( self, object_file: str ) -> tuple[str, ...]:
        return self._includes.get(object_file, ())

    @classmethod
    def _parse( cls, text: str, build: Path ) -> dict[str, tuple[str, ...]]:
        includes: dict[str, list[str]] = {}
        current: list[str] | None = None
        for line in text.splitlines():
            if not line.strip():
                current = None
            elif not line[0].isspace():
                current = includes.setdefault(Workspace.normalize(build / line.split(cls.SEPARATOR, 1)[0]), [])
            elif current is not None:
                current.append(Workspace.normalize(build / line.strip()))
        return {target: tuple(files) for target, files in includes.items()}
