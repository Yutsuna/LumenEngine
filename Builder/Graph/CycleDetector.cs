namespace LumenBuilder.Graph;

/// <summary>
/// Detects cycles in the dependency graph.
/// </summary>
public sealed class CycleDetector
{
    private readonly DependencyGraph Graph;

    /// <summary>
    /// Creates a new cycle detector for the given dependency graph.
    /// </summary>
    public CycleDetector(DependencyGraph Graph)
    {
        this.Graph = Graph;
    }

    /// <summary>
    /// Checks if the graph has cycles.
    /// If a cycle is found, the path of the cycle is returned.
    /// </summary>
    public bool HasCycle(out IReadOnlyList<string> CyclePath)
    {
        var Visited = new HashSet<string>();
        var RecStack = new HashSet<string>();
        var Path = new List<string>();

        foreach (string Node in Graph.GetAllNodes())
        {
            if (DetectCycleDfs(Node, Visited, RecStack, Path))
            {
                CyclePath = Path;
                return true;
            }
        }

        CyclePath = Array.Empty<string>();
        return false;
    }

    /// <summary>
    /// Depth-first search to detect cycles.
    /// </summary>
    private bool DetectCycleDfs(
        string Node,
        HashSet<string> Visited,
        HashSet<string> RecStack,
        List<string> Path)
    {
        if (RecStack.Contains(Node))
        {
            int CycleStart = Path.IndexOf(Node);

            if (CycleStart >= 0)
            {
                Path = Path.GetRange(CycleStart, Path.Count - CycleStart);
                Path.Add(Node);
            }
            return true;
        }

        if (Visited.Contains(Node))
        {
            return false;
        }

        Visited.Add(Node);
        RecStack.Add(Node);
        Path.Add(Node);

        var Deps = Graph.GetDependencies(Node);
        for (int DepsIndex = 0; DepsIndex < Deps.Count; ++DepsIndex)
        {
            if (DetectCycleDfs(Deps[DepsIndex], Visited, RecStack, Path))
            {
                return true;
            }
        }

        Path.RemoveAt(Path.Count - 1);
        RecStack.Remove(Node);
        return false;
    }

}
