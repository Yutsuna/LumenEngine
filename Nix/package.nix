{
  pkgs,
  ...
}:
let
  gccStdenv = pkgs.gcc16Stdenv;
  deps = import ./deps.nix { inherit pkgs; };
  version = builtins.head (builtins.split "\n" (builtins.readFile ../VERSION.md));
in
gccStdenv.mkDerivation {
  pname = "Lumen";
  inherit version;
  src = pkgs.lib.cleanSource ../.;

  inherit (deps) nativeBuildInputs buildInputs;

  meta = {
    description = "Lumen Rendering Engine";
    platforms = pkgs.lib.platforms.all;
  };
}
