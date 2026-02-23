using System.Runtime.InteropServices;

namespace LumenBuilder.Common;

/// <summary>
/// Platform detection and configuration.
/// </summary>
public enum PlatformType
{
    Windows,
    Linux,
    MacOS
}

/// <summary>
/// Build configuration.
/// </summary>
public enum BuildConfiguration
{
    Debug,
    Development,
    Release
}

/// <summary>
/// CPU architecture.
/// </summary>
public enum Architecture
{
    X64,
    Arm64
}

/// <summary>
/// Immutable platform context.
/// </summary>
public sealed record PlatformInfo(
    PlatformType Type,
    Architecture Architecture,
    string ObjectExtension,
    string StaticLibExtension,
    string SharedLibExtension,
    string ExecutableExtension
)
{

    /// <summary>
    /// Detects the current platform and architecture.
    /// </summary>
    public static PlatformInfo Detect()
    {
        PlatformType Plat;
        if (RuntimeInformation.IsOSPlatform(OSPlatform.Windows))
            Plat = PlatformType.Windows;
        else if (RuntimeInformation.IsOSPlatform(OSPlatform.OSX))
            Plat = PlatformType.MacOS;
        else
            Plat = PlatformType.Linux;

        Architecture Arch = RuntimeInformation.OSArchitecture switch
        {
            System.Runtime.InteropServices.Architecture.Arm64 => Architecture.Arm64,
            _ => Architecture.X64
        };

        return Plat switch
        {
            PlatformType.Windows => new PlatformInfo(
                Plat, Arch, ".obj", ".lib", ".dll", ".exe"),
            PlatformType.MacOS => new PlatformInfo(
                Plat, Arch, ".o", ".a", ".dylib", ""),
            _ => new PlatformInfo(
                Plat, Arch, ".o", ".a", ".so", "")
        };
    }

}
