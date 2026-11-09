# Module: RHIVulkan (work in progress)

- **Type:** shared library (planned); implements the `RHI` interfaces with a
  Vulkan backend (`vulkan-loader`, `vulkan-headers`, validation layers and
  `vulkan-tools` provided by the Nix devshell).
- **Status:** **not built.** `Source/RHIVulkan/{Private,Public,Tests}` exist
  but are empty; absent from `ModuleNames`. Do not depend on it yet.
