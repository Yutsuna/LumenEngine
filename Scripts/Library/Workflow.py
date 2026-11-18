from __future__ import annotations

import os
import sys
from collections.abc import Iterator, Mapping
from contextlib import contextmanager

__all__ = ["Workflow"]


class Workflow:
    """CI feedback channel: GitHub workflow commands when running in Actions, plain text otherwise."""

    def __init__( self, environment: Mapping[str, str] | None = None ) -> None:
        self._environment = os.environ if environment is None else environment

    @property
    def active( self ) -> bool:
        return self._environment.get("GITHUB_ACTIONS") == "true"

    def error( self, title: str, message: str ) -> None:
        if self.active:
            print(f"::error title={self._property(title)}::{self._data(message)}")
        else:
            print(f"error: {title}: {message}", file=sys.stderr)

    def annotate( self, file: str, line: int, column: int, title: str, message: str ) -> None:
        """File-level annotation; only meaningful in Actions, the full output is already printed locally."""
        if self.active:
            print(
                f"::error file={self._property(file)},line={line},col={column},"
                f"title={self._property(title)}::{self._data(message)}"
            )

    @contextmanager
    def group( self, title: str ) -> Iterator[None]:
        print(f"::group::{title}" if self.active else f"--- {title}")
        try:
            yield
        finally:
            if self.active:
                print("::endgroup::")

    def summary( self, markdown: str ) -> None:
        path = self._environment.get("GITHUB_STEP_SUMMARY")
        if path:
            with open(path, "a", encoding="utf-8") as stream:
                stream.write(markdown)

    @staticmethod
    def _data( value: str ) -> str:
        return value.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")

    @classmethod
    def _property( cls, value: str ) -> str:
        return cls._data(value).replace(":", "%3A").replace(",", "%2C")
