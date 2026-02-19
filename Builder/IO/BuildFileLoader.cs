using LumenBuilder.Common;
using LumenBuilder.Model;
using LumenBuilder.Parsing;

namespace LumenBuilder.IO;

/// <summary>
/// Loads and parses build files from disk.
/// </summary>
public sealed class BuildFileLoader
{
    private readonly BuildFileParser Parser;
    private readonly DiagnosticBag Diagnostics;

    /// <summary>
    /// Creates a new build file loader.
    /// </summary>
    public BuildFileLoader(BuildFileParser Parser, DiagnosticBag Diagnostics)
    {
        this.Parser = Parser;
        this.Diagnostics = Diagnostics;
    }

    /// <summary>
    /// Loads and parses a single build file.
    /// </summary>
    public ModuleDescriptor? LoadModule(string BuildFilePath)
    {
        string ModuleName = Paths.GetModuleName(BuildFilePath);
        string ModuleDir = Paths.GetDirectory(BuildFilePath);

        try
        {
            string Source = File.ReadAllText(BuildFilePath);
            return Parser.Parse(ModuleName, ModuleDir, Source.AsSpan());
        }
        catch (IOException Ex)
        {
            Diagnostics.Error($"Failed to read {BuildFilePath}: {Ex.Message}");
            return null;
        }
        catch (BuildException Ex)
        {
            Diagnostics.Error(Ex.Message, BuildFilePath, Ex.Line, Ex.Column);
            return null;
        }
    }

    /// <summary>
    /// Loads and parses multiple build files in parallel.
    /// </summary>
    public BuildGraph LoadAll(IEnumerable<string> BuildFiles)
    {
        var Modules = new Dictionary<string, ModuleDescriptor>();
        var LockObj = new object();

        Parallel.ForEach(BuildFiles, File =>
        {
            var Module = LoadModule(File);
            if (Module != null)
            {
                lock (LockObj)
                {
                    if (!Modules.TryAdd(Module.Name, Module))
                    {
                        Diagnostics.Error($"Duplicate module: {Module.Name}");
                    }
                }
            }
        });

        return new BuildGraph(Modules);
    }

}
