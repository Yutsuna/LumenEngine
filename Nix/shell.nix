{
  pkgs,
  inputs,
  ...
}:
let
  deps = import ./deps.nix { inherit pkgs; };
in
pkgs.mkShell {
  inherit (deps) nativeBuildInputs buildInputs;

  packages = deps.devInputs;

  shellHook = pkgs.lib.optionalString pkgs.stdenv.hostPlatform.isLinux ''
    export NIX_CFLAGS_LINK="-fuse-ld=mold"
  '';
}
