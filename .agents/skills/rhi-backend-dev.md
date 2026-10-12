---
name: rhi-backend-dev
description: Workflow for working on Lumen's RHI abstraction or concrete rendering/window backends.
---

# RHI and Backend Development

Use this skill when adding a new backend, extending an abstraction, or modifying backend-specific code. Authoritative constraints: `rules/architecture.md`.

## Two independent dimensions

```text
Window / platform backend:   X11, SDL, ...        behind the Window abstraction (planned)
Rendering backend:           Vulkan, OpenGL, ...  behind the RHI abstraction (planned)
```

These are orthogonal. All combinations must be possible:

```text
SDL + Vulkan    SDL + OpenGL    X11 + Vulkan    X11 + OpenGL
```

Generic engine code must never know which combination is active, and selecting a window backend must never select (or imply) a rendering backend.

## Abstraction boundary

```text
Rendering / AppCore / engine code
        ↓  depends on
RHI / Window abstraction (generic interfaces)
        ↓  implemented by
Vulkan / OpenGL / X11 / SDL / ...
```

- Engine-level code depends on the abstraction layer; backend code implements it. Dependencies never flow upward.
- No backend-specific types, enums, handles, or calling conventions in generic public headers.

## Current repository reality

- Neither `Source/RHI/` nor `Source/Window/` exists yet. Backend work today happens inside `Core`/`AppCore` (e.g. `AppCore/Public/Generic/`); creating those modules is an architecture task (`skills/lumen-architecture.md`).

## Adding a backend

1. Define or read the generic interface (`Source/<Module>/Public/`).
2. Create a backend implementation directory (e.g. `Source/<Module>/Private/Vulkan/`, or a dedicated module — decision by `lumen-architect`).
3. Implement the interface; keep backend types out of public headers and inside backend translation units.
4. Wire dependencies through the build: register module dependencies in `Depends` **before** the dependent module in `Source/meson.build`; external SDKs (e.g. `vulkan`) need a ModuleBuilder extension plus `Nix/deps.nix` (`rules/build.md`).

## Design checks

1. Backend errors surface through the abstraction with clear status types, not leaked error enums.
2. Resource lifetimes and CPU/GPU synchronization are documented.
3. Backend-specific compile flags and headers do not leak into generic targets.

## Validation

1. Build the module containing the abstraction and the backend target (`ninja -C <builddir>` after `meson setup --reconfigure` if sources changed).
2. Build a target that uses the abstraction and confirm it does not require backend-specific compile flags.
3. Validate the backend's runtime behavior with whatever harness exists — currently none; state `Not run` with the reason rather than implying runtime coverage (`rules/validation.md`).
4. Apply `skills/code-review.md` to check for backend leakage.
