namespace LumenBuilder.Graph;

/// <summary>
/// Topological sort for build order computation using Kahn's algorithm.
/// Dependencies are built before dependents.
/// </summary>
public sealed class TopologicalSorter
{
    private readonly DependencyGraph Graph;

    /// <summary>
    /// Creates a new topological sorter for the given dependency graph.
    /// </summary>
    public TopologicalSorter(DependencyGraph Graph)
    {
        this.Graph = Graph;
    }

    /// <summary>
    /// Performs topological sort and returns the sorted list of modules.
    /// </summary>
    public IReadOnlyList<string> Sort()
    {
        var Nodes = Graph.GetAllNodes();
        var InDegree = InitializeInDegree(Nodes);

        ComputeInDegreeFromDependencies(Nodes, InDegree);

        var Queue = InitializeQueueWithRoots(InDegree);
        var Result = new List<string>();

        ProcessQueue(Queue, InDegree, Result);

        return Result;
    }

    /// <summary>
    /// Initializes the in-degree map with zero for all nodes.
    /// </summary>
    private Dictionary<string, int> InitializeInDegree(IReadOnlySet<string> Nodes)
    {
        var InDegree = new Dictionary<string, int>();

        foreach (string Node in Nodes)
        {
            InDegree[Node] = 0;
        }

        return InDegree;
    }

    /// <summary>
    /// InDegree = number of dependencies (not dependents).
    /// A module with deps [B, C] has in-degree = 2.
    /// </summary>
    private void ComputeInDegreeFromDependencies(
        IReadOnlySet<string> Nodes,
        Dictionary<string, int> InDegree)
    {
        foreach (string Node in Nodes)
        {
            var Dependencies = Graph.GetDependencies(Node);
            InDegree[Node] = Dependencies.Count;
        }
    }

    /// <summary>
    /// Enqueues all nodes with zero in-degree.
    /// </summary>
    private Queue<string> InitializeQueueWithRoots(Dictionary<string, int> InDegree)
    {
        var Queue = new Queue<string>();

        foreach (var Kvp in InDegree)
        {
            if (Kvp.Value == 0)
            {
                Queue.Enqueue(Kvp.Key);
            }
        }

        return Queue;
    }

    /// <summary>
    /// Processes the queue and builds the topological order.
    /// </summary>
    private void ProcessQueue(
        Queue<string> Queue,
        Dictionary<string, int> InDegree,
        List<string> Result)
    {
        while (Queue.Count > 0)
        {
            string Current = Queue.Dequeue();
            Result.Add(Current);

            /// <summary>
            /// If A depends on B, then B is a dependency of A.
            /// So when B is processed, we reduce the in-degree of A.
            /// </summary>
            var Dependents = Graph.GetDependents(Current);
            for (int DependentIndex = 0; DependentIndex < Dependents.Count; ++DependentIndex)
            {
                string Dependent = Dependents[DependentIndex];
                --InDegree[Dependent];

                if (InDegree[Dependent] == 0)
                {
                    Queue.Enqueue(Dependent);
                }
            }
        }

    }

}
