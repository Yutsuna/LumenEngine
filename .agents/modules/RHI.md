# Module: RHI (work in progress)

- **Type:** Shared library (`Source/RHI/meson.build`), depends on `Core` and
  `Maths`.
- **Status:** **configured but empty.** `RHI` is listed in `ModuleNames`
  (`Source/meson.build`) and has a valid `meson.build`, but every file under
  `Source/RHI/{Public,Private,Tests}` is a 0-byte placeholder. There is no
  usable API yet: do not include its headers or depend on its behaviour until
  the stubs are implemented.
- **Role:** hardware-agnostic render-hardware interface abstraction layered
  over AppCore/OS and under Renderer.

## Intended layout (header/file names are present as stubs)

`Public/` is organised by topic:

- `Types/` — `IRHIObject`, `RHIFormat`, `RHICapabilities`, `RHILimits`,
  `RHIError`, `RHIResourceState`, `RHIIdentifiers`, `RHIExtent`,
  `RHISampleCount`, `RHIShaderStage`, `RHIQueueType`, `RHITimeline`, …
- `Device/` — `IRHIDevice`, `RHIBackendCast`, `RHIDeviceStatus`.
- `Module/` — `IRHIModule`, `RHIAdapterInfo`, `RHIDeviceDesc`.
- `Resources/` — `IRHIResource`, `IRHIBuffer`, `IRHITexture`,
  `IRHITextureView`, `IRHISampler` and their descriptors.
- `Memory/` — allocator interfaces (`IRHIAllocator`, `IRHIHeap`,
  `IRHITransientResourceAllocator`, `IRHIDefragmenter`, …) plus concrete
  allocator declarations (`RHILinearAllocator`, `RHIRingAllocator`,
  `RHITlsfAllocator`, `RHIPoolAllocator`, …).
- `Pipelines/` — `IRHIPipeline`, `IRHIPipelineLayout`, graphics/compute
  descriptors, blend/depth-stencil/rasterizer/multisample state.
- `Commands/` — `IRHICommandList`, `IRHICommandAllocator`,
  `RHICommandStream`, transitions, copy/draw/rendering descriptors.
- `Bindings/` — `IRHIBindingLayout`, `IRHIBindingSet`,
  `IRHIDescriptorAllocator`, descriptor/binding descriptors.
- `Synchronization/` — `IRHISemaphore`, `RHIDeferredReleaseQueue`,
  `RHISemaphoreDesc`.
- `Presentation/` — `IRHISwapchain`, swapchain/surface descriptors, present
  mode, colour space.
- `Queue/`, `Queries/`, `Shaders/`, `Caches/`, `Debug/`, `Utilities/`,
  `Validation/` — queues/submits, query pools, shader packages/reflection,
  caches, debug sinks, checked math/alignment and a validation layer.

`Private/` mirrors those topics; `Tests/` contains `Null*` conformance
backends and per-topic tests (`Types`, `Memory`, `Commands`, `Pipelines`,
`Resources`, `Validation`, …).

## Notes for agents

- Treat this doc as a map of intended names, not a working API: the files are
  empty until the module is implemented.
- Naming follows `rules/coding-style.md`: backend-agnostic interfaces are
  `I`-prefixed pure interfaces, backend objects are `F`-prefixed.
- `RHIOpenGL` and `RHIVulkan` are meant to implement these interfaces as
  separate modules; they are still empty and absent from `ModuleNames`.
- Nix devshell already provides Vulkan headers/loader and validation layers
  (`VK_LAYER_PATH`) and Mesa for OpenGL; only the modules are missing.
