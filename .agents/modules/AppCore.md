# Module: AppCore

- **Type:** Shared library (`Source/AppCore/meson.build`), depends on `Core`,
  `Maths`; external dependency `sdl3` (pkg-config, Linux only).
- **Status:** built.
- **Role:** application/platform layer — windows, application lifecycle,
  input, window events (SDL3-backed on Linux, Win32-backed on Windows). The
  Vulkan/GL backends and Renderer plug in on top of this later.

## Public surface (`Source/AppCore/Public/`)

- `Generic/` — platform-independent API: `GenericApplication`,
  `GenericApplicationInput`, `GenericApplicationMessageHandler`,
  `GenericWindow`, `GenericWindowDescription`. Generic `_DESCRIPTION`-style
  virtuals are placeholders to be implemented per-platform — marked with
  `/** NOTE: To be implemented by children */` (do not "fix" those as errors).
- `Linux/` — Linux SDL3 implementation of the generic API.
- `Windows/` — Win32 implementation of the generic API
  (`FWindowsApplication`, `FWindowsWindow`). No SDL3 dependency on Windows;
  native handles are exposed as `ENativeWindowSystem::Win32` (HWND in
  `NativeWindow`, HINSTANCE in `NativeDisplay`).

## Notes for agents

- Window/application types are `F`-prefixed framework objects, the scene
  graph stays out of this module (that's `Scene`, wrapped by `Launch`).
- Platform differences stay behind the `Generic/` interfaces; call sites
  must not include platform-specific headers directly.
- SDL3 is linked only in this module (`ExternalDeps` in its meson.build);
  other modules must go through AppCore APIs.
