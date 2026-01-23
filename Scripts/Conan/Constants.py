from typing import Final, List

__all__ = ["LIB_PATTERNS", "HEADER_PATTERNS"]

LIB_PATTERNS: Final[List[str]] = ["*.a", "*.so", "*.dylib", "*.lib"]
HEADER_PATTERNS: Final[List[str]] = ["*.hpp", "*.h", "*.inl"]
