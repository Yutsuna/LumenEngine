# Module: RHIOpenGL

- **Type:** Shared library (`Source/RHIOpenGL/meson.build`), depends on `Core`,
  `Maths`, `RHI`. Listed in `ModuleNames` on Linux, immediately after `RHI`.
  The `gl`/`egl` pkg-config dependencies are opportunistic: GL entry points are
  resolved at runtime with `dlopen`/`eglGetProcAddress`, so a stale pkg-config
  environment does not break configuration.
- **Status:** built.
- **Role:** Linux-only OpenGL 4.6 core RHI backend. Creates a headless EGL
  surfaceless context (Mesa `llvmpipe` in CI) on one dedicated GL Executor
  thread and implements the `IRHIDevice` factories on top of it.

## Public surface (`Source/RHIOpenGL/Public/`)

- `RHIOpenGL/RHIOpenGLModule.hpp` — `CreateRHIOpenGLModule()`
  (`RHIOPENGL_EXPORT`).
- `RHIOpenGL/RHIOpenGLSurfaceBridge.hpp` — `CreateRHIOpenGLSurfaceBridge()`
  (`RHIOPENGL_EXPORT`) plus `FOpenGLNativeWindowDesc` and
  `EOpenGLNativeWindowSystem`. The factory returns `Unsupported` while
  native-display device initialization is unavailable. These are the module's
  two exported factory functions. Concrete backend classes live in `Private/`
  and are never exported; tests link them in through `GlobalTestModules`
  instead.

## Private layout (`Source/RHIOpenGL/Private/RHIOpenGL/`)

| Area | Responsibility |
| --- | --- |
| `Loader/` | Typed GL dispatch table, GL 4.6 constants, runtime symbol resolution |
| `Platform/Linux/` | EGL surfaceless context; library/display handles cached for the process |
| `Device/` | GL Executor and submission scheduler, capability/limit probe, device context, `IRHIDevice` factories |
| `Module/` | `IRHIModule`, adapter enumeration, device creation |
| `Types/` | Format, error and object-identity mapping helpers |
| `Memory/` | Heaps, upload/readback allocators, transient allocator, native defragmenter |
| `Resources/` | Buffers, textures, texture views, samplers |
| `Bindings/` | Binding layouts, binding sets, descriptor allocator |
| `Shaders/`, `Pipelines/`, `Caches/` | GLSL 460 source shaders, pipeline layouts, DSA VAO cache, graphics/compute programs, LRU pipeline cache |
| `Commands/` | Command stream and opcodes, allocator, validated command list, FBO/state caches, replay executor |
| `Synchronization/`, `Queue/` | Timelines, semaphores, query pools, submission and condition-variable waits |
| `Presentation/` | Bounded swapchain ring and `IRHISurfaceBridge` present path |

Tests live in `Source/RHIOpenGL/Tests/` (backend-local suites plus the
conformance device factory). `Tests/Main.cpp` runs the standard pass with
validation off, then the conformance pass with validation on, and exits **84**
on failure.

## Notes for agents

- **GL-thread rule:** every native GL call, including `glDelete*` and sync
  object deletion, runs on the GL Executor. Destructors retire handles through
  `FOpenGLContext::DeferRelease`, whose callbacks are drained on the executor
  before shutdown; an invoked task fails fast if that executor has stopped. The
  only exception is capability probing, which creates and destroys its own
  temporary EGL context on the calling thread before any device exists.
- **Lock DAG:** `Queue::SubmitMutex` → device registry → context
  `SchedulerMutex` → semaphore mutex → task queue → debug-sink leaf → the
  rank-7 wait-state leaf (mutex + condition variable, no lock taken under it).
  Never call `InvokeOnGLThread`/`ExecuteOnGLThread` while a ranked lock is
  held, and release `SchedulerMutex` before polling GL or signalling semaphores.
- **Header isolation:** do not include `<GL/gl.h>`, `<GL/glx.h>`, `<EGL/egl.h>`
  or `<X11/Xlib.h>` in general translation units — their macros (`None`, `Bool`,
  `Success`, `Always`, `Status`) collide with Lumen names. Keep vendor headers
  in one `Private/` translation unit and go through `Loader/` everywhere else.
- **Namespaces:** `.hpp` files use nested `namespace Lumen { namespace RHIOpenGL
  { … } }`; `.cpp` files qualify explicitly and put file-local helpers in an
  anonymous namespace; test files open `namespace Lumen { namespace { … } }`.
- **Honest capabilities:** advertise only what has been functionally verified —
  `AsyncCompute`, bindless resources, pipeline statistics, mesh/geometry/
  tessellation stages and Mailbox presentation are unsupported, and the only
  shader format is GLSL 460 source. Do not infer a capability from an extension
  string alone.
- **Dependencies:** no `AppCore`/SDL dependency; windowed surfaces come from
  `IRHISurfaceBridge`. Do not depend on this module from production renderer
  code until downstream integration is approved.
- **EGL process lifetime:** the EGL library handle and default display are
  cached for the process — repeated per-device `eglInitialize`/`eglTerminate`
  cycles exhaust Mesa's display slots in long conformance runs.
- **Backend limits at M7:** the device still creates a surfaceless EGL context,
  so `CreateRHIOpenGLSurfaceBridge` rejects coherent native-window handles with
  `Unsupported`. `FOpenGLSwapchain::Present` delegates to a caller bridge and
  has no blit into a window-visible framebuffer. Windowed OpenGL presentation
  therefore needs native-display device initialization before it can be
  claimed. Conformance covers headless draw/dispatch and Null-surface rings;
  it does not verify pixel values from the texture-to-buffer readback path.
- **Build/verification:** reconfigure all three trees when adding a `.cpp`
  (`Scripts/BuildMeson.bash`); `Build`, `Build-Release` and `Build-TSAN` must all
  compile, and `RHIOpenGLTests` must pass with TSan clean and
  `python3 Scripts/CheckClangFormat.py` clean. Mesa's per-draw driver
  allocations are covered by `lsan.supp`.
