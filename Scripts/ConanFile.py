from pathlib import Path
from typing import Any, cast

from conan import ConanFile
from Conan.DependencyResolver import DependencyResolver
from Conan.Packager import Packager


class LumenEngineConan(ConanFile):
    name = "lumenengine"
    version = "0.1.0"
    author = "Yutsuna"
    url = "https://github.com/Leorevoir/LumenEngine"
    description = "A highly modular Game Engine written in C++23"

    settings = ("os", "compiler", "build_type", "arch")
    generators = ("CMakeDeps", "PkgConfigDeps")
    exports = "Conan/*"

    def requirements(self) -> None:
        resolver = DependencyResolver(self._safe_recipe_folder, self.settings)
        deps = resolver.resolve()

        for req in deps.requires:
            fn_requires = cast(Any, self.requires)
            fn_requires(req)

    def build(self) -> None:
        self.output.info("Build step is handled by an external orchestrator.")

    def package(self) -> None:
        Packager(self).execute()

    def package_info(self) -> None:
        resolver = DependencyResolver(self._safe_recipe_folder, self.settings)
        deps = resolver.resolve()

        self.cpp_info.libs = deps.libs
        self.cpp_info.defines = deps.defines
        self.cpp_info.bindirs = ["bin"]
        self.cpp_info.libdirs = ["lib"]
        self.cpp_info.includedirs = ["include"]

        if deps.pkg_config_name:
            self.cpp_info.set_property("pkg_config_name", deps.pkg_config_name)

    @property
    def _safe_recipe_folder(self) -> Path:
        folder = cast(str, self.recipe_folder)
        return Path(folder)
