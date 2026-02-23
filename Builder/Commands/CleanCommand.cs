using LumenBuilder.Common;

namespace LumenBuilder.Commands;

/// <summary>
/// Clean command implementation.
/// </summary>
public sealed class CleanCommand
{
    private readonly DiagnosticBag Diagnostics;

    /// <summary>
    /// Creates a new clean command.
    /// </summary>
    public CleanCommand(DiagnosticBag Diagnostics)
    {
        this.Diagnostics = Diagnostics;
    }

    /// <summary>
    /// Executes the clean command for the given root directory.
    /// </summary>
    public int Execute(string RootDirectory)
    {
        string IntermediateDir = Paths.Combine(RootDirectory, Paths.IntermediateDir);
        string BinariesDir = Paths.Combine(RootDirectory, Paths.BinariesDir);

        int Deleted = 0;

        if (Directory.Exists(IntermediateDir))
        {
            try
            {
                Directory.Delete(IntermediateDir, true);
                Diagnostics.Info($"Deleted {IntermediateDir}");
                ++Deleted;
            }
            catch (IOException Ex)
            {
                Diagnostics.Error($"Failed to delete {IntermediateDir}: {Ex.Message}");
            }
        }

        if (Directory.Exists(BinariesDir))
        {
            try
            {
                Directory.Delete(BinariesDir, true);
                Diagnostics.Info($"Deleted {BinariesDir}");
                ++Deleted;
            }
            catch (IOException Ex)
            {
                Diagnostics.Error($"Failed to delete {BinariesDir}: {Ex.Message}");
            }
        }

        if (Deleted == 0)
        {
            Diagnostics.Info("Nothing to clean");
        }

        return Diagnostics.HasErrors ? 1 : 0;
    }

}
