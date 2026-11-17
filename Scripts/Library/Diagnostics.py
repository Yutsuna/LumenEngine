from __future__ import annotations

import re
from collections.abc import Iterator
from dataclasses import dataclass
from typing import ClassVar

__all__ = ["Diagnostic"]


@dataclass(frozen=True)
class Diagnostic:
    """One `file:line:col: warning|error: message` record, the format shared by gcc, clang and clang-tools."""

    file: str
    line: int
    column: int
    message: str

    PATTERN: ClassVar[re.Pattern[str]] = re.compile(
        r"^(?P<file>.+?):(?P<line>\d+):(?P<column>\d+): (?:warning|error): (?P<message>.*)$", re.MULTILINE
    )

    @classmethod
    def parse( cls, output: str ) -> Iterator[Diagnostic]:
        for match in cls.PATTERN.finditer(output):
            yield cls(match["file"], int(match["line"]), int(match["column"]), match["message"])
