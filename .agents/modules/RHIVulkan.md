# Module: RHIVulkan (work in progress)

- **Type:** shared library; implements the `RHI` interfaces with a Vulkan backend
  (`vulkan-loader`, `vulkan-headers`, validation layers and `vulkan-tools`
  from the Nix devshell on Linux, from the repo-local MSYS2 on Windows —
  see `skills/windows-workflow.md`). White-box tests reuse the implementation objects
  statically without exporting private symbols from the shared library.
- **Status:** built; native module/device/queue code is present, and the
  memory/resources layer is implemented. Vulkan 1.4 is required for the loader,
  instance and adapters. `Source/RHIVulkan/{Private,Public,Tests}` contain the
  backend scaffolding, handle wrappers, RHI-to-Vulkan type mappings, checked
  conversions and result translation. `Private/RHIVulkan/Memory` and
  `Private/RHIVulkan/Resources` now own native heaps, buffers, images, views,
  samplers, staging rings, buffer recycling, transient placement and compaction
  planning. Listed in `ModuleNames`. Do not depend on it from production
  renderer code yet.
- **Tests:** the standard test runner builds 89 backend-local tests covering
  native-free utilities, synthetic negotiation, real module/device/queue
  integration, and the heap/staging/resource/pool/transient/defrag paths.
  No shared-conformance factory or custom driver-policy runner is currently
  wired into the module. Submission, presentation, native shader modules,
  pipelines and queries remain absent. Native defragmenter `Execute` remains an
  explicit `Unsupported` boundary until copy commands are implemented.
- **Shaders:** shader build tools are in the Linux devshell (`shaderc`,
  `spirv-cross`, `spirv-tools` in `Nix/deps.nix`); they are not in the Windows
  MSYS2 package set. There is no shader-generation target or generated shader
  artifact in the module. No
  Python script or runtime Python dependency is used.
