# Module: RHI (work in progress)

- **Type:** shared library (planned) — hardware-agnostic render-hardware
  interface abstraction layered over AppCore/Os, under Renderer.
- **Status:** **not built.** `Source/RHI/{Private,Public,Tests}` exist but are
  empty; absent from `ModuleNames`. Do not include its headers or depend on
  it yet.

## Intended layout (when implemented)

- `RHI` defines the backend-agnostic interface scenes render through;
  `RHIOpenGL` and `RHIVulkan` implement it as separate modules — APIs of RHI
  are `I`-prefixed pure interfaces (see `rules/coding-style.md`), backend
  objects `F`.
- Nix devshell already provides Vulkan headers/loader and validation layers
  (`VK_LAYER_PATH`) — the tooling is ready; only the modules are empty.
