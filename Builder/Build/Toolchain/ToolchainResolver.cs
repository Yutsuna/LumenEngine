using LumenBuilder.Common;

namespace LumenBuilder.Build.Toolchain;

/// <summary>
/// Resolves the appropriate toolchain for the current platform.
/// </summary>
public sealed class ToolchainResolver
{
    public Compiler Resolve(PlatformInfo Platform)
    {
        return Platform.Type switch
        {
            PlatformType.Windows => new MsvcToolchain(),
            _ => new ClangToolchain()
        };
    }

    public Compiler Resolve(string ToolchainName)
    {
        return ToolchainName.ToLowerInvariant() switch
        {
            "msvc" or "cl" => new MsvcToolchain(),
            "clang" or "gcc" or "g++" => new ClangToolchain(),
            _ => new ClangToolchain()
        };
    }
}
