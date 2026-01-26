using LumenBuilder.Build;
using LumenBuilder.Build.Toolchain;
using LumenBuilder.Common;
using LumenBuilder.Dependencies;

using LumenBuilder.Emit.Ide;
using LumenBuilder.Emit.Makefile;
using LumenBuilder.Emit.Ninja;

using LumenBuilder.IO;
using LumenBuilder.Parsing;

namespace LumenBuilder
{

    namespace Commands
    {

        /// <summary>
        /// Build command options.
        /// </summary>
        public sealed record BuildOptions(
            string[] SourceRoots,
            string RootDirectory,
            string OutputDirectory,
            BuildConfiguration Configuration,
            OutputFormat OutputFormat,
            string? Toolchain
        );

        /// <summary>
        /// Output format for build files.
        /// </summary>
        public enum OutputFormat
        {
            Ninja,
            Makefile,
            CMake
        }

        /// <summary>
        /// Build command implementation.
        /// </summary>
        public sealed class BuildCommand
        {
            public const int BuildSuccessCode = 0;
            public const int BuildFailureCode = 84;
            private readonly DiagnosticBag Diagnostics;

            /// <summary>
            /// Creates a new build command.
            /// </summary>
            public BuildCommand(DiagnosticBag Diagnostics)
            {
                this.Diagnostics = Diagnostics;
            }

            /// <summary>
            /// Executes the build command with the given options.
            /// </summary>
            public int Execute(BuildOptions Options)
            {
                Diagnostics.Info($"Starting build: {Options.OutputFormat}");

                var Scanner = new FileScanner(Diagnostics);
                var BuildFiles = Scanner.FindBuildFiles(Options.SourceRoots);

                if (!BuildFiles.Any())
                {
                    Diagnostics.Error("No .build files found");
                    return BuildFailureCode;
                }

                var Parser = new BuildFileParser();
                var Loader = new BuildFileLoader(Parser, Diagnostics);
                var Graph = Loader.LoadAll(BuildFiles);

                if (Diagnostics.HasErrors)
                {
                    return BuildFailureCode;
                }

                Diagnostics.Info($"Loaded {Graph.Count} modules");

                var ValidationResult = BuildFileValidator.ValidateAll(Graph.Modules.Values);
                if (!ValidationResult.IsValid)
                {
                    for (int ErrorIndex = 0; ErrorIndex < ValidationResult.Errors.Count; ErrorIndex++)
                    {
                        var Error = ValidationResult.Errors[ErrorIndex];
                        Diagnostics.Error($"[{Error.ModuleName}] {Error.Message}");
                    }
                    return BuildFailureCode;
                }

                var Platform = PlatformInfo.Detect();
                var Resolver = new ToolchainResolver();
                var Toolchain = Options.Toolchain != null
                    ? Resolver.Resolve(Options.Toolchain)
                    : Resolver.Resolve(Platform);

                var ExternalDeps = new DependencyResolver(Options.RootDirectory, Platform);

                var Context = new BuildContext(
                    Graph,
                    Platform,
                    Options.Configuration,
                    Toolchain,
                    Options.RootDirectory,
                    Options.OutputDirectory,
                    Diagnostics,
                    ExternalDeps
                );

                var Planner = new BuildPlanner(Context);
                var Plan = Planner.CreatePlan();

                if (Diagnostics.HasErrors)
                {
                    return BuildFailureCode;
                }

                Diagnostics.Info($"Planned {Plan.CompileUnits.Count} compile units, {Plan.LinkTargets.Count} link targets");

                switch (Options.OutputFormat)
                {
                    case OutputFormat.Ninja:
                        WriteToNinja(Context, Plan, Options);
                        break;

                    case OutputFormat.Makefile:
                        WriteToMakefile(Context, Plan, Options);
                        break;

                    case OutputFormat.CMake:
                        WriteToCMake(Context, Plan, Options);
                        break;
                }

                return BuildSuccessCode;
            }

            /// <summary>
            /// Writes the build plan to a Ninja build file.
            /// </summary>
            private void WriteToNinja(BuildContext Context, BuildPlan Plan, BuildOptions Options)
            {
                var NinjaWriter = new NinjaWriter(Context);

                NinjaWriter.Write(Paths.Combine(Options.OutputDirectory, "build.ninja"), Plan);
                Diagnostics.Info("Generated build.ninja");
            }

            /// <summary>
            /// Writes the build plan to a Makefile.
            /// </summary>
            private void WriteToMakefile(BuildContext Context, BuildPlan Plan, BuildOptions Options)
            {
                var MakefileWriter = new MakefileWriter(Context);

                MakefileWriter.Write(Paths.Combine(Options.OutputDirectory, "Makefile"), Plan);
                Diagnostics.Info("Generated Makefile");
            }

            /// <summary>
            /// Writes the build plan to a CMakeLists.txt file.
            /// </summary>
            private void WriteToCMake(BuildContext Context, BuildPlan Plan, BuildOptions Options)
            {
                var CMakeWriter = new CMakeWriter(Context);

                CMakeWriter.Write(Paths.Combine(Options.OutputDirectory, "CMakeLists.txt"), Plan);
                Diagnostics.Info("Generated CMakeLists.txt");
            }

        }

    }

}
