# Module: RHIVulkan (work in progress)

- **Type:** shared library; implements the `RHI` interfaces with a Vulkan backend
  (`vulkan-loader`, `vulkan-headers`, validation layers and `vulkan-tools`
  provided by the Nix devshell). White-box tests reuse the implementation objects
  statically without exporting private symbols from the shared library.
- **Status:** built. `Source/RHIVulkan/{Private,Public,Tests}` contain the
  backend scaffolding, handle wrappers, RHI-to-Vulkan type mappings, checked
  conversions and result translation. Listed in `ModuleNames`. Do not depend on
  it from production renderer code yet.
