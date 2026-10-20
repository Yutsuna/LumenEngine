import os

__all__ = [ "set_output", "set_multiline_output", "write_summary", "annotate_error" ]

def _append( env_var : str, text: str ) -> None:
    path = os.environ.get( env_var )
    if not path:
        return
    with open(path, "a", encoding="utf-8") as stream:
        stream.write(text)


def set_output( name: str, value: object ) -> None:
    _append("GITHUB_OUTPUT", f"{name}={value}\n")


def set_multiline_output(name: str, value: str, delimiter: str = "EOF_SCOPES") -> None:
    _append("GITHUB_OUTPUT", f"{name}<<{delimiter}\n{value}\n{delimiter}\n")


def write_summary(markdown: str) -> None:
    _append("GITHUB_STEP_SUMMARY", markdown)


def annotate_error(title: str, message: str) -> None:
    print(f"::error title={title}::{message}")
