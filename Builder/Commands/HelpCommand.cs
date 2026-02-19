namespace LumenBuilder.Commands;

/// <summary>
/// Help command implementation.
/// </summary>
public sealed class HelpCommand
{

    /// <summary>
    /// Executes the help command, writing output to the given TextWriter.
    /// </summary>
    public void Execute(TextWriter Output)
    {
        Output.WriteLine(HelpText);
    }

    private const string HelpText = """
    LumenBuilder - Modular C++ Build System

    Usage: LumenBuilder <command> [options]

    Commands:
      build     Generate build files (Ninja, Makefile, or CMake)
      clean     Remove intermediate and binary directories
      help      Show this help message

    Build Options:
      --format <ninja|makefile|cmake>  Output format (default: ninja)
      --config <debug|dev|release>     Build configuration (default: debug)
      --toolchain <clang|g++>          Compiler toolchain (default: auto)
      --output <dir>                   Output directory (default: .)
      --root <dir>                     Root directory (default: .)

    Examples:
      LumenBuilder build --format ninja --config release
      LumenBuilder build --format cmake
      LumenBuilder clean

    """;

}
