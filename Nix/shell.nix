{
  pkgs,
  inputs,
  ...
}:
let
  deps = import ./deps.nix { inherit pkgs; };
  stdenv = pkgs.overrideCC pkgs.stdenv pkgs.ccacheWrapper;
in
pkgs.mkShell.override { inherit stdenv; } {
  nativeBuildInputs =
    deps.nativeBuildInputs
    ++ deps.buildInputs
    ++ (with pkgs; [
      gcc16
      valgrind
      gdb
      gtest
      clang-tools
      jq
    ]);

  shellHook = ''
    mkdir -p $PWD/cache/
    export CCACHE_DIR="$PWD/cache/.ccache"
    export CCACHE_SLOPPINESS="pch_defines,time_macros,file_stat_matches,file_macro"
    #export CC="ccache gcc"
    #export CXX="ccache g++"
    ${pkgs.lib.optionalString pkgs.stdenv.hostPlatform.isLinux ''
      export NIX_CFLAGS_LINK="-fuse-ld=mold"
    ''}
  '';
}
