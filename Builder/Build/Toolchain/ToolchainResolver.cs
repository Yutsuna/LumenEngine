using LumenBuilder.Common;

namespace LumenBuilder.Build.Toolchain;

/// <summary>
/// Resolves the appropriate toolchain for the current platform.
/// Falls back to the Clang toolchain if none is found.
/// </summary>
public sealed class ToolchainResolver
{

    public Compiler Resolve(string? ToolchainName, PlatformInfo Platform)
    {
        if (string.IsNullOrEmpty(ToolchainName))
        {
            return Platform.Type switch
            {
                PlatformType.Windows => new ClangToolchain(),
                PlatformType.Linux => new GppToolchain(),
                PlatformType.MacOS => new ClangToolchain(),
                _ => new ClangToolchain()
            };
        }
        
        return ToolchainName.ToLowerInvariant() switch
        {
            "clang++" => new ClangToolchain(),
            "g++" => new GppToolchain(),
            "msvc" => throw new BuildException("MSVC is not supported. Please use Clang++ or G++."),
            _ => new ClangToolchain()
        };
    }

}
