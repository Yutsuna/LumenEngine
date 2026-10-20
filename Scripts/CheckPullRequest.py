from __future__ import annotations

import os
import sys
from Github.Configuration import *
from Github.Models import GitHubIssue
from argparse import ArgumentParser, Namespace

from Scripts.GitHub import annotate_error

GLOBAL_EXAMPLE = "Feat(AppCore): #142 Add SDL3 window support"

def _verify_issue(number: int) -> int:
    repository = os.environ.get("GITHUB_REPOSITORY")
    if not repository:
        annotate_error("Missing configuration", "Need: GITHUB_REPOSITORY environment variable to be set.")
        return 1

    token = os.environ.get("GH_TOKEN") or os.environ.get("GITHUB_TOKEN")
    if not token:
        annotate_error("Missing configuration", "Need: GH_TOKEN or GITHUB_TOKEN environment variable to be set.")
        return 1

    client = GitHubIssue(
        repository=repository,
        token=token,
        api_url=os.environ.get("GITHUB_API_URL", "https://api.github.com"),
    )

    try:
        issue = client.get(number)
    except IssueNotFound:
        annotate_error("Unknown issue", f"{number} is not found in {repository}.")
        return 1
    except IssueLookupError as exc:
        annotate_error("Issue lookup failed", f"Unable to read #{number}: {exc}")
        return 1

    if "pull_request" in issue:
        annotate_error("Not an issue", f"#{number} is a Pull Request, please change to reference Issue number.")
        return 1

    print(f"  #{number} is a valid Issue.")
    return 0


def _report_invalid_title(title: str, reason: str) -> int:
    annotate_error("Invalid PR title", reason)
    write_summary(
        "\n".join(
            [
                "## PR Title does not match the required format",
                "",
                f"**Current title:** `{title}`",
                "",
                f"**Failure reason:** {reason}",
                "",
                "**Required format:** `<PRType>(<PRScope>): <IssueNumber> <Description>`",
                "",
                "- `PRType` ∈ Chore / Fix / Feat / CI / Test / Style / BREAKING",
                "- `PRScope` ∈ `Source/` 下的一级目录名，或 `Lumen`",
                "- `IssueNumber` 形如 `#123`，必须是关联的 Issue 编号",
                "- `Description` 首字母大写",
                "",
                f"**Example:** `{EXAMPLE}`",
                "",
            ]
        )
    )
    return 1


def _parse_arguments( argv: list[str] ) -> Namespace:
    parser = ArgumentParser(description="Ensure the Pull Request format is valid.")
    parser.add_argument(
        "--skip-issue-check",
        action="store_true",
        help="Skip checking the existence of the associated issue."
    )
    parser.add_argument(
        "--skip-title-check",
        action="store_true",
        help="Skip checking the format of the PR title."
    )
    return parser.parse_args(argv)


def _scope_error(scopes: LumenScopes, title : str, exception : str)->None:
    print("Allowed Scopes: ")
    for scope in scopes:
        print(f"  - {scope}")
    return _report_invalid_title(title, str(exception))

def main(argv: list[str] ) -> int:
    args = _parse_arguments(argv)
    title = os.environ.get("PR_TITLE", "")
    scopes = LumenScopes.discover(args.source_root)

    try:
        parsed = PRTitle.parse(title, scopes)
    except ScopeError as exception:
        return _scope_error(scopes, title, str(exception))
    except TitleError as exception:
        return _report_invalid_title(title, str(exception))

    set_output("type", parsed.pr_type)
    set_output("scope", parsed.scope)
    set_output("issue", parsed.issue)
    set_output("description", parsed.description)

    print(f"""
Pull Request is valid.
PullRequest Type = {parsed.pr_type}
PullRequest Scope = {parsed.scope}
Issue Number = #{parsed.issue}
Description = {parsed.description}
    """)

    if args.skip_issue_check:
        return 0
    return _verify_issue(parsed.issue)


if __name__ == "__main__":
    sys.exit(main( sys.argv[1:] ))
