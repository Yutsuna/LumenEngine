from __future__ import annotations

import time
from abc import abstractmethod
from argparse import ArgumentParser, Namespace
from contextlib import closing
from dataclasses import dataclass
from pathlib import Path
from typing import ClassVar

from .Cache import ResultCache
from .EntryPoint import EntryPoint
from .Executable import Executable
from .Hashing import Digest, FileDigests
from .Report import RunReporter
from .Runner import GLOBAL_DEFAULT_WORKERS, Job, ParallelRunner
from .Sources import GLOBAL_EXTENSIONS, SourceTree
from .Workflow import Workflow
from .Workspace import Workspace

__all__ = ["CheckContext", "SourceCheck"]


@dataclass(frozen=True)
class CheckContext:
    workspace: Workspace
    executable: Executable
    arguments: Namespace
    files: FileDigests


class SourceCheck(EntryPoint):
    """
    Runs one external tool over every source file, in parallel, skipping files already known valid.

    A file is skipped iff its key is unchanged, where key = fingerprint (tool, configuration)
    + scope (what else the result depends on) + the file's own content.
    Subclasses only describe their tool: the pipeline itself is shared.
    """

    TOOL: ClassVar[str]
    HINT: ClassVar[str] = ""
    SOURCE_DIR: ClassVar[str] = "Source"

    # ---- hooks ---------------------------------------------------------------------------------------------------

    @abstractmethod
    def fingerprint( self, context: CheckContext ) -> str:
        """Digest of everything global that influences results (tool version, configuration, flags)."""

    @abstractmethod
    def command( self, context: CheckContext, path: Path ) -> tuple[str, ...]:
        """Full command line analyzing `path`; exit code 0 means the file is valid."""

    def prepare( self, context: CheckContext ) -> None:
        """Validate prerequisites (raise ConfigurationError) and load tool-specific resources."""

    def select( self, context: CheckContext, sources: list[Path] ) -> list[Path]:
        return sources

    def scope( self, context: CheckContext, path: Path ) -> str:
        """Per-file digest of what else the result depends on (compile flags, included headers...)."""
        return ""

    def extensions( self, arguments: Namespace ) -> tuple[str, ...]:
        return GLOBAL_EXTENSIONS

    def label( self, context: CheckContext ) -> str:
        return self.TOOL

    def cwd( self, context: CheckContext ) -> Path | None:
        return None

    def rewrites( self, context: CheckContext ) -> bool:
        """True when the tool modifies the files it analyzes: the key is then recomputed after the run."""
        return False

    # ---- pipeline ------------------------------------------------------------------------------------------------

    def configure( self, parser: ArgumentParser ) -> None:
        super().configure(parser)
        parser.add_argument("paths", nargs="*", type=Path, help=f"Only these files (default: all under {self.SOURCE_DIR}/).")
        parser.add_argument("--root", type=Path, default=None, help="Repository root (default: $GITHUB_REPOSITORY_SOURCE, $GITHUB_WORKSPACE, then the parent of Scripts/).")
        parser.add_argument("--source-dir", default=self.SOURCE_DIR, help="Directory to scan, relative to the root.")
        parser.add_argument("--exclude", action="append", default=[], metavar="GLOB", help="fnmatch glob on the root-relative path, repeatable.")
        parser.add_argument("-j", "--jobs", type=int, default=GLOBAL_DEFAULT_WORKERS, help="Parallel workers.")
        parser.add_argument("--tool", default=None, metavar="PATH", help=f"{self.TOOL} executable (default: ${Executable.environment_variable(self.TOOL)}, then PATH).")
        parser.add_argument("--cache-dir", type=Path, default=None, help="Cache directory (default: $LINT_CACHE_DIR or <root>/.cache/Lint).")
        parser.add_argument("--no-cache", action="store_true", help="Ignore cached results (fresh results are still stored).")
        parser.add_argument("--fail-fast", action="store_true", help="Stop scheduling files after the first failure.")
        parser.add_argument("-v", "--verbose", action="store_true", help="Print every file that passes.")

    def execute( self, arguments: Namespace ) -> int:
        context = CheckContext(
            Workspace.locate(arguments.root), Executable.locate(self.TOOL, arguments.tool), arguments, FileDigests()
        )
        self.prepare(context)
        tree = SourceTree(
            context.workspace, context.workspace.root / arguments.source_dir, self.extensions(arguments), arguments.exclude
        )
        sources = self.select(context, tree.discover(arguments.paths))
        if not sources:
            print(f"{self.label(context)}: no files to analyze.")
            return 0
        return self._check(context, sources)

    def _check( self, context: CheckContext, sources: list[Path] ) -> int:
        started = time.perf_counter()
        label = self.label(context)
        arguments = context.arguments
        fingerprint = self.fingerprint(context)
        runner = ParallelRunner(arguments.jobs)
        cache = ResultCache.open(context.workspace, label, arguments.cache_dir, enabled=not arguments.no_cache)

        with cache:
            pending = [job for job in self._jobs(context, fingerprint, sources) if not cache.is_valid(job.path, job.key)]
            print(f"{label}: {len(sources)} files | {len(sources) - len(pending)} cached & valid | {len(pending)} to analyze | {runner.workers} workers")
            if not pending:
                return 0
            reporter = RunReporter(context.workspace, Workflow(), label, len(pending), arguments.verbose)
            with closing(runner.run(pending)) as results:
                for result in results:
                    self._settle(context, fingerprint, cache, result.job, result.ok)
                    reporter.record(result)
                    if arguments.fail_fast and not result.ok:
                        break
        return reporter.finish(self.HINT, time.perf_counter() - started)

    def _jobs( self, context: CheckContext, fingerprint: str, sources: list[Path] ) -> list[Job]:
        cwd = self.cwd(context)
        return [
            Job(path, self._key(context, fingerprint, path, context.files.of(path)), self.command(context, path), cwd)
            for path in sources
        ]

    def _key( self, context: CheckContext, fingerprint: str, path: Path, content: str ) -> str:
        return Digest.of(fingerprint, self.scope(context, path), content)

    def _settle( self, context: CheckContext, fingerprint: str, cache: ResultCache, job: Job, ok: bool ) -> None:
        if not ok:
            cache.forget(job.path)
        elif self.rewrites(context):
            cache.mark_valid(job.path, self._key(context, fingerprint, job.path, FileDigests.read(job.path)))
        else:
            cache.mark_valid(job.path, job.key)
