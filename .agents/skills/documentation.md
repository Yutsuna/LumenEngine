---
name: documentation
description: Workflow for writing comments and developer documentation in Lumen.
---

# Documentation

Use this skill when writing file headers, class comments, function comments, or developer docs.

## Mandatory file header

Every new C/C++ file must begin with:

```cpp
/// @file <Module>/<Path>/<FileName>
/// @brief <Concise description of responsibility>
```

## What to document

- **Why**, not merely **what**.
- Ownership and lifetime of returned pointers, references, or resources.
- Thread-safety and synchronization requirements.
- Performance characteristics or complexity guarantees.
- Invariants and preconditions.
- Backend constraints or assumptions.

## What not to document

- Trivial getters whose name already says everything.
- Obvious control flow inside implementations.
- Out-of-date TODOs; either fix them or track them externally.

## Style

- Use professional technical English.
- Use Doxygen-style `///` comments for public APIs.
- Keep comments adjacent to the code they describe.

## Validation

1. Proofread for spelling and clarity.
2. Ensure comments match the code after edits.
3. Run the code-review skill to catch misleading or stale docs.

## What must not be done

- Do not document implementation details that should be abstracted away.
- Do not use non-English identifiers, comments, or documentation.
- Do not claim performance improvements or architectural decisions without evidence.
- Do not introduce placeholder project names (TrafficEngine, Volt, EngineName, etc.).
