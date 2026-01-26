using System.Diagnostics;
using LumenBuilder.Common;

namespace LumenBuilder.Build.Toolchain;

/// <summary>
/// Resolves the appropriate toolchain for the current platform.
/// Falls back to the Clang/GCC toolchain on Windows when MSVC is not available.
/// </summary>
public sealed class ToolchainResolver
{
    public Compiler Resolve(PlatformInfo Platform)
    {
        if (Platform.Type == PlatformType.Windows)
        {
            if (IsCommandAvailable("cl.exe"))
            {
                return new MsvcToolchain();
            }

            return new ClangToolchain();
        }

        return new ClangToolchain();
    }

    public Compiler Resolve(string ToolchainName)
    {
        return ToolchainName.ToLowerInvariant() switch
        {
            "msvc" or "cl" => new MsvcToolchain(),
            "clang++" => new ClangToolchain(),
            "g++" => new GppToolchain(),
            _ => new ClangToolchain()
        };
    }

    private static bool IsCommandAvailable(string command)
    {
        try
        {
            var psi = new ProcessStartInfo("where", command)
            {
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                UseShellExecute = false,
                CreateNoWindow = true
            };

            using var p = Process.Start(psi);

            if (p == null)
            {
                return false;
            }
            p.WaitForExit(2000);
            return p.ExitCode == 0;
        }
        catch
        {
            return false;
        }
    }
}
