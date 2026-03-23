{
  pkgs,
  lumenPackage,
  lumenCli,
}:

pkgs.mkShell {
  inputsFrom = [ lumenPackage ];

  packages = [
    lumenCli
    pkgs.vulkan-tools
    pkgs.vulkan-validation-layers
  ];

  shellHook = ''
    export LD_LIBRARY_PATH=${
      pkgs.lib.makeLibraryPath [
        pkgs.vulkan-loader
        pkgs.sdl3
      ]
    }:$LD_LIBRARY_PATH

    export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"
  '';
}
