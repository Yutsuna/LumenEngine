# Module: Renderer (work in progress)

- **Type:** shared library (planned), dependencies expected: `Core`,
  `Maths`, `RHI` (not yet defined in its meson.build — none exists).
- **Status:** **not built.** `Source/Renderer/{Private,Public,Tests}` exist
  but are empty, and the module is absent from `ModuleNames`
  (`Source/meson.build`). Do not include its headers or list it in `Depends`
  of other modules yet.
