from __future__ import annotations

import sys
from abc import ABC, abstractmethod
from argparse import ArgumentParser, Namespace
from collections.abc import Sequence
from typing import ClassVar, NoReturn

from .Errors import ConfigurationError
from .Workflow import Workflow

__all__ = ["EntryPoint"]


class EntryPoint(ABC):
    """Single place for argument parsing, error reporting and exit codes of every script in Scripts/."""

    DESCRIPTION: ClassVar[str] = ""

    def configure( self, parser: ArgumentParser ) -> None:
        """Declare arguments. Overrides extend the parent's: call super().configure(parser) first."""

    @abstractmethod
    def execute( self, arguments: Namespace ) -> int:
        """Do the work and return the process exit code."""

    def main( self, argv: Sequence[str] ) -> int:
        parser = ArgumentParser(description=self.DESCRIPTION)
        self.configure(parser)
        try:
            return self.execute(parser.parse_args(argv))
        except ConfigurationError as exception:
            Workflow().error(exception.title, str(exception))
            return 1

    @classmethod
    def run( cls ) -> NoReturn:
        sys.exit(cls().main(sys.argv[1:]))
