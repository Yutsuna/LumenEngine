from __future__ import annotations

import os
from functools import cached_property

from Library import Digest, FileDigests, SourceTree, Workspace

from .NinjaDependencies import NinjaDependencies

__all__ = ["IncludeScope"]


class IncludeScope:
    """
    Digest of the project headers a translation unit includes.
    Without a dependency log (never built, no ninja) it degrades to the digest of every header: slower, never wrong.
    """

    def __init__( self, workspace: Workspace, headers: SourceTree, dependencies: NinjaDependencies, files: FileDigests ) -> None:
        self._workspace = workspace
        self._headers = headers
        self._dependencies = dependencies
        self._files = files
        self._prefix = Workspace.normalize(workspace.root) + os.sep

    def digest( self, object_file: str ) -> str:
        return self._included(object_file) or self._every_header

    def _included( self, object_file: str ) -> str:
        parts = [
            f"{dependency[len(self._prefix):]}={self._files.of(dependency)}"
            for dependency in sorted(set(self._dependencies.of(object_file)))
            if dependency.startswith(self._prefix)
        ]
        return Digest.of(*parts) if parts else ""

    @cached_property
    def _every_header( self ) -> str:
        return Digest.of(*(f"{self._workspace.relative(header)}={self._files.of(header)}" for header in self._headers.discover()))
