from __future__ import annotations

from argparse import ArgumentParser
from pathlib import Path

from Library import CheckContext, ConfigurationError, Digest, SourceCheck


class CheckClangFormat( SourceCheck ):
    DESCRIPTION = "Run clang-format on Source/ in parallel, skipping files already known valid."
    TOOL = "clang-format"
    STYLE_FILE = ".clang-format"
    HINT = "Run `python Scripts/CheckClangFormat.py --fix` locally and commit the result."

    _style: Path

    def configure( self, parser: ArgumentParser ) -> None:
        super().configure(parser)
        parser.add_argument("--fix", action="store_true", help="Reformat files in place instead of only checking them.")

    def prepare( self, context: CheckContext ) -> None:
        self._style = context.workspace.root / self.STYLE_FILE
        if not self._style.is_file():
            raise ConfigurationError("Missing configuration", f"Need: {self._style} to exist.")

    def fingerprint( self, context: CheckContext ) -> str:
        return Digest.of(context.executable.version, self._style.read_bytes())

    def rewrites( self, context: CheckContext ) -> bool:
        return context.arguments.fix

    def command( self, context: CheckContext, path: Path ) -> tuple[str, ...]:
        mode = ("-i",) if context.arguments.fix else ("--dry-run", "--Werror")
        return (context.executable.path, f"--style=file:{self._style}", *mode, str(path))


if __name__ == "__main__":
    CheckClangFormat.run()
