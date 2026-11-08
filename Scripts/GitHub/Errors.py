__all__ = ["IssueLookupError", "IssueNotFound", "ScopeError", "TitleError"]


class TitleError(Exception):
    """The PR title does not follow the convention."""


class ScopeError(TitleError):
    """The scope is syntactically valid but not an allowed one."""


class IssueNotFound(Exception):
    """The referenced issue number does not exist in this repository."""


class IssueLookupError(Exception):
    """The GitHub API could not be queried."""
