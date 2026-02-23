using LumenBuilder.Model;

namespace LumenBuilder.Graph;

/// <summary>
/// Dependency graph representation for build order computation.
/// </summary>
public sealed class DependencyGraph
{
    private readonly Dictionary<string, List<string>> Adjacency = new();
    private readonly Dictionary<string, List<string>> ReverseAdjacency = new();
    private readonly HashSet<string> Nodes = new();

    /// <summary>
    /// Adds a module and its dependencies to the graph.
    /// </summary>
    public void AddModule(ModuleDescriptor Module)
    {
        string Name = Module.Name;
        Nodes.Add(Name);

        if (!Adjacency.ContainsKey(Name))
        {
            Adjacency[Name] = new List<string>();
        }

        for (int DepsIndex = 0; DepsIndex < Module.Dependencies.Count; ++DepsIndex)
        {
            string Dep = Module.Dependencies[DepsIndex];

            Nodes.Add(Dep);
            Adjacency[Name].Add(Dep);

            if (!ReverseAdjacency.ContainsKey(Dep))
            {
                ReverseAdjacency[Dep] = new List<string>();
            }
            ReverseAdjacency[Dep].Add(Name);
        }

    }

    /// <summary>
    /// Gets all nodes in the graph.
    /// </summary>
    public IReadOnlySet<string> GetAllNodes() => Nodes;

    /// <summary>
    /// Gets the dependencies of a module.
    /// </summary>
    public IReadOnlyList<string> GetDependencies(string Module)
    {
        if (Adjacency.TryGetValue(Module, out var Deps))
        {
            return Deps;
        }
        return Array.Empty<string>();
    }

    /// <summary>
    /// Gets the dependents of a module.
    /// </summary>
    public IReadOnlyList<string> GetDependents(string Module)
    {
        if (ReverseAdjacency.TryGetValue(Module, out var Deps))
        {
            return Deps;
        }
        return Array.Empty<string>();
    }

    /// <summary>
    /// Gets the number of nodes in the graph.
    /// </summary>
    public int NodeCount => Nodes.Count;
}
