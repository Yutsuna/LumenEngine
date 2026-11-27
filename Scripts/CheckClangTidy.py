from __future__ import annotations

import json
import shlex
import tempfile
from argparse import ArgumentParser, Namespace
from pathlib import Path
from typing import ClassVar

from Clang import CompilationDatabase, HeaderFilter, IncludeScope, NinjaDependencies
from Library import (
    GLOBAL_EXTENSIONS,
    GLOBAL_HEADER_EXTENSIONS,
    CheckContext,
    ConfigurationError,
    Digest,
    SourceCheck,
    SourceTree,
)


class CheckClangTidy( SourceCheck ):
    DESCRIPTION = "Run clang-tidy on Source/ in parallel, skipping files already known valid."
    TOOL = "clang-tidy"
    CONFIG_FILE = ".clang-tidy"
    ARGUMENTS = ("--quiet", "--warnings-as-errors=*")
    TRANSLATION_UNIT = ".cpp"

    # Meson builds with GCC and passes -freflection project-wide. clang-tidy uses clang's driver, which
    # does not know that flag. clangd can drop flags via CompileFlags.Remove, clang-tidy has no such key:
    # strip them from a copy of compile_commands.json instead (see .clangd / .clang-tidy).
    INCOMPATIBLE_FLAGS: ClassVar[tuple[str, ...]] = ("-freflection",)

    _config: Path
    _build: Path
    _database_dir: Path
    _temporary: tempfile.TemporaryDirectory[str]
    _database: CompilationDatabase
    _scope: IncludeScope
    _header_filter: str

    def configure( self, parser: ArgumentParser ) -> None:
        super().configure(parser)
        parser.add_argument("--build-dir", default="Build", help="Meson build directory with compile_commands.json: Build | Build-Release | Build-TSAN.")
        parser.add_argument("--extra-arg", action="append", default=[], metavar="ARG", help="Forwarded as --extra-arg=ARG, repeatable.")
        parser.add_argument("--direct-headers", action="store_true", help="Also analyze *.hpp / *.inl directly (default: through the .cpp that include them).")

    def extensions( self, arguments: Namespace ) -> tuple[str, ...]:
        return GLOBAL_EXTENSIONS if arguments.direct_headers else (self.TRANSLATION_UNIT,)

    @classmethod
    def _sanitize( cls, arguments: list[str] ) -> list[str]:
        return [argument for argument in arguments if argument not in cls.INCOMPATIBLE_FLAGS]

    @classmethod
    def _sanitized_database( cls, build: Path, destination: Path ) -> None:
        source = build / CompilationDatabase.FILE_NAME
        try:
            records = json.loads(source.read_text(encoding="utf-8"))
        except OSError as exception:
            raise ConfigurationError("Missing compilation database", f"Need: {source}. Run `meson setup {build.name}` first.") from exception
        except ValueError as exception:
            raise ConfigurationError("Invalid compilation database", f"{source}: {exception}") from exception

        for record in records:
            if "arguments" in record:
                record["arguments"] = cls._sanitize(record["arguments"])
            else:
                record["command"] = shlex.join(cls._sanitize(shlex.split(record.get("command", ""))))

        (destination / CompilationDatabase.FILE_NAME).write_text(json.dumps(records), encoding="utf-8")

    def prepare( self, context: CheckContext ) -> None:
        workspace, arguments = context.workspace, context.arguments
        self._config = workspace.root / self.CONFIG_FILE
        if not self._config.is_file():
            raise ConfigurationError("Missing configuration", f"Need: {self._config} to exist.")

        self._build = workspace.absolute(arguments.build_dir).resolve()
        self._temporary = tempfile.TemporaryDirectory(prefix="lumen-clang-tidy-")
        self._database_dir = Path(self._temporary.name)
        self._sanitized_database(self._build, self._database_dir)
        self._database = CompilationDatabase.load(self._database_dir)

        source_dir = workspace.root / arguments.source_dir
        self._header_filter = HeaderFilter(workspace, source_dir).pattern
        headers = SourceTree(workspace, source_dir, GLOBAL_HEADER_EXTENSIONS, arguments.exclude)
        self._scope = IncludeScope(workspace, headers, NinjaDependencies.load(self._build), context.files)

    def label( self, context: CheckContext ) -> str:
        return f"{self.TOOL}-{self._build.name}"

    def cwd( self, context: CheckContext ) -> Path | None:
        return self._build

    def select( self, context: CheckContext, sources: list[Path] ) -> list[Path]:
        unknown = {path for path in sources if path.suffix == self.TRANSLATION_UNIT and self._database.get(path) is None}
        if unknown:
            print(f"warning: {len(unknown)} {self.TRANSLATION_UNIT} not built by {self._build.name}, skipped:")
            for path in sorted(unknown)[:10]:
                print(f"  - {context.workspace.relative(path)}")
        return [path for path in sources if path not in unknown]

    def fingerprint( self, context: CheckContext ) -> str:
        return Digest.of(
            context.executable.version, self._config.read_bytes(), self._header_filter,
            *self.ARGUMENTS, *context.arguments.extra_arg, *self.INCOMPATIBLE_FLAGS,
        )

    def scope( self, context: CheckContext, path: Path ) -> str:
        entry = self._database.get(path)
        command, output = (entry.portable(context.workspace), entry.output) if entry else ("", "")
        return Digest.of(command, self._scope.digest(output))

    def command( self, context: CheckContext, path: Path ) -> tuple[str, ...]:
        return (
            context.executable.path, "-p", str(self._database_dir), f"--config-file={self._config}",
            f"--header-filter={self._header_filter}", *self.ARGUMENTS,
            *(f"--extra-arg={extra}" for extra in context.arguments.extra_arg), str(path),
        )


if __name__ == "__main__":
    CheckClangTidy.run()
