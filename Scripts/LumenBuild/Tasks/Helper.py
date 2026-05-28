from invoke import task

from LumenBuild.Constants import SANITIZER_FLAGS
from LumenBuild.Constants import EAnsiColor as C


def _GetVersion() -> str:
    try:
        with open("VERSION.md", "r") as f:
            return f.read().strip()
    except Exception:
        return "unknown"


@task(name="help")
def PrintHelp(ctx):
    version = _GetVersion()
    sanitizers = " | ".join(SANITIZER_FLAGS)

    print(f"""
{C.BOLD}{C.CYAN}Lumen Build System v{version}{C.RESET}

{C.BOLD}USAGE{C.RESET}
  lumen <command>[.<subcommand>] [options]

{C.BOLD}INITIALIZATION COMMANDS{C.RESET}

  {C.GREEN}lumen init <name> [--directory <dir>] [--force]{C.RESET}
    Create a new Lumen project with the specified name.

{C.BOLD}BUILD COMMANDS{C.RESET}

  {C.GREEN}lumen build.debug [--sanitizer <s>] [--target <Target>] [--testing]{C.RESET}
    Build in Debug mode (alias lumen build.d)
    Where valid sanitizers are:
    {C.YELLOW}{sanitizers}{C.RESET}

  {C.GREEN}lumen build.release [--lto] [--target <Target>] [--testing]{C.RESET}
    Build in Release mode (alias lumen build.r)

{C.BOLD}TESTING COMMANDS{C.RESET}

  {C.GREEN}lumen test [--filter <regex>]{C.RESET}
    Run LumenEngine's CTest-based unit tests.

{C.BOLD}OTHER COMMANDS{C.RESET}

  {C.GREEN}lumen clean [--debug | --release]{C.RESET}
    Wipe the entire [Build/<Config>] directory.
    By default, both Debug and Release artefacts are removed.

  {C.GREEN}lumen format [--check]{C.RESET}
    Run clang-format in parallel over all C++ sources.

  {C.GREEN}lumen tidy --module <Module>{C.RESET}
    Run clang-tidy on a specific module (e.g. Core, Renderer).
""")
