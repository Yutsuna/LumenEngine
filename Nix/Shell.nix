{ pkgs, lumenPackage }:

pkgs.mkShell {
  inputsFrom = [ lumenPackage ];

  buildInputs = with pkgs; [
    vulkan-tools
  ];

  shellHook = ''
    export LD_LIBRARY_PATH=${
      pkgs.lib.makeLibraryPath [
        pkgs.vulkan-loader
        pkgs.sdl3
      ]
    }:$LD_LIBRARY_PATH
  '';
}
