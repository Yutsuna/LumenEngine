from invoke import task

from LumenBuild.Constants import EAnsiColor as C, SANITIZER_FLAGS


@task(name="help")
def PrintHelp(ctx):
    sanitizers = " | ".join(SANITIZER_FLAGS)

    print(f"""
{C.BOLD}{C.CYAN}╔══════════════════════════════════════════════════════╗
║           Lumen Build System  v0.1.0                ║
╚══════════════════════════════════════════════════════╝{C.RESET}

{C.BOLD}USAGE{C.RESET}
  lumen <command>[.<subcommand>] [options]

{C.BOLD}BUILD COMMANDS{C.RESET}

  {C.GREEN}lumen build.debug{C.RESET}  (alias: {C.GREEN}build.d{C.RESET})
      Configure + compile in Debug mode.

  {C.GREEN}lumen build.debug --sanitizer <s>{C.RESET}
      Debug build with a sanitizer.
      Available: {C.YELLOW}{sanitizers}{C.RESET}

  {C.GREEN}lumen build.debug --target <Target>{C.RESET}
      Build a single CMake target (e.g. Core, Renderer).

  {C.GREEN}lumen build.debug --testing{C.RESET}
      Enable test targets (requires GoogleTest).

  {C.GREEN}lumen build.release{C.RESET}  (alias: {C.GREEN}build.r{C.RESET})
      Configure + compile in Release mode (-O3).

  {C.GREEN}lumen build.release --lto{C.RESET}
      Release build with Link-Time Optimisation.

  {C.GREEN}lumen build.help{C.RESET}
      Detailed build subcommand reference.

{C.BOLD}OTHER COMMANDS{C.RESET}

  {C.GREEN}lumen test{C.RESET}
      Run all CTest tests (parallel, Debug build).

  {C.GREEN}lumen test --filter <regex>{C.RESET}
      Run only tests matching the pattern.

  {C.GREEN}lumen format{C.RESET}
      Run clang-format in parallel over all C++ sources.

  {C.GREEN}lumen format --check{C.RESET}
      Verify formatting without modifying files (CI mode).

  {C.GREEN}lumen clean{C.RESET}
      Wipe the entire Build/ directory.

  {C.GREEN}lumen clean --debug{C.RESET}
      Remove Debug/ build artefacts only.

  {C.GREEN}lumen clean --release{C.RESET}
      Remove Release/ build artefacts only.
""")
