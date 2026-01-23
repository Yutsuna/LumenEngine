from pathlib import Path
from typing import TYPE_CHECKING, cast

from conan.tools.files import copy
from Conan.Constants import HEADER_PATTERNS, LIB_PATTERNS

if TYPE_CHECKING:
    from conan import ConanFile


class Packager:

    def __init__(self, conan_file: "ConanFile"):
        self.conan = conan_file
        recipe_folder = Path(cast(str, self.conan.recipe_folder))

        self.project_root = recipe_folder.parent
        self.source_dir = self.project_root / "LumenEngine" / "Source"
        self.bin_dir = self.project_root / "Binaries"
        self.int_dir = self.project_root / "Intermediate"

    def execute(self) -> None:
        self._pack_libraries()
        self._pack_binaries()
        self._pack_headers()

    def _pack_libraries(self) -> None:
        for folder in (self.bin_dir, self.int_dir):

            if not folder.exists():
                continue

            for pattern in LIB_PATTERNS:
                copy(self.conan, pattern, src=str(folder), dst="lib", keep_path=True)

    def _pack_binaries(self) -> None:
        if self.bin_dir.exists():
            copy(self.conan, "*", src=str(self.bin_dir), dst="bin", keep_path=True)

    def _pack_headers(self) -> None:
        if not self.source_dir.exists():
            return

        for pattern in HEADER_PATTERNS:
            copy(
                self.conan,
                pattern,
                src=str(self.source_dir),
                dst="include",
                keep_path=True,
            )

