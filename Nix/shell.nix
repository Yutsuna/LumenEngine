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

    ${pkgs.lib.optionalString pkgs.stdenv.hostPlatform.isLinux ''
      export NIX_CFLAGS_LINK="-fuse-ld=mold"
    ''}
  '';
}
