# Module: RHIVulkan (work in progress)

- **Type:** static library; implements the `RHI` interfaces with a Vulkan backend
  (`vulkan-loader`, `vulkan-headers`, validation layers and `vulkan-tools`
  provided by the Nix devshell). Kept as a static library while the backend is
  under development so that its private mapping utilities remain directly
  testable.
- **Status:** built. `Source/RHIVulkan/{Private,Public,Tests}` contain the
  backend scaffolding, handle wrappers, RHI-to-Vulkan type mappings, checked
  conversions and result translation. Listed in `ModuleNames`. Do not depend on
  it from production renderer code yet.
