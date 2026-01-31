using LumenBuilder.Build.Toolchain;
using LumenBuilder.Common;
using LumenBuilder.Dependencies;
using LumenBuilder.Model;

namespace LumenBuilder.Build;

/// <summary>
/// Immutable build context containing all build configuration.
/// </summary>
public sealed record BuildContext(
    BuildGraph Graph,
    PlatformInfo Platform,
    BuildConfiguration Configuration,
    Compiler Toolchain,
    string RootDirectory,
    string OutputDirectory,
    DiagnosticBag Diagnostics,
    DependencyResolver ExternalDeps
);
