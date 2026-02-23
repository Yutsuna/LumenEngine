using LumenBuilder.Common;

namespace LumenBuilder.Model;

/// <summary>
/// Platform-specific dependencies for a module.
/// </summary>
public sealed record PlatformDependencies(
    IReadOnlyList<string> Linux,
    IReadOnlyList<string> Windows,
    IReadOnlyList<string> MacOS
)
{
    public IReadOnlyList<string> ForPlatform(PlatformType Type) => Type switch
    {
        PlatformType.Linux => Linux,
        PlatformType.Windows => Windows,
        PlatformType.MacOS => MacOS,
        _ => Array.Empty<string>()
    };
}

/// <summary>
/// Platform-specific system libraries for linking.
/// </summary>
public sealed record PlatformLibraries(
    IReadOnlyList<string> Linux,
    IReadOnlyList<string> Windows,
    IReadOnlyList<string> MacOS
)
{
    public IReadOnlyList<string> ForPlatform(PlatformType Type) => Type switch
    {
        PlatformType.Linux => Linux,
        PlatformType.Windows => Windows,
        PlatformType.MacOS => MacOS,
        _ => Array.Empty<string>()
    };
}

/// <summary>
/// Immutable module descriptor parsed from a .build file.
/// </summary>
public sealed record ModuleDescriptor(
    string Name,
    string Directory,
    ModuleType Type,
    IReadOnlyList<string> Sources,
    IReadOnlyList<string> PublicIncludes,
    IReadOnlyList<string> PrivateIncludes,
    IReadOnlyList<string> Defines,
    IReadOnlyList<string> Dependencies,
    PlatformDependencies PlatformDeps,
    PlatformLibraries PlatformLibs
);
