from __future__ import annotations

from argparse import ArgumentParser, Namespace
from pathlib import Path

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

    _config: Path
    _build: Path
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

    def prepare( self, context: CheckContext ) -> None:
        workspace, arguments = context.workspace, context.arguments
        self._config = workspace.root / self.CONFIG_FILE
        if not self._config.is_file():
            raise ConfigurationError("Missing configuration", f"Need: {self._config} to exist.")

        self._build = workspace.absolute(arguments.build_dir).resolve()
        self._database = CompilationDatabase.load(self._build)

        source_dir = workspace.root / arguments.source_dir
        self._header_filter = HeaderFilter(workspace, source_dir).pattern
        headers = SourceTree(workspace, source_dir, GLOBAL_HEADER_EXTENSIONS, arguments.exclude)
        self._scope = IncludeScope(workspace, headers, NinjaDependencies.load(self._build), context.files)

    def label( self, context: CheckContext ) -> str:
        return f"{self.TOOL}-{self._build.name}"

    def cwd( self, context: CheckContext ) -> Path | None:
        return self._build  # relative `../Source/..` paths in diagnostics resolve against the build directory

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
            *self.ARGUMENTS, *context.arguments.extra_arg,
        )

    def scope( self, context: CheckContext, path: Path ) -> str:
        """The result depends on the exact compile flags and on every project header the file includes."""
        entry = self._database.get(path)
        command, output = (entry.portable(context.workspace), entry.output) if entry else ("", "")
        return Digest.of(command, self._scope.digest(output))

    def command( self, context: CheckContext, path: Path ) -> tuple[str, ...]:
        return (
            context.executable.path, "-p", str(self._build), f"--config-file={self._config}",
            f"--header-filter={self._header_filter}", *self.ARGUMENTS,
            *(f"--extra-arg={extra}" for extra in context.arguments.extra_arg), str(path),
        )


if __name__ == "__main__":
    CheckClangTidy.run()
