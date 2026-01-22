using LumenBuilder.Commands;
using LumenBuilder.Common;

namespace LumenBuilder
{

    /// <summary>
    /// Main entry point for LumenBuilder.
    /// </summary>
    public static class Program
    {

        const int BuildSuccessCode = 0;
        const int BuildExceptionCode = 84;

        public static int Main(string[] Args)
        {
            var Diagnostics = new DiagnosticBag();

            if (Args.Length == 0)
            {
                new HelpCommand().Execute(Console.Out);
                return BuildSuccessCode;
            }

            string Command = Args[0].ToLowerInvariant();

            try
            {
                return Command switch
                {
                    "build" => RunBuild(Args, Diagnostics),
                    "clean" => RunClean(Args, Diagnostics),
                    "help" or "--help" or "-h" => RunHelp(),
                    _ => UnknownCommand(Command)
                };
            }
            catch (BuildException Ex)
            {
                Diagnostics.Error(Ex.Message);
                PrintDiagnostics(Diagnostics);
                return BuildExceptionCode;
            }
            catch (Exception Ex)
            {
                Console.Error.WriteLine($"Fatal error: {Ex.Message}");
                return BuildExceptionCode;
            }
            finally
            {
                PrintDiagnostics(Diagnostics);
            }
        }

        private static int RunBuild(string[] Args, DiagnosticBag Diagnostics)
        {
            var Options = ParseBuildOptions(Args);
            var BuildCmd = new BuildCommand(Diagnostics);
            return BuildCmd.Execute(Options);
        }

        private static int RunClean(string[] Args, DiagnosticBag Diagnostics)
        {
            string RootDir = GetArgValue(Args, "--root") ?? Directory.GetCurrentDirectory();
            var CleanCmd = new CleanCommand(Diagnostics);
            return CleanCmd.Execute(RootDir);
        }

        private static int RunHelp()
        {
            new HelpCommand().Execute(Console.Out);
            return BuildSuccessCode;
        }

        private static int UnknownCommand(string Command)
        {
            Console.Error.WriteLine($"Unknown command: {Command}");
            Console.Error.WriteLine("Use 'LumenBuilder help' for usage information.");
            return BuildExceptionCode;
        }

        private static BuildOptions ParseBuildOptions(string[] Args)
        {
            string RootDir = GetArgValue(Args, "--root") ?? Directory.GetCurrentDirectory();
            string OutputDir = GetArgValue(Args, "--output") ?? RootDir;
            string ConfigStr = GetArgValue(Args, "--config") ?? "debug";
            string? Toolchain = GetArgValue(Args, "--toolchain");
            string FormatStr = GetArgValue(Args, "--format") ?? "ninja";

            if (Args.Length > 1 && !Args[1].StartsWith("--"))
            {
                FormatStr = Args[1];
            }

            OutputFormat Format = FormatStr.ToLowerInvariant() switch
            {
                "makefile" or "make" => OutputFormat.Makefile,
                "cmake" => OutputFormat.CMake,
                _ => OutputFormat.Ninja
            };

            BuildConfiguration Config = ConfigStr.ToLowerInvariant() switch
            {
                "dev" or "development" => BuildConfiguration.Development,
                "release" => BuildConfiguration.Release,
                _ => BuildConfiguration.Debug
            };

            string[] SourceRoots = new[]
            {
                Paths.Combine(RootDir, "LumenEngine"),
                Paths.Combine(RootDir, "Examples")
            };

            return new BuildOptions(
                SourceRoots,
                RootDir,
                OutputDir,
                Config,
                Format,
                Toolchain
            );
        }

        private static string? GetArgValue(string[] Args, string Name)
        {
            for (int ArgIndex = 0; ArgIndex < Args.Length - 1; ++ArgIndex)
            {
                if (Args[ArgIndex].Equals(Name, StringComparison.OrdinalIgnoreCase))
                    return Args[ArgIndex + 1];
            }
            return null;
        }

        private static void PrintDiagnostics(DiagnosticBag Diagnostics)
        {
            var All = Diagnostics.GetAll();

            for (int DiagIndex = 0; DiagIndex < All.Count; DiagIndex++)
            {
                var Diag = All[DiagIndex];

                TextWriter Output = Diag.Severity == DiagnosticSeverity.Error
                    ? Console.Error
                    : Console.Out;

                Output.WriteLine(Diag.ToString());
            }

        }

    }

}
