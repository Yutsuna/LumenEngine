# Module: RHIVulkan (work in progress)

- **Type:** shared library; implements the `RHI` interfaces with a Vulkan backend
  (`vulkan-loader`, `vulkan-headers`, validation layers and `vulkan-tools`
  provided by the Nix devshell). White-box tests reuse the implementation objects
  statically without exporting private symbols from the shared library.
- **Status:** built; V2 native module/device/queue code is present, and the V3a
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
  integration, and the V3a heap/staging/resource/pool/transient/defrag paths.
  No shared-conformance factory or custom driver-policy runner is currently
  wired into the module. Submission, presentation, native shader modules,
  pipelines and queries remain absent. Native defragmenter `Execute` is an
  explicit `Unsupported` boundary until the V4 command milestone.
- **Shaders:** shader build tools are available in the devshell, but there is no
  shader-generation target or generated shader artifact in the module. No
  Python script or runtime Python dependency is used.
