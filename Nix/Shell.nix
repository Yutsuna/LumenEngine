{
  pkgs,
  lumenPackage,
  lumenCli,
}:

let
  vmaInclude = "${pkgs.vulkan-memory-allocator}/include";
  vkBootstrapInclude = "${pkgs.vk-bootstrap}/include";

  vulkanLibs = pkgs.lib.makeLibraryPath [
    pkgs.vulkan-loader
    pkgs.sdl3
    pkgs.vk-bootstrap
  ];
in
pkgs.mkShell {
  inputsFrom = [ lumenPackage ];

  packages = [
    lumenCli
    pkgs.vulkan-tools
    pkgs.vulkan-validation-layers
  ];

  shellHook = ''
    export CPATH="${vmaInclude}:${vkBootstrapInclude}:$CPATH"
    export CPLUS_INCLUDE_PATH="${vmaInclude}:${vkBootstrapInclude}:$CPLUS_INCLUDE_PATH"
    
    export LD_LIBRARY_PATH="${vulkanLibs}:$LD_LIBRARY_PATH"
    export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"
  '';
}
