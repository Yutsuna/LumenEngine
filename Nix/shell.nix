{
  pkgs,
  ...
}:
let
  deps = import ./deps.nix { inherit pkgs; };
  ldLibraryPath = pkgs.lib.makeLibraryPath deps.buildInputs;
in
pkgs.mkShell {
  inherit (deps) nativeBuildInputs buildInputs;

  packages = deps.devInputs;

  shellHook = ''
    export LD_LIBRARY_PATH="${ldLibraryPath}:$LD_LIBRARY_PATH"
    export VK_LAYER_PATH="${pkgs.vulkan-validation-layers}/share/vulkan/explicit_layer.d"

    ${pkgs.lib.optionalString pkgs.stdenv.hostPlatform.isLinux ''
      export NIX_CFLAGS_LINK="-fuse-ld=mold"
    ''}
  '';
}
