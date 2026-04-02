{
  pkgs,
  lumenPackage,
  lumenCli,
}:

let
  vmaInclude = "${pkgs.vulkan-memory-allocator}/include";

  vulkanLibs = pkgs.lib.makeLibraryPath [
    pkgs.vulkan-loader
    pkgs.sdl3
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
    export CPATH="${vmaInclude}:$CPATH"
    export CPLUS_INCLUDE_PATH="${vmaInclude}:$CPLUS_INCLUDE_PATH"
    
    export LD_LIBRARY_PATH="${vulkanLibs}:$LD_LIBRARY_PATH"
    export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"
  '';
}
