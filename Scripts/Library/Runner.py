from __future__ import annotations

import os
import subprocess
from collections.abc import Iterator, Sequence
from concurrent.futures import ThreadPoolExecutor, as_completed
from dataclasses import dataclass
from pathlib import Path

__all__ = ["GLOBAL_DEFAULT_WORKERS", "Job", "JobResult", "ParallelRunner"]

GLOBAL_DEFAULT_WORKERS: int = os.cpu_count() or 4


@dataclass(frozen=True)
class Job:
    path: Path
    key: str
    command: tuple[str, ...]
    cwd: Path | None = None


@dataclass(frozen=True)
class JobResult:
    job: Job
    returncode: int
    output: str

    @property
    def ok( self ) -> bool:
        return self.returncode == 0


class ParallelRunner:
    """Runs jobs concurrently (threads are enough: the work happens in the subprocesses)."""

    def __init__( self, workers: int ) -> None:
        self._workers = max(1, workers)

    @property
    def workers( self ) -> int:
        return self._workers

    def run( self, jobs: Sequence[Job] ) -> Iterator[JobResult]:
        """Yields results as they complete. Closing the iterator early cancels every job not yet started."""
        if not jobs:
            return
        pool = ThreadPoolExecutor(max_workers=min(self._workers, len(jobs)))
        try:
            # Biggest files first: the slowest jobs start early, so the pool does not idle on a long tail.
            futures = [pool.submit(self._execute, job) for job in sorted(jobs, key=self._weight, reverse=True)]
            for future in as_completed(futures):
                yield future.result()
        finally:
            pool.shutdown(wait=True, cancel_futures=True)

    @staticmethod
    def _weight( job: Job ) -> int:
        try:
            return job.path.stat().st_size
        except OSError:
            return 0

    @staticmethod
    def _execute( job: Job ) -> JobResult:
        try:
            completed = subprocess.run(
                job.command, cwd=job.cwd, capture_output=True, text=True, errors="replace",
                stdin=subprocess.DEVNULL, check=False,
            )
        except OSError as exception:
            return JobResult(job, 127, f"Unable to launch {job.command[0]}: {exception}")
        return JobResult(job, completed.returncode, (completed.stdout + completed.stderr).strip())
