# Module: Renderer (work in progress)

- **Type:** shared library (planned). The Meson stub declares dependencies on
  `Core`, `Maths`, `RHI`, `Scene`.
- **Status:** **not built.** `Source/Renderer/meson.build` exists, but the
  module is absent from `ModuleNames` (`Source/meson.build`) and has no
  production implementation yet. Do not include its headers or list it in
  `Depends` of other modules until it is activated.
