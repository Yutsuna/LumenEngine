using LumenBuilder.Common;

namespace LumenBuilder.Build.Toolchain;

/// <summary>
/// Resolves the appropriate toolchain for the current platform.
/// </summary>
public sealed class ToolchainResolver
{

    public Compiler Resolve(string? ToolchainName, PlatformInfo Platform)
    {
        if (string.IsNullOrEmpty(ToolchainName))
        {
            return Platform.Type switch
            {
                PlatformType.Linux => UnixToolchain.Gpp(),
                _ => UnixToolchain.Clang()
            };
        }

        return ToolchainName.ToLowerInvariant() switch
        {
            "clang++" => UnixToolchain.Clang(),
            "g++" => UnixToolchain.Gpp(),
            "msvc" => throw new BuildException("MSVC is not supported. Please use Clang++ or G++."),
            _ => UnixToolchain.Clang()
        };
    }

}
