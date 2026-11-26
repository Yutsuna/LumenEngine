# Module: RHIOpenGL (M0 + M1 + M2 + M3 built)

- **Type:** shared library; implements the `RHI` interfaces with a Linux-only
  OpenGL 4.6 core backend (headless EGL surfaceless, Mesa `llvmpipe` in CI).
- **Status:** **built.** Registered in `ModuleNames` on Linux after `RHI`;
  `Depends = Core, Maths, RHI` (plus opportunistic `gl`/`egl` pkg-config deps).
  - **M0:** public `CreateRHIOpenGLModule()`, typed GL dispatch, EGL 4.6
    headless context, one GL Executor thread, capability/limit probe,
    `FRHIDeviceBase` device, logical queues, identity helpers.
  - **M1:** persistent GL-buffer heaps; dedicated and placed buffers with
    coherent maps; immutable textures, texture views and sampler objects with
    parent/heap retention; upload/readback rings, buffer pool, transient
    interval coloring, defrag planning; a minimal command allocator/list that
    records validated buffer copies.
  - **M2:** GLSL 460 source shaders; binding layouts/sets/allocator; pipeline
    layouts, DSA VAO cache, graphics/compute program linking, versioned LRU
    pipeline cache; full validated command recording plus a single-threaded GL
    replay executor (FBO/state caches, barriers, draws/dispatch, queries);
    occlusion/timestamp query pools.
  - **M3:** CPU timeline/binary semaphores; async queue submission with a
    ready/blocked scheduler, `GLsync` fence ring polled with
    `glClientWaitSync(..., 0)`, backoff wakeups and deferred release queue;
    device-loss polling plus KHR_debug bridging through the leaf debug-sink
    lock; bounded swapchain ring with windowed `IRHISurfaceBridge` present and
    Null/offscreen support (Mailbox rejected); inline-constant UBO ring upload,
    native blend constants/depth bias/stencil reference/debug regions, and
    PBO texture-to-buffer readback. The conformance factory now requests all
    queue families, creates swapchains and injects out-of-date state.
- **Verification:** `RHIOpenGLTests` runs backend-local tests plus the full
  portable conformance matrix with the validation on/off matrix and exit 84.
  With M3 enabled the runner reports **60 standard tests** and **49 conformance
  tests** per pass (109 checks, exit 0) on all three trees.
  `Build`, `Build-Release` (-O3) and `Build-TSAN` all compile and pass with
  exit 0; Mesa's per-draw driver allocations are covered by `lsan.supp`.
- **GL-thread rule:** every native GL call, including `glDelete*` and sync
  object deletion, happens on the GL Executor; `InvokeOnGLThread` uses a shared
  latch so the worker still owns it after waking the caller (TSan clean).
  Native debug callbacks only copy text under the leaf debug-sink lock.
- **Lock DAG:** `Queue::SubmitMutex` → device registry → context
  `SchedulerMutex` → semaphore mutex → context task queue → debug sink leaf.
  `InvokeOnGLThread` is never called while any ranked lock is held; scheduler
  locks are released before polling GL or signalling semaphores.
- **Dependency rule:** no `AppCore`/SDL dependency; windowed surfaces come from
  `IRHISurfaceBridge`. Do not depend on this module from production renderer
  code until downstream integration is approved.
- **EGL process lifetime:** the EGL library handle and default display are
  cached for the process because repeated per-device `eglInitialize`/
  `eglTerminate` cycles exhaust Mesa's display slots in long conformance runs.
