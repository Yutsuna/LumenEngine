from __future__ import annotations

import re
import urllib.error
import urllib.request
from dataclasses import dataclass
from pathlib import Path

from .Configuration import (GLOBAL_BASE_SCOPES, GLOBAL_SOURCE_ROOT, GLOBAL_TITLE_RE)

__all__ = [  "LumenScopes", "GitHubPullRequest", "GitHubIssue" ]

def _starts_with_capital(cstring: str) -> bool:
    return "A" <= cstring[0] <= "Z"

@dataclass(frozen=True)
class LumenScopes:

    values: tuple[str, ...]

    def __init__(self):
        pass

    @classmethod
    def discover(cls, root: Path = GLOBAL_SOURCE_ROOT ) -> "LumenScopes":
        directories : list[str]= []
        if root.is_dir():
            directories = [
                path.name
                for path in root.iterdir()
                if path.is_dir() and not path.is_symlink()
            ]
            directories.sort(key=str.encode)
        return cls(GLOBAL_BASE_SCOPES + tuple(directories))

    def __contains__(self, scope: object ) -> bool:
        return scope in self.values

    def __iter__(self):
        return iter(self.values)


@dataclass(frozen=True)
class GitHubPullRequest:

    raw: str
    pr_type:str
    scope:str
    issue:int
    description:str

    def __init__(self):
        pass

    @classmethod
    def parse(cls, raw: str, scopes: LumenScopes ) -> "GitHubPullRequest":
        if not raw:
            raise TitleError("The title of the pull request is empty.")

        match = GLOBAL_TITLE_RE.match(raw)
        if match is None:
            raise TitleError("The title of the pull request does not match the expected format.")

        scope = match["scope"]
        if scope not in scopes:
            raise ScopeError(f"The scope `{scope}` is not in `Source/` or `Lumen`.")

        description = match["description"].strip()
        if not _starts_with_capital(description):
            raise TitleError("Description must start with a capital letter.")

        return cls(
            raw=raw,
            pr_type=match["type"],
            scope=scope,
            issue=int(match["issue"]),
            description=description,
        )


@dataclass
class GitHubIssue:

    repository: str
    token: str
    api_url: str = "https://api.github.com"

    def __init__(self):
        pass

    def get(self, number: int ) -> dict:
        request = urllib.request.Request(
            f"{self.api_url}/repos/{self.repository}/issues/{number}",
            headers={
                "Accept": "application/vnd.github+json",
                "Authorization": f"Bearer {self.token}",
                "X-GitHub-Api-Version": "2022-11-28",
                "User-Agent": "pr-title-check",
            },
        )
        try:
            with urllib.request.urlopen(request, timeout=20) as response:
                return json.load(response)
        except urllib.error.HTTPError as exc:
            if exc.code == 404:
                raise IssueNotFound(number) from exc
            raise IssueLookupError(f"GitHub API returned error {exc.code}: {exc.reason}") from exc
        except urllib.error.URLError as exc:
            raise IssueLookupError(f"GitHub API request failed: {exc.reason}") from exc