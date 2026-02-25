using System.Runtime.InteropServices;

namespace LumenBuilder.Build.Toolchain;

/// <summary>
/// Detects the fastest available linker by probing PATH.
/// Priority order: mold > gold > ld.
/// </summary>
public static class LinkerDetector
{
    private static readonly string[] Candidates = ["mold", "gold", "ld"];

    /// <summary>
    /// Returns the name of the fastest available linker found in PATH, or null if none.
    /// </summary>
    public static string? Detect()
    {
        string PathVar = Environment.GetEnvironmentVariable("PATH") ?? "";
        string[] PathDirs = PathVar.Split(Path.PathSeparator, StringSplitOptions.RemoveEmptyEntries);

        // Windows executables carry extensions; Unix binaries typically don't.
        string[] Extensions = RuntimeInformation.IsOSPlatform(OSPlatform.Windows)
            ? [".exe", ""]
            : [""];

        foreach (string Candidate in Candidates)
        {
            foreach (string Dir in PathDirs)
            {
                foreach (string Ext in Extensions)
                {
                    if (File.Exists(Path.Combine(Dir, Candidate + Ext)))
                        return Candidate;
                }
            }
        }

        return null;
    }
}
