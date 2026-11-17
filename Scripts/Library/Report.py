from __future__ import annotations

from dataclasses import replace

from .Diagnostics import Diagnostic
from .Runner import JobResult
from .Workflow import Workflow
from .Workspace import Workspace

__all__ = ["RunReporter"]


class RunReporter:
    """Turns job results into console output, GitHub annotations and a step summary."""

    ANNOTATION_LIMIT = 40
    SUMMARY_LIMIT = 50

    def __init__( self, workspace: Workspace, workflow: Workflow, title: str, total: int, verbose: bool = False ) -> None:
        self._workspace = workspace
        self._workflow = workflow
        self._title = title
        self._total = total
        self._verbose = verbose
        self._passed = 0
        self._failures: list[JobResult] = []
        self._annotated: set[Diagnostic] = set()

    def record( self, result: JobResult ) -> None:
        name = self._workspace.relative(result.job.path)
        if result.ok:
            self._passed += 1
            if self._verbose:
                print(f"  ok  {name}")
            return
        self._failures.append(result)
        with self._workflow.group(f"FAIL {name}"):
            print(result.output)
        self._annotate(result)

    def finish( self, hint: str, elapsed: float ) -> int:
        failed = len(self._failures)
        cancelled = self._total - self._passed - failed
        print(
            f"{self._title}: {self._passed} passed, {failed} failed"
            + (f", {cancelled} cancelled" if cancelled else "")
            + f" in {elapsed:.2f}s"
        )
        if not failed:
            return 0
        names = sorted(self._workspace.relative(result.job.path) for result in self._failures)
        lines = [f"## {self._title}: {failed} file(s) failed", ""]
        lines += [f"- `{name}`" for name in names[: self.SUMMARY_LIMIT]]
        if failed > self.SUMMARY_LIMIT:
            lines.append(f"- ... and {failed - self.SUMMARY_LIMIT} more")
        if hint:
            lines += ["", hint]
        self._workflow.summary("\n".join(lines) + "\n")
        self._workflow.error(f"{self._title} failed", f"{failed} file(s) failed, see the log for details.")
        return 1

    def _annotate( self, result: JobResult ) -> None:
        if not self._workflow.active:
            return
        base = result.job.cwd or self._workspace.root
        for diagnostic in Diagnostic.parse(result.output):
            if len(self._annotated) >= self.ANNOTATION_LIMIT:
                return
            name = self._workspace.relative(self._workspace.absolute(diagnostic.file, base))
            located = replace(diagnostic, file=name)
            if name.startswith("..") or located in self._annotated:  # outside the repo / header seen by several TUs
                continue
            self._annotated.add(located)
            self._workflow.annotate(name, located.line, located.column, self._title, located.message)
