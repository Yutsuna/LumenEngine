import re
from pathlib import Path

GLOBAL_PR_TYPES: tuple[str,...] = ("Chore", "Fix", "Feat", "CI", "Test", "Style", "BREAKING")
GLOBAL_BASE_SCOPES: tuple[str, ...] = ("Lumen",)
GLOBAL_SOURCE_ROOT = Path("Source")
GLOBAL_TITLE_RE = re.compile(
    rf"^(?P<type>{'|'.join(GLOBAL_PR_TYPES)})"
    r"\((?P<scope>[A-Za-z0-9._-]+)\): "
    r"#(?P<issue>[0-9]+) "
    r"(?P<description>[A-Z][^\s].*)$"
)
