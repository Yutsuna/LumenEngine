import sys
from pathlib import Path
from typing import NoReturn

ERROR: int = 84
SUCCESS: int = 0


def fatal(msg: str) -> NoReturn:
    print(f"ERROR: {msg}", file=sys.stderr)
    sys.exit(ERROR)


CURRENT_SCRIPT_DIR: Path = Path(__file__).resolve().parent
SOURCE_DIR_PATH: Path = (CURRENT_SCRIPT_DIR / ".." / "Source").resolve()

MESON_TEMPLATE: str = """#{
# Source/$MODULE$/meson.build
# Source/$MODULE$ meson entry-point
#}

ModuleDefinition = {
    'Name' : '$MODULE$',
    'Type' : 'SharedLibrary',
    'SourceDir' : meson.current_source_dir(),
    'Private' : 'Private',
    'Public' : 'Public',
    'Tests' : 'Tests',
    'Depends' : []
}
"""


def write_file(path: Path, content: str) -> None:
    path.write_text(content)


def make_dirs(*paths: Path) -> None:
    for path in paths:
        path.mkdir(parents=True, exist_ok=True)


def render_meson_build(module_name: str) -> str:
    return MESON_TEMPLATE.replace("$MODULE$", module_name)


def create_module_meson_build(module_path: Path) -> None:
    write_file(module_path / "meson.build", render_meson_build(module_path.name))


def create_module(module_name: str) -> None:
    module_path: Path = SOURCE_DIR_PATH / module_name
    if module_path.exists():
        fatal(f"Module '{module_name}' already exists at {module_path}")

    make_dirs(module_path / "Public", module_path / "Private")
    create_module_meson_build(module_path)


def parse_args(argv: list[str]) -> str:
    args: list[str] = argv[1:]
    if len(args) != 1:
        fatal(f"Incorrect number of arguments, expected 1 module name got {len(args)}")
    return args[0]


def main() -> int:
    module_name = parse_args(sys.argv)

    if not SOURCE_DIR_PATH.exists():
        fatal(f"Source directory does not exist: {SOURCE_DIR_PATH}")

    create_module(module_name)
    return SUCCESS


if __name__ == "__main__":
    sys.exit(main())
