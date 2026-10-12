---
name: lumen-architecture
description: Workflow for designing or changing Lumen's module structure and dependencies.
---

# Lumen Architecture

Use this skill when adding a module, changing module boundaries, or designing a new abstraction layer.

## Inspection

1. Read `Source/meson.build` and `Source/Meson/ModuleBuilder/meson.build`.
2. Read the `meson.build` of every module involved in the change.
3. Read public headers in `Source/<Module>/Public/` to understand current surfaces.

## Design checks

1. Dependency graph must remain a DAG.
2. Backend-specific code must stay behind RHI or Window abstractions.
3. Public API changes must be documented and minimized.
4. New modules follow the existing `ModuleDefinition` template.

## Implementation

1. Add or update `Source/<Module>/meson.build`.
2. Register new modules in `Source/meson.build`.
3. Update `Depends` arrays to reflect deliberate dependencies.
4. Ensure exported symbols use `<MODULE>_EXPORT` for shared modules.

## Validation

1. Reconfigure Meson: `meson setup --reconfigure <builddir>`.
2. Build affected targets.
3. Run `lumen-reviewer` or `code-review` skill for architecture review.
