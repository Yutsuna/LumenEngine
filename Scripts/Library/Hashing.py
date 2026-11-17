from __future__ import annotations

import hashlib
import os
from pathlib import Path

__all__ = ["Digest", "FileDigests"]


class Digest:
    SIZE = 16

    @classmethod
    def of( cls, *parts: str | bytes ) -> str:
        """Order-sensitive, length-prefixed digest: ("ab", "c") never collides with ("a", "bc")."""
        hasher = hashlib.blake2b(digest_size=cls.SIZE)
        for part in parts:
            data = part.encode("utf-8") if isinstance(part, str) else part
            hasher.update(len(data).to_bytes(8, "little"))
            hasher.update(data)
        return hasher.hexdigest()


class FileDigests:
    """Memoised file hashing: a header included by 500 translation units is read once."""

    MISSING = "missing"

    def __init__( self ) -> None:
        self._memo: dict[str, str] = {}

    def of( self, path: str | Path ) -> str:
        key = os.fspath(path)
        digest = self._memo.get(key)
        if digest is None:
            digest = self._memo[key] = self.read(key)
        return digest

    @classmethod
    def read( cls, path: str | Path ) -> str:
        """Uncached: use after a file was rewritten."""
        try:
            return Digest.of(Path(path).read_bytes())
        except OSError:
            return cls.MISSING
