# Full Architectural & Implementation Plan: `RHIOpenGL` Backend

This document specifies the complete architecture, subsystem contracts, OpenGL 4.6/4.5 mappings, concurrency model, error handling, conformance integration, and verification plan for `Source/RHIOpenGL/`, strictly conforming to `.agents/` rules (`coding-style.md`, `core-first.md`, `build.md`, `testing.md`, `modules/RHI.md`, `modules/Core.md`) and `Source/RHI/Public/`.

---

## 1. Strategic Architectural Decisions

### 1.1 Target Profile, Loading & Zero Macro Pollution
- **Target API**: **OpenGL 4.6 Core Profile** with graceful probe/fallback to **OpenGL 4.5 Core** (`GL_ARB_direct_state_access`, `GL_ARB_buffer_storage`, `GL_ARB_clip_control`, `GL_ARB_texture_storage`, `GL_ARB_texture_view`, `GL_ARB_separate_shader_objects`, `GL_ARB_multi_draw_indirect`, `GL_ARB_compute_shader`). No legacy/compatibility-profile entry points are ever called.
- **Self-Contained Khronos ABI Declarations (`Private/RHIOpenGL/Loader/GLSpec.hpp`)**:
  - Do **not** include `<GL/gl.h>`, `<GL/glx.h>`, `<EGL/egl.h>`, or `<X11/Xlib.h>` across general translation units. X11/GLX/EGL headers leak preprocessor macros (`None`, `Bool`, `Success`, `Always`, `Status`) that collide directly with `Lumen::Bool`, `ERHIResourceState::None`, and `ERHICompareFunction::Always`.
  - Define exact Khronos-ABI OpenGL 4.6 / EGL / WGL / GLX types and enum constants using Lumen typedefs (`UInt32`, `Int32`, `UInt64`, `Float32`, `AnsiChar`, `IPtr`, `UPtr`) inside `Lumen::RHIOpenGL`, and isolate OS dynamic library calls (`dlopen`/`dlsym` on `LUMEN_PLATFORM_LINUX`/`MACOS`, `LoadLibraryW`/`GetProcAddress` on `LUMEN_PLATFORM_WINDOWS`) strictly inside `Private/RHIOpenGL/Platform/`.
  - **Benefit**: Compiles warning-free with `-Werror -Wconversion -Wsign-conversion -Wold-style-cast -Wshadow -Wdouble-promotion`, avoids hard link-time failure if a target environment lacks `libGL.so` in `buildInputs`, and satisfies `RHIOpenGL`'s module dependency constraint (`Core`, `RHI`, `Maths` only — **no** `AppCore`, **no** `SDL3`).

### 1.2 Threading & Context Execution Model: Single Dedicated GL Executor Thread
OpenGL contexts can only be current on **one thread at a time**, and key objects (VAOs, FBOs, query objects) are **not shared** across OpenGL contexts. Meanwhile, `IRHIDevice` factories, `IRHIBuffer::Map`/`Unmap`, `IRHIQueue::Submit`, `IRHISemaphore`, `IRHIQueryPool::ReadResults`, and `TSharedRef` destruction can occur concurrently from any CPU thread.

**Solution — Dedicated GL Executor + Dependency Scheduler (`FOpenGLContext`):**
1. **`FOpenGLContext` owns one `Lumen::FThread` ("GL Executor")** backed by `FStopToken`, `HAL::FPlatformMutex`, `FConditionVariable`, and a non-blocking/ready task queue (`TQueue`) plus a GPU submission scheduler.
2. **Only the GL Executor thread ever makes an OpenGL context current or calls OpenGL functions.**
   - Eliminates `wglMakeCurrent`/`eglMakeCurrent` multi-thread races, cross-context VAO/FBO/Query non-sharing bugs, and TSan data races.
   - `TSharedRef` destruction of any `FOpenGL*` object from an arbitrary worker thread enqueues native `glDelete*` cleanup onto `FOpenGLContext` (`FRHIDeferredReleaseQueue` / internal deletion queue), never calling OpenGL from a context-less thread.
3. **Synchronous vs Asynchronous Operations**:
   - **Synchronous GL-thread round-trip (`InvokeOnGLThread`)**: Resource/shader/pipeline/query creation and `ReadResults` (when GL read is required) post a task and wait on a lightweight `FConditionVariable` completion latch (guarded by an assertion/check that `FThread::GetCurrentId() != ExecutorThreadId` to prevent self-deadlock).
   - **Zero-round-trip CPU operations**: Persistently-mapped `Upload`/`Readback` buffers (`glNamedBufferStorage` + `GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT`), `IRHIBindingSet::Write`, `IRHICommandList::*` recording into `FRHICommandStream`, `IRHICommandAllocator::Reset`, `IRHISemaphore::GetValue`/`Signal`, `IRHIQueue::GetCompletedValue`, and `FRHICommandList` validation all execute locally on caller threads without touching the GL Executor.
   - **Asynchronous Submission + Non-Blocking Fence Polling**: `IRHIQueue::Submit` pushes an `FOpenGLSubmission` to the scheduler and returns immediately once queued. The GL Executor dispatches ready submissions (whose wait semaphores are satisfied), inserts `glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0)` + `glFlush()`, and polls active `GLsync` fences with `0` timeout (`glClientWaitSync(..., GL_SYNC_FLUSH_COMMANDS_BIT, 0)`) in a non-blocking in-flight ring so **other queues and submissions never deadlock waiting behind a blocked semaphore**.

### 1.3 Acyclic Ownership DAG
`RHI.md` mandates that `FRHIDeviceBase` tracks live children via non-owning `IRHIObject*` + destruction callbacks so `Device <-> Child` cycles never form. Because `FOpenGLDevice` also retains `TSharedRef<FOpenGLQueue>` instances for `GetQueue()`, a naive `FOpenGLQueue -> TSharedRef<FOpenGLDevice>` pointer would create a leak cycle.

**Strict Acyclic Graph:**
```
FOpenGLDevice ──(TSharedRef)──▶ FOpenGLContext ◀──(TSharedRef)── FOpenGLQueue
     │                                 ▲                         (all FOpenGL* resources,
     └──(TSharedRef)──▶ FOpenGLQueue   │                          pipelines, pools, semaphores,
                                       └────────────────────────── swapchains, command lists)
```
- `FOpenGLContext` holds **zero** strong references to `FOpenGLDevice`, `FOpenGLQueue`, or any child RHI object.
- If the user drops `TSharedRef<IRHIDevice>` while still holding `TSharedRef<IRHIBuffer>`, `FOpenGLDevice::~FOpenGLDevice()` drains active queues, detaches `DebugSink` cleanly, transitions status to `Closed`, and returns; `FOpenGLContext` remains alive until the final child object releases its reference and deletes its GL handle on the GL thread.

### 1.4 Strict Lock Ordering (TSan-Safe DAG)
Mutex acquisition must strictly obey this total order (never acquire an earlier-ranked mutex while holding a later-ranked one):
1. `FOpenGLQueue::SubmitMutex` (per-queue submission serialization)
2. `FOpenGLDevice` / `FRHIDeviceBase` registry mutex (never held across GL thread wait)
3. `FOpenGLContext::SchedulerMutex` (submission queue, ready/blocked lists, active fences)
4. `FOpenGLSemaphore::Mutex` (value + condition variable)
5. `FOpenGLContext::TaskQueueMutex` (GL Executor wakeup/task queue)
6. `FOpenGLContext::DebugSinkMutex` (leaf lock protecting `IRHIDebugSink*` pointer + deduplication set — `glDebugMessageCallback` only ever acquires this leaf lock)

*Crucial rule*: Release `SchedulerMutex` **before** polling GL or signaling `FOpenGLSemaphore` so `SchedulerMutex` and `Semaphore::Mutex` never invert.

---

## 2. Module Layout, Build & Export Rules

### 2.1 File Hierarchy
```text
Source/
├── meson.build                                     (add 'RHIOpenGL' after 'Core', 'Maths', 'RHI')
└── RHIOpenGL/
    ├── meson.build                                 (SharedLibrary; Depends: Core, Maths, RHI; ExtraTestSources: RHIConformanceSources)
    ├── Public/
    │   └── RHIOpenGL/
    │       └── RHIOpenGLModule.hpp                 (RHIOPENGL_EXPORT CreateRHIOpenGLModule())
    ├── Private/
    │   └── RHIOpenGL/
    │       ├── Loader/
    │       │   ├── GLSpec.hpp                      (Clean Khronos OpenGL 4.6 / EGL / WGL / GLX ABI constants & typedefs)
    │       │   ├── GLDispatchTable.hpp             (Typed GL function pointers, capability extension flags)
    │       │   └── GLLoader.hpp / .cpp             (Dynamic resolution via platform GetProcAddress / IRHISurfaceBridge)
    │       ├── Platform/
    │       │   ├── IOpenGLPlatformContext.hpp      (Headless context create/destroy/makeCurrent/getProcAddress abstraction)
    │       │   ├── Linux/
    │       │   │   ├── EGLPlatformContext.hpp / .cpp  (Primary: EGL_PLATFORM_SURFACELESS_MESA + EGL_DEFAULT_DISPLAY + Pbuffer)
    │       │   │   └── GLXPlatformContext.hpp / .cpp  (Fallback when X11 DISPLAY exists without surfaceless EGL)
    │       │   ├── Windows/
    │       │   │   └── WGLPlatformContext.hpp / .cpp  (Hidden message-only HWND + WGL_ARB_create_context 4.6 Core)
    │       │   └── MacOS/
    │       │       └── CGLPlatformContext.hpp / .cpp  (Optional stub/CGL 4.1 fallback -> fails capability gate cleanly)
    │       ├── Types/
    │       │   ├── OpenGLFormatMapping.hpp / .cpp  (ERHIFormat <-> GL internal, external, type, vertex attrib, aspects)
    │       │   └── OpenGLErrorTranslation.hpp / .cpp (GL error / reset status -> FRHIError + DebugSink emission)
    │       ├── Device/
    │       │   ├── OpenGLContext.hpp / .cpp        (GL Executor FThread, TaskQueue, Scheduler, GLsync ring, DeferredDelete, DebugSink bridge)
    │       │   ├── OpenGLCapabilityProbe.hpp / .cpp (Populate FRHICapabilities, FRHILimits, ERHIAdapterType from real GL queries)
    │       │   └── OpenGLDevice.hpp / .cpp         (Derives FRHIDeviceBase; all IRHIDevice factories + status/limits/capability gating)
    │       ├── Module/
    │       │   └── OpenGLModule.hpp / .cpp         (Implements IRHIModule; version 1.1.0; EnumerateAdapters; CreateDevice)
    │       ├── Memory/
    │       │   ├── OpenGLHeap.hpp / .cpp           (IRHIHeap backed by GL buffer + persistent map; requirements validation)
    │       │   ├── OpenGLBufferPool.hpp / .cpp     (IRHIBufferPool with timeline pending queue + compatible reuse)
    │       │   ├── OpenGLUploadAllocator.hpp / .cpp(IRHIUploadAllocator composing FRHIRingAllocator + Upload buffer)
    │       │   ├── OpenGLReadbackAllocator.hpp/.cpp(IRHIReadbackAllocator composing FRHIRingAllocator + Readback buffer)
    │       │   ├── OpenGLTransientResourceAllocator.hpp/.cpp (Interval-coloring aliasing placement inside reusable heaps)
    │       │   └── OpenGLDefragmenter.hpp / .cpp   (IRHIDefragmenter BuildPlan + GL buffer copy Execute + observer notify)
    │       ├── Resources/
    │       │   ├── OpenGLResourceTraits.hpp        (Common helper for OpenGL device ownership check & GL handle/state)
    │       │   ├── OpenGLBuffer.hpp / .cpp         (IRHIBuffer; dedicated & placed; persistent coherent map + fallback)
    │       │   ├── OpenGLTexture.hpp / .cpp        (IRHITexture; immutable glTextureStorage*; deterministic byte requirements)
    │       │   ├── OpenGLTextureView.hpp / .cpp    (IRHITextureView; glTextureView; retains parent IRHITexture)
    │       │   └── OpenGLSampler.hpp / .cpp        (IRHISampler; glCreateSamplers DSA + anisotropy/compare/LOD)
    │       ├── Bindings/
    │       │   ├── OpenGLBindingLayout.hpp / .cpp  (IRHIBindingLayout; hash, bindless & limit checks)
    │       │   ├── OpenGLBindingSet.hpp / .cpp     (IRHIBindingSet; atomic validated snapshot replacement + dirty tracking)
    │       │   └── OpenGLDescriptorAllocator.hpp/.cpp (FRHIDescriptorAllocatorBase + IRHIDescriptorPoolBackend)
    │       ├── Shaders/
    │       │   ├── OpenGLShader.hpp / .cpp         (IRHIShader; GLSL Source & SPIR-V glShaderBinary/glSpecializeShader)
    │       │   └── OpenGLShaderBinder.hpp / .cpp   (Links reflection / GL uniform & block bindings to pipeline layout flat units)
    │       ├── Pipelines/
    │       │   ├── OpenGLPipelineLayout.hpp / .cpp (IRHIPipelineLayout; deterministic flat unit assignment per resource class)
    │       │   ├── OpenGLVAOCache.hpp / .cpp       (GL-thread cache of VertexLayoutHash -> GLuint VAO via glVertexArrayAttribFormat)
    │       │   ├── OpenGLGraphicsPipeline.hpp/.cpp (IRHIPipeline; Graphics state + linked program + compatibility key)
    │       │   └── OpenGLComputePipeline.hpp / .cpp(IRHIPipeline; Compute linked program + compatibility key)
    │       ├── Caches/
    │       │   └── OpenGLPipelineCache.hpp / .cpp  (IRHIPipelineCache; bounded LRU + versioned checksummed binary serialization)
    │       ├── Commands/
    │       │   ├── OpenGLCommandStreamOpcodes.hpp  (Trivially copyable command structs & opcode enum)
    │       │   ├── OpenGLCommandAllocator.hpp/.cpp (Derives FRHICommandAllocatorBase)
    │       │   ├── OpenGLCommandList.hpp / .cpp    (IRHICommandList; full state machine, queue filtering, validation, FRHICommandStream)
    │       │   ├── OpenGLFramebufferCache.hpp/.cpp (GL-thread FBO cache keyed by attachment view handles + formats)
    │       │   ├── OpenGLStateCache.hpp / .cpp     (Redundant-state elimination + dynamic state + inline constant ring UBO)
    │       │   └── OpenGLCommandExecutor.hpp / .cpp(Decodes FRHICommandStream into DSA GL calls, barriers, copies, draws, queries)
    │       ├── Synchronization/
    │       │   └── OpenGLSemaphore.hpp / .cpp      (IRHISemaphore; atomic monotonic Timeline & 0/1 consume Binary + FConditionVariable)
    │       ├── Queue/
    │       │   └── OpenGLQueue.hpp / .cpp          (IRHIQueue; serialized Submit, WaitIdle, timed Wait, GetCompletedValue)
    │       ├── Presentation/
    │       │   └── OpenGLSwapchain.hpp / .cpp      (IRHISwapchain; Window via IRHISurfaceBridge + Null offscreen; safe Resize)
    │       └── Queries/
    │           └── OpenGLQueryPool.hpp / .cpp      (IRHIQueryPool; Occlusion, Timestamp, PipelineStatistics + timeline ReadResults)
    └── Tests/
        ├── Main.cpp                                (Two-pass conformance runner: validation off, then *Conformance* validation on)
        ├── Conformance/
        │   └── OpenGLConformanceDeviceFactory.hpp / .cpp (Implements IRHIConformanceDeviceFactory)
        └── Functional/
            ├── Loader/GLLoaderTests.cpp
            ├── Types/OpenGLFormatMappingTests.cpp
            ├── Module/OpenGLModuleTests.cpp
            ├── Memory/OpenGLMemoryTests.cpp
            ├── Commands/OpenGLCommandValidationTests.cpp
            ├── Pipelines/OpenGLPipelineCacheTests.cpp
            ├── Synchronization/OpenGLQueueSchedulerTests.cpp
            └── Presentation/OpenGLSwapchainBridgeTests.cpp
```

### 2.2 Coding Standard & Export Rules Applied Everywhere
- **Exports**: Only `Public/RHIOpenGL/RHIOpenGLModule.hpp` exports `CreateRHIOpenGLModule()` with `RHIOPENGL_EXPORT` (`#include <RHIOpenGLExport.hpp>`). Concrete classes in `Private/` are **never** exported; `RHIOpenGLTests` links objects statically via `GlobalTestModules` (`rules/testing.md`).
- **Namespaces**:
  - `.hpp` files use nested `namespace Lumen { namespace RHIOpenGL { ... } }` (never `namespace Lumen::RHIOpenGL { }`).
  - `.cpp` and `.inl` files **never** open `namespace Lumen { }`; qualify explicitly with `Lumen::RHIOpenGL::`, `Lumen::RHI::`, `Lumen::`. File-local helpers live in `namespace { ... }` (never `static` free functions).
  - `Tests/**/*.cpp` open `namespace Lumen { }` and put helpers in `namespace { ... }`.
- **No Exceptions / Monadic Core**: Every function/ctor/dtor/override is `noexcept`. Zero `try`/`catch`/`throw` outside `LUMEN_CHECK*` macros in `Tests/`. Failure uses `TExpected`/`FRHIExpected`/`MakeUnexpected` + `LUMEN_EXPECT`/`LUMEN_TRY_EXPECT_ERROR`; absence uses `TOptional` + `LUMEN_OPTIONAL`/`LUMEN_TRY_OPTIONAL_ERROR`.
- **Core-First**: Strictly use `FThread`, `FStopToken`, `HAL::FPlatformMutex`, `HAL::FPlatformRecursiveSharedMutex`, `HAL::TLockGuard`/`TUniqueLock`/`TSharedLock`, `FConditionVariable`, `HAL::FPlatformTime`, `TVector`, `TArray`, `TMap`, `TQueue`, `TVariant`, `TFunction`, `TUniquePtr`, `TSharedRef`/`TSharedPtr`, `Algo::*`, `FHash::TypeHash<T>()`, `RHIChecked*`, `RHIAlign*`, `LUMEN_PLATFORM_*`, `LUMEN_ASSERT`.
- **Object Forwarding**: All concrete `IRHIObject` subclasses compose `RHI::FRHIObjectBase Identity;` and use `LUMEN_RHI_OBJECT_DECLARATIONS` + `LUMEN_RHI_OBJECT_FORWARDERS( RHIOpenGL, FClassName )` (`<RHI/Types/RHIObjectForward.hpp>`). No virtual inheritance.

### 2.3 Meson Configuration (`Source/RHIOpenGL/meson.build`)
```python
RHIOpenGLModule = {
    'Type': 'SharedLibrary',
    'Depends': ['Core', 'Maths', 'RHI'],
    'ExtraTestSources': RHIConformanceSources,
    'ExtraTestIncludeDirectories': RHIConformanceIncludeDirs, # Source/RHI/Tests if exported by Source/RHI/meson.build
}
```
*(Verify the exact variable names exported by `Source/RHI/meson.build` for `RHIConformanceSources` and conformance test headers during wiring.)*

---

## 3. Detailed Subsystem Design

### 3.1 Platform Headless Context & Dynamic Function Loading (`Platform/`, `Loader/`)
1. **Linux Context Provider (`EGLPlatformContext` -> `GLXPlatformContext` fallback)**:
   - Dynamically `dlopen("libEGL.so.1")` (with fallback `"libEGL.so"`).
   - Probe `EGL_EXT_device_enumeration` (`eglQueryDevicesEXT`) + `EGL_EXT_platform_device` (`eglGetPlatformDisplayEXT(EGL_PLATFORM_DEVICE_EXT, ...)`), falling back to `EGL_PLATFORM_SURFACELESS_MESA` (`eglGetPlatformDisplayEXT(0x31DD, EGL_DEFAULT_DISPLAY, nullptr)`), then `eglGetDisplay(EGL_DEFAULT_DISPLAY)`. This works in headless CI/Mesa `llvmpipe` with zero X11/Wayland display server!
   - `eglBindAPI(EGL_OPENGL_API)`. Request 4.6 Core (`EGL_CONTEXT_MAJOR_VERSION=4, MINOR=6, EGL_CONTEXT_OPENGL_PROFILE_MASK=EGL_CONTEXT_OPENGL_CORE_PROFILE_BIT`, plus `EGL_CONTEXT_OPENGL_DEBUG_BIT` when `bEnableDebug` is true); fall back to 4.5 Core if 4.6 is refused.
   - Bind `EGL_NO_SURFACE` if `EGL_KHR_surfaceless_context` is advertised, else create a 1x1 `eglCreatePbufferSurface`.
   - `GetProcAddress`: `eglGetProcAddress`, falling back to `dlsym` on `libOpenGL.so.0` / `libGL.so.1` for core 1.x symbols if an ICD returns null for core functions.
   - If EGL fails (e.g., X11-only legacy driver without EGL), fall back to `GLXPlatformContext` (`dlopen("libGL.so.1")`, `glXChooseFBConfig` Pbuffer + `glXCreateContextAttribsARB`).
2. **Windows (`WGLPlatformContext`)**:
   - Create hidden dummy `HWND` (`STATIC` class), `GetDC`, `ChoosePixelFormat` + `SetPixelFormat`, temporary legacy context to load `wglCreateContextAttribsARB` + `wglChoosePixelFormatARB`, then replace with Core 4.6 (or 4.5 fallback) context (`WGL_CONTEXT_CORE_PROFILE_BIT_ARB`, optional `WGL_CONTEXT_DEBUG_BIT_ARB`).
3. **Failure Resilience**:
   - If no OS context can be initialized (e.g., stripped sandbox without GPU or Mesa), `EnumerateAdapters()` returns an empty `TVector<FRHIAdapterInfo>` (or structured `Unsupported` if explicitly failed), and `CreateDevice()` returns `MakeRHINotFound` / `MakeRHIUnsupported` without crashing or leaking library handles.

### 3.2 Module, Capability Probing & Device (`Module/`, `Device/`)

#### `FOpenGLModule` (`IRHIModule`)
- `GetApiVersion()`: returns `FRHIApiVersion{ 1U, 1U, 0U }` (matching `RHI.md` contract version **1.1.0**).
- `EnumerateAdapters()`:
  - Probes available headless context(s), reads `GL_VENDOR`, `GL_RENDERER`, `GL_VERSION`, `GL_SHADING_LANGUAGE_VERSION`, `GL_NUM_EXTENSIONS` + `glGetStringi(GL_EXTENSIONS, i)`.
  - Assigns deterministic nonzero `AdapterId` via `RHIHashBytes` over `"RHIOpenGL:" + Vendor + ":" + Renderer + ":" + DeviceIndex` (ensuring nonzero by mapping `0 -> 1`).
  - Infers `ERHIAdapterType`:
    - `Software` if renderer contains `"llvmpipe"`, `"softpipe"`, `"swrast"`, `"swiftshader"`, `"microsoft basic render"`.
    - `Integrated` if `"intel"` / `"amd radeon graphics"` / `"vega"` / `"mali"` / `"adreno"` heuristic; else `Discrete` (or `Unknown` if unrecognized).
  - **Queue Families**:
    - `Graphics`: `Count = 1U`, `bSupportsPresentation = true`, `bSupportsTimestamps = SupportsTimestampQueries`
    - `Compute`: `Count = 1U`, `bSupportsPresentation = false`, `bSupportsTimestamps = SupportsTimestampQueries`
    - `Copy`: `Count = 1U`, `bSupportsPresentation = false`, `bSupportsTimestamps = SupportsTimestampQueries`
  - **Honest Capability Table (`FRHICapabilities`)**:
    | RHI Capability Flag | Probed OpenGL Condition |
    | --- | --- |
    | `DynamicRendering` | Always `true` (FBOs are dynamic) |
    | `TimelineSemaphores` | Always `true` (scheduler + `GLsync` + atomic timeline) |
    | `BindlessResources` | Only `true` if `GL_ARB_bindless_texture` is present |
    | `IndirectDraw` | `true` on GL >= 4.3 (`GL_ARB_draw_indirect` + compute indirect) |
    | `MultiDrawIndirect` | `true` on GL >= 4.3 (`GL_ARB_multi_draw_indirect`) |
    | `TimestampQueries` | `true` on GL >= 3.3 (`GL_ARB_timer_query`) |
    | `OcclusionQueries` | Always `true` (`GL_SAMPLES_PASSED` / `GL_ANY_SAMPLES_PASSED`) |
    | `PipelineStatistics` | Only `true` if `GL_ARB_pipeline_statistics_query` is present |
    | `MeshShaders` | Only `true` if `GL_NV_mesh_shader` or `GL_EXT_mesh_shader` |
    | `RayTracing` | `false` |
    | `VariableRateShading` | Only `true` if `GL_NV_shading_rate_image` |
    | `ConservativeRasterization` | Only `true` if `GL_NV_conservative_raster` or `GL_INTEL_conservative_rasterization` |
    | `DepthClamp` | Always `true` (`GL_DEPTH_CLAMP`) |
    | `AnisotropicFiltering` | `true` if GL >= 4.6 or `GL_EXT_texture_filter_anisotropic` / `GL_ARB_texture_filter_anisotropic` |
    | `AsyncCompute` | `true` (scheduler executes `Compute` queue with full semaphore dependency graph) |
    | `Presentation` | Always `true` (via `IRHISurfaceBridge` + `Null` offscreen) |
    | `GeometryShaders` | Always `true` (GL 3.2+ core) |
    | `Tessellation` | Always `true` (GL 4.0+ core) |
    - `ShaderFormats`: always contains `ERHIShaderFormat::Source` (GLSL); adds `ERHIShaderFormat::PortableBytecode` and `ERHIShaderFormat::BackendBytecode` **only** if `GL >= 4.6` or `GL_ARB_gl_spirv` is present.
  - **Quantified Limits (`FRHILimits`)**:
    - `MaxBufferSize`: `Max(256 MiB, Min(GL_MAX_SHADER_STORAGE_BLOCK_SIZE, 2 GiB))` (clamped positive)
    - `MaxTextureDimension1D` / `2D`: `GL_MAX_TEXTURE_SIZE`
    - `MaxTextureDimension3D`: `GL_MAX_3D_TEXTURE_SIZE`
    - `MaxTextureArrayLayers`: `GL_MAX_ARRAY_TEXTURE_LAYERS`
    - `MaxColorAttachments`: `GL_MAX_COLOR_ATTACHMENTS` (always >= 8 in GL 4.5+)
    - `MaxBindingSets`: `8U`
    - `MaxDescriptorsPerSet`: `32U` (validated against `GL_MAX_COMBINED_*`)
    - `MaxInlineConstantBytes`: `256U` (emulated via reserved UBO slot `0`)
    - `MaxVertexAttributes`: `GL_MAX_VERTEX_ATTRIBS`
    - `MaxVertexBuffers`: `GL_MAX_VERTEX_ATTRIB_BINDINGS`
    - `MaxComputeWorkgroupSize`: `GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS`
    - `MinBufferAlignment`: power-of-two LCM of `GL_UNIFORM_BUFFER_OFFSET_ALIGNMENT` and `GL_SHADER_STORAGE_BUFFER_OFFSET_ALIGNMENT` (typically `256ULL`)
    - `MinTextureRowAlignment`: `1ULL` (`GL_PACK_ALIGNMENT` / `GL_UNPACK_ALIGNMENT = 1` supported by core GL)
    - `TimestampPeriod`: `1ULL` (OpenGL `GL_TIMESTAMP` is in nanoseconds)
    - Assert `AdapterInfo.Validate()` succeeds before caching/returning.

#### `FOpenGLDevice` (`FRHIDeviceBase`)
- `CreateDevice(InDesc)`:
  1. Call `InDesc.Validate()` + `EnumerateAdapters()`.
  2. Select matching `AdapterId` (or adapter `0` when `AdapterId` is empty; return `MakeRHINotFound` if requested ID does not exist).
  3. Run `RHI::CheckRHICapabilityGate( Adapter, InDesc )` (`<RHI/Device/RHIBackendSupport.hpp>`).
  4. Validate `InDesc.Queues` against `Adapter.QueueFamilies` (every requested `Count <= Family.Count`; reject unsupported queue type or excess count with `MakeRHIInvalidQueue` / `MakeRHIUnsupported`).
  5. Spawn `FOpenGLContext` (creates headless GL context on the GL Executor `FThread`, initializes `GLDispatchTable`, sets `glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE)`, enables `GL_TEXTURE_CUBE_MAP_SEAMLESS`, configures `glDebugMessageCallback` if `InDesc.bEnableDebug`).
  6. Instantiate requested `FOpenGLQueue` instances (stored inside `FOpenGLDevice`) and return `TSharedRef<IRHIDevice>`.
- **Device Loss Detection**:
  - Periodically on GL thread (after submissions / errors), check `glGetGraphicsResetStatus() != GL_NO_ERROR`. If reset occurs, transition `SetStatus(ERHIDeviceStatus::Lost)`, notify `DebugSink`, wake all waiting queues/semaphores, and fail subsequent creation/submission calls with `MakeRHIDeviceLost`.

### 3.3 Format & Type Translation (`Types/`)
- Complete table covering all 54 `ERHIFormat` values in `OpenGLFormatMapping`:
  - **Sized Internal Format**, **External Upload/Download Format**, **Pixel Data Type**, **Vertex Attribute `(Components, Type, bNormalized, bInteger)`**, and **Supported Usage Mask** (e.g., `RGB32*` supports VertexBuffer/ShaderResource, not RenderTarget; `BC*` supports ShaderResource/Copy only when S3TC/BPTC extension is present).
  - **`BGRA8Unorm` / `BGRA8Srgb`**: stored internally as `GL_RGBA8` / `GL_SRGB8_ALPHA8`, uploaded/downloaded with external format `GL_BGRA` (`GL_UNSIGNED_BYTE`), vertex format uses `size = GL_BGRA` (`GL_UNSIGNED_BYTE`, normalized). This avoids double-swizzle bugs while preserving RGBA shader semantics and BGRA memory transfers.
  - **Depth/Stencil Pixel Transfers**:
    - `ERHIFormatAspect::Depth` -> `GL_DEPTH_COMPONENT`
    - `ERHIFormatAspect::Stencil` -> `GL_STENCIL_INDEX`
    - `Depth | Stencil` -> `GL_DEPTH_STENCIL` (`GL_UNSIGNED_INT_24_8` for `D24S8`, `GL_FLOAT_32_UNSIGNED_INT_24_8_REV` for `D32FS8`)

### 3.4 Memory Architecture & Resources (`Memory/`, `Resources/`)
1. **`FOpenGLHeap` (`IRHIHeap`)**:
   - Validates `InDesc.Validate()`.
   - Allocates a backing `GLuint BackingBuffer` via `glCreateBuffers` + `glNamedBufferStorage`:
     - `DeviceLocal`: `GL_DYNAMIC_STORAGE_BIT`
     - `Upload`: `GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_DYNAMIC_STORAGE_BIT` (persistently mapped once on GL thread!)
     - `Readback`: `GL_MAP_READ_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT | GL_DYNAMIC_STORAGE_BIT | GL_CLIENT_STORAGE_BIT` (persistently mapped once on GL thread!)
   - `Map(InOffset, InSize)` / `Unmap()`: rejects `DeviceLocal` (`InvalidArgument` with `ERHIErrorCondition::Memory`), checks `InSize > 0` + `RHICheckedAdd(InOffset, InSize) <= Desc.Size`, rejects double-map (`InvalidState`), returns `TSpan<Byte>`.
2. **`FOpenGLBuffer` (`IRHIBuffer`)**:
   - Dedicated (`CreateBuffer`) or Placed (`CreatePlacedBuffer`).
   - Placed creation validates: heap belongs to same `FOpenGLContext` (`TryRHIBackendCast<FOpenGLHeap>`), `InAllocation.IsValid()`, `InAllocation.Heap == nullptr || InAllocation.Heap == InHeap.Get()`, `InAllocation.MemoryClass == InDesc.MemoryClass`, `InDesc.Size <= InAllocation.Size`, `RHICheckedAdd(InAllocation.Offset, InAllocation.Size) <= HeapDesc.Size`, and `InAllocation.Offset % Max(InAllocation.Alignment, Limits.MinBufferAlignment) == 0`. Retains `TSharedRef<IRHIHeap> ParentHeap` and records `HeapOffset = InAllocation.Offset` (placed buffers share `ParentHeap`'s `BackingBuffer` and offset directly into it in all GL range/copy/vertex/index calls — giving genuine GPU memory aliasing!).
   - `Map`/`Unmap`: uses persistent coherent pointer directly (zero GL round-trip) with per-buffer `bIsMapped` state tracking and automatic `Unmap` on destruction.
3. **`FOpenGLTexture` (`IRHITexture`) & `FOpenGLTextureView` (`IRHITextureView`)**:
   - Deterministic `ComputeTextureRequirements(InDesc)`: computes exact byte size across all mip levels & array layers using `GetRHIFormatInfo`, aligned to `Max(Limits.MinBufferAlignment, 4096ULL)`. Both `CreateTexture` and `CreatePlacedTexture` use this exact function.
   - Sample count validation (`SampleCount > Count1` rejects `MipCount > 1` and `Depth > 1`, checks `<= GL_MAX_SAMPLES`).
   - Target selection:
     - `SampleCount > Count1`: `LayerCount == 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D_MULTISAMPLE_ARRAY`
     - `Depth > 1`: `GL_TEXTURE_3D` (requires `LayerCount == 1`)
     - `LayerCount > 1`: `GL_TEXTURE_2D_ARRAY` (compatible with `Texture2D`, `Texture2DArray`, `TextureCube` via `glTextureView`!)
     - Else: `GL_TEXTURE_2D` (so `{1, 1, 1}` textures work seamlessly as `Texture2D` color targets and views)
   - Uses immutable `glCreateTextures` + `glTextureStorage*` so `glTextureView` and `glCopyImageSubData` always succeed.
   - `FOpenGLTextureView`: validates `InDesc.Validate()`, checks `InDesc.Texture` belongs to this device, checks target compatibility (e.g., `TextureCube` requires `LayerCount >= 6 && Width == Height`; `Texture1D` returns `Unsupported` if parent is `2D`/`3D` since `glTextureView` forbids `2D -> 1D`), checks format view-class compatibility if format is overridden, creates `glTextureView` (for format/subresource slices) or aliases parent handle cleanly when full range + same target/format + multisample, and retains `TSharedRef<IRHITexture>`.
4. **`FOpenGLSampler` (`IRHISampler`)**:
   - Validates `InDesc.Validate()`. Gates `MaxAnisotropy > 1U` on `ERHICapabilityFlags::AnisotropicFiltering` (`MakeRHIUnsupported` if absent; clamps to `GL_MAX_TEXTURE_MAX_ANISOTROPY` otherwise). Creates via `glCreateSamplers`.
5. **Memory Pools & Allocators**:
   - **`FOpenGLBufferPool` (`IRHIBufferPool`)**: mutex-protected `Available` & `Pending` buckets matching exact `FRHIBufferDesc`; validates foreign buffers; moves pending buffers to `Available` on `AdvanceCompleted` when `IsRHICompletionReached`.
   - **`FOpenGLUploadAllocator` (`IRHIUploadAllocator`) & `FOpenGLReadbackAllocator` (`IRHIReadbackAllocator`)**: compose `RHI::FRHIRingAllocator(InCapacity)` + a persistently-mapped `FOpenGLBuffer` (`Upload`/`Readback`), tracking active allocation IDs under mutex. `GetBuffer` validates ownership and returns the backing staging buffer.
   - **`FOpenGLTransientResourceAllocator` (`IRHITransientResourceAllocator`)**: validates `BeginBatch`/`EndBatch` nesting; on `Allocate(InBatch)`, validates `InBatch.Validate()`, sorts requests by `Lifetime.Begin` (`Algo::Sort`), performs interval-graph coloring + aligned offset packing per `MemoryClass` into reusable `FOpenGLHeap` backing objects (grown on demand), assigns `FRHIAllocationId` + `Heap*`, and returns `TVector<FRHITransientAllocation>`.
   - **`FOpenGLDefragmenter` (`IRHIDefragmenter`)**:
     - `BuildPlan`: validates non-overlapping valid input allocations, groups per heap, sorts by offset (`Algo::StableSort`), packs toward offset `0` respecting alignment (`RHIAlignUp`), avoids overlapping hazard conflicts (or stages safely), computes `ReclaimedBytes`.
     - `Execute`: validates `InPlan.Validate()`, verifies `IsRHICompletionReached(InPlan.ObservedCompletion, Move.Completion)` for every move (`InvalidState` if not reached), executes `glCopyNamedBufferSubData` on the GL thread (using scratch buffer when source and destination overlap within the same heap), and emits `GetRHIAllocationObserverRegistry().NotifyMoved(Move.Source, Move.Destination)`.

### 3.5 Bindings & Descriptor Allocation (`Bindings/`)
- **`FOpenGLBindingLayout`**: validates `FRHIBindingLayoutDesc::Validate()`, gates `bBindless` on `ERHICapabilityFlags::BindlessResources`, checks total descriptor count `<= Limits.MaxDescriptorsPerSet`, caches `Desc.GetHash()`.
- **`FOpenGLBindingSet`**: mutex + atomic `bDirty`. `Write(InWrites)` validates **every** entry upfront (slot exists in layout, `ArrayElement < Entry.Count`, `TVariant` matches `ERHIBindingType`, resource belongs to device, resource `Usage` flags match binding type, buffer offset + size fit and satisfy `MinBufferAlignment`) before **atomically swapping** the entire binding snapshot and setting `bDirty = true`.
- **`FOpenGLDescriptorAllocator`**: derives `RHI::FRHIDescriptorAllocatorBase` and implements `RHI::IRHIDescriptorPoolBackend` (`CreatePool`/`ReleasePool` issuing device-scoped pool tokens under mutex).

### 3.6 Shaders, Pipeline Layouts, Pipelines & Caches (`Shaders/`, `Pipelines/`, `Caches/`)
- **`FOpenGLShader` (`IRHIShader`)**:
  - Validates `InDesc.Validate()`, checks `Capabilities.Supports(InDesc.Format)`, gates stage (`Geometry`/`Tessellation`/`MeshShaders`), validates non-empty bytecode (and 4-byte alignment + SPIR-V magic `0x07230203` for `PortableBytecode`/`BackendBytecode`).
  - Copies bytecode into owned `TVector<Byte>` and updates `Desc.Bytecode` span to point at owned bytes.
  - Compiles on GL thread (`glCreateShader` + `glShaderSource` + `glCompileShader` for `Source`; `glShaderBinary(GL_SHADER_BINARY_FORMAT_SPIR_V)` + `glSpecializeShader(..., EntryPoint)` for SPIR-V). On failure, extracts `glGetShaderInfoLog`, emits to `DebugSink`, deletes shader, and returns `MakeRHIBackendFailure(ERHIErrorCondition::Shader, ...)`.
- **`FOpenGLPipelineLayout` (`IRHIPipelineLayout`)**:
  - Validates `InDesc.Validate()`, checks `BindingLayouts.size() <= Limits.MaxBindingSets`, checks inline constant max end `<= Limits.MaxInlineConstantBytes`, checks device ownership of layouts.
  - Deterministically linearizes `(SetIndex, SortedSlot)` into flat GL units:
    - Reserve **UBO unit `0`** for Inline Constants (`glBindBufferRange(GL_UNIFORM_BUFFER, 0, ...)`).
    - `ConstantBuffer` -> UBO units `[1 .. GL_MAX_COMBINED_UNIFORM_BLOCKS - 1]`
    - `ReadOnlyBuffer` / `ReadWriteBuffer` -> SSBO units `[0 .. GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS - 1]`
    - `Texture` / `CombinedTextureSampler` / `Sampler` -> Texture/Sampler units `[0 .. GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS - 1]`
    - `StorageTexture` -> Image units `[0 .. GL_MAX_IMAGE_UNITS - 1]`
    - Rejects with `MakeRHIInvalidLimit` if any GL resource unit cap is exceeded.
- **`FOpenGLGraphicsPipeline` & `FOpenGLComputePipeline` (`IRHIPipeline`)**:
  - Validates descriptor, device ownership, format aspects (`ColorFormats` must be Color; `DepthStencilFormat` if not `Unknown` must be Depth/Stencil), attachment count `<= Limits.MaxColorAttachments`, vertex limits, `SampleCount <= GL_MAX_SAMPLES`, `!bDepthClip` gated on `DepthClamp`.
  - Links `GLuint Program` on GL thread (checking `GL_LINK_STATUS` -> returning `MakeRHIBackendFailure(ERHIErrorCondition::Pipeline, InfoLog)` on link error).
  - Binds reflection/program resources (`OpenGLShaderBinder`) via `glUniformBlockBinding`, `glShaderStorageBlockBinding`, and `glProgramUniform1i` to the pipeline layout's deterministic flat GL units (plus inline constant UBO unit `0`).
  - `OpenGLVAOCache` builds/caches a `GLuint VAO` on the GL thread using GL 4.5 DSA `glVertexArrayAttribFormat`, `glVertexArrayAttribBinding`, `glEnableVertexArrayAttrib`, and `glVertexArrayBindingDivisor` (`Vertex` = `0`, `Instance` = `1`) keyed by `VertexLayout.GetHash()` (even an empty layout gets a valid non-zero VAO for core-profile procedural draws).
  - Computes canonical `FRHIPipelineCompatibility`.
- **`FOpenGLPipelineCache` (`IRHIPipelineCache`)**:
  - Synchronized via `HAL::FPlatformRecursiveSharedMutex` + `RHI::FRHICacheStatsTracker`.
  - Enforces `Desc.MaxEntries`, `Desc.MaxBytes`, and `Desc.bAllowEviction` (LRU eviction when `true`; `OutOfMemory` when `false` and full).
  - `InitialData` & `Serialize()`: versioned binary format (`Magic = "LPCO"`, `FormatVersion`, `BackendVersion`, `EntryCount`, FNV-1a checksum over payload, serialized `FRHIPipelineCacheKey` fields + `glGetProgramBinary` blob when `GL_ARB_get_program_binary` is supported). Corrupted header, truncated payload, version mismatch, or checksum failure returns `MakeRHICorrupted(ERHIErrorCondition::Cache, ...)`.

### 3.7 Commands, Recording, FBO/State Cache & Execution (`Commands/`)
1. **`FOpenGLCommandAllocator` (`IRHICommandAllocator`)**:
   - Derives `RHI::FRHICommandAllocatorBase(InQueueType)`. Tracks child lists, resets only when no list is pending above `InCompleted`.
2. **`FOpenGLCommandList` (`IRHICommandList`)**:
   - Uses `ERHICommandListState` + `RHITransitionCommandList` + `FRHICommandStream`.
   - Enforces full validation at recording time before writing trivially-copyable command structs into `FRHICommandStream` and retaining `TSharedRef`/`TSharedPtr` inputs via `Stream.Retain(...)`:
     - **Queue capability checks**: `Copy` queue allows only transitions, copy commands, timestamp queries (if supported), and debug regions; `Compute` queue disallows rendering/draw/vertex/index/viewport/scissor/blend/depth-bias/stencil commands; `Graphics` queue allows all.
     - **Scope checks**: `BeginRendering` / `Copy*` / `Transition` / `Dispatch*` require `Recording`; `EndRendering` / `Draw*` require `Rendering`; `End()` rejects open `Rendering` or open `ActiveQuery`; `Reset()` rejects `Pending` or open `ActiveQuery`.
     - **Pipeline & Dynamic state checks**: `SetBindingSet` validates bound pipeline + layout compatibility + index bounds; `SetInlineConstants` calls `ValidateRHIInlineConstantWrite` and checks layout inline constant range; `SetBlendConstants`/`SetDepthBias`/`SetStencilReference` return `InvalidState` when the bound graphics pipeline lacks the corresponding `ERHIDynamicState` flag; `Draw*` requires compatible bound graphics pipeline, required dynamic states recorded, bound vertex buffer slots, and (for `DrawIndexed`) bound index buffer; `Indirect` draws/dispatches gate on `IndirectDraw` capability and validate `FRHIIndirectArguments::Validate`.
     - **Queries & Debug Regions**: validates active query pool ownership, index bounds, single-active-query invariant, and balanced debug region count.
3. **`FOpenGLCommandExecutor` + `FOpenGLFramebufferCache` + `FOpenGLStateCache`**:
   - **`BeginRendering`**: looks up/configures `GLuint FBO` via `glNamedFramebufferTexture` / `glNamedFramebufferTextureLayer` + `glNamedFramebufferDrawBuffers`, verifies `GL_FRAMEBUFFER_COMPLETE`, toggles `GL_FRAMEBUFFER_SRGB` per sRGB attachment presence, toggles `GL_MULTISAMPLE`, clears attachments using exact `fv`/`iv`/`uiv`/`fi` call per format numeric type under rendering area scissor, and clears/invalidates on `EndRendering` via `glInvalidateNamedFramebufferData` when `StoreOp == DontCare`.
   - **Coordinate System**: `glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE)` maps NDC depth to `[0, 1]`; `glViewport` & `glScissor` convert RHI top-left `(X, Y, W, H)` to OpenGL bottom-left `GL_Y = FBHeight - (Y + H)`.
   - **Barriers**: translates `FRHIBufferTransition` / `FRHITextureTransition` `(Before -> After)` into targeted `glMemoryBarrier(...)` bits (`SHADER_STORAGE`, `SHADER_IMAGE_ACCESS`, `TEXTURE_FETCH`, `VERTEX_ATTRIB_ARRAY`, `ELEMENT_ARRAY`, `COMMAND`, `UNIFORM`, `BUFFER_UPDATE`, `FRAMEBUFFER`, `CLIENT_MAPPED_BUFFER`).
   - **Inline Constants**: maintains a 64 KiB ring UBO bound at UBO unit `0` (`glBindBufferRange`) updated before `Draw*`/`Dispatch*` whenever dirty.
   - **Copies**: `glCopyNamedBufferSubData` (with placed `HeapOffset`), `glCopyImageSubData`, `glTextureSubImage2D/3D` & `glCompressedTextureSubImage2D/3D` via `GL_PIXEL_UNPACK_BUFFER`,
