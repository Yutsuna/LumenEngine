__all__ = ["ConfigurationError"]


class ConfigurationError(Exception):
    """A prerequisite (file, tool, build directory) is missing or invalid."""

    def __init__( self, title: str, message: str ) -> None:
        super().__init__(message)
        self.title = title
