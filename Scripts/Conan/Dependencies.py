from typing import Any, Dict, Final, List, Optional
from dataclasses import dataclass, field

__all__ = ["DEFAULT_PLATFORM_DEPS", "DependencyConfig"]

DEFAULT_PLATFORM_DEPS: Final[Dict[str, Dict[str, Any]]] = {
    "Linux": {
        "requires": ["sdl/3.2.20"],
        "libs": ["SDL3"],
        "defines": ["LINUX_BUILD"],
        "pkg_config_name": "sdl3",
    },
    "Macos": {
        "requires": ["sdl/3.2.20"],
        "libs": ["SDL3"],
        "defines": ["MACOS_BUILD"],
        "pkg_config_name": "sdl3",
    },
    "Windows": {
        "requires": ["sdl/3.2.20"],
        "libs": ["SDL3"],
        "defines": ["WINDOWS_BUILD"],
    },
}


@dataclass(frozen=True)
class DependencyConfig:
    requires: List[str] = field(default_factory=list)
    libs: List[str] = field(default_factory=list)
    defines: List[str] = field(default_factory=list)
    pkg_config_name: Optional[str] = None
