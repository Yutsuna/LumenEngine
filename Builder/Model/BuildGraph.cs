namespace LumenBuilder.Model;

/// <summary>
/// Immutable build graph containing all parsed modules.
/// </summary>
public sealed class BuildGraph
{
    public IReadOnlyDictionary<string, ModuleDescriptor> Modules { get; }

    /// <summary>
    /// Creates a new build graph.
    /// </summary>
    public BuildGraph(IReadOnlyDictionary<string, ModuleDescriptor> Modules)
    {
        this.Modules = Modules;
    }

    /// <summary>
    /// Tries to get a module by name.
    /// </summary>
    public bool TryGetModule(string Name, out ModuleDescriptor? Module)
    {
        if (Modules.TryGetValue(Name, out var Found))
        {
            Module = Found;
            return true;
        }
        Module = null;
        return false;
    }

    public int Count => Modules.Count;
}
