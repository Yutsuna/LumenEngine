import json
from pathlib import Path
from typing import Any, cast

from Conan.Dependencies import DEFAULT_PLATFORM_DEPS, DependencyConfig


class DependencyResolver:
    def __init__(self, recipe_folder: Path, settings: Any):
        self.recipe_folder = recipe_folder
        self.settings = settings

    def resolve(self) -> DependencyConfig:
        os_name = self._normalized_os
        base_config = DEFAULT_PLATFORM_DEPS.get(os_name, {}).copy()

        deps_dir = self.recipe_folder.parent / "Dependencies"
        json_path = deps_dir / f"{os_name}.json"

        if json_path.exists():
            try:
                with json_path.open("r", encoding="utf-8") as f:
                    base_config.update(json.load(f))
            except (json.JSONDecodeError, OSError) as e:
                print(
                    f"Warning: Could not parse dependency override at {json_path}: {e}"
                )

        return DependencyConfig(
            requires=base_config.get("requires", []),
            libs=base_config.get("libs", []),
            defines=base_config.get("defines", []),
            pkg_config_name=base_config.get("pkg_config_name"),
        )

    @property
    def _normalized_os(self) -> str:
        raw_os = str(cast(Any, self.settings).get_safe("os", "linux")).lower()

        if raw_os.startswith("win"):
            return "Windows"

        if raw_os.startswith("mac"):
            return "Macos"

        return "Linux"
