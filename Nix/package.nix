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
  pname = "TrafficEngine";
  inherit version;
  src = pkgs.lib.cleanSource ../.;

  nativeBuildInputs = deps.nativeBuildInputs;
  buildInputs = deps.buildInputs ++ [ pkgs.gcc16.cc.lib ];

  meta = {
    description = "TrafficEngine simulation library";
    platforms = pkgs.lib.platforms.all;
  };
}
