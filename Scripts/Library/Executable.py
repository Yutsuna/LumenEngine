from __future__ import annotations

import os
import shutil
import subprocess
from functools import cached_property
from pathlib import Path

from .Errors import ConfigurationError

__all__ = ["Executable"]


class Executable:
    NEWEST_SUFFIX = 22
    OLDEST_SUFFIX = 14

    def __init__( self, path: str ) -> None:
        self.path = path

    @classmethod
    def find( cls, *names: str ) -> Executable | None:
        for name in names:
            path = shutil.which(name)
            if path:
                return cls(path)
        return None

    @classmethod
    def locate( cls, name: str, override: str | None = None ) -> Executable:
        """Override, then $<NAME>, then `name`, then `name-<NEWEST..OLDEST>` (Ubuntu ships versioned LLVM tools)."""
        requested = override or os.environ.get(cls.environment_variable(name))
        candidates = (requested,) if requested else (
            name, *(f"{name}-{version}" for version in range(cls.NEWEST_SUFFIX, cls.OLDEST_SUFFIX - 1, -1))
        )
        found = cls.find(*candidates)
        if found is None:
            raise ConfigurationError(
                "Missing tool", f"{name} not found. Use --tool or set ${cls.environment_variable(name)}."
            )
        return found

    @staticmethod
    def environment_variable( name: str ) -> str:
        return name.upper().replace("-", "_")

    @cached_property
    def version( self ) -> str:
        result = self.capture("--version")
        return (result.stdout + result.stderr).strip()

    def capture( self, *arguments: str, cwd: Path | None = None ) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [self.path, *arguments], cwd=cwd, capture_output=True, text=True, errors="replace",
            stdin=subprocess.DEVNULL, check=False,
        )
