{
  pkgs,
  ...
}:
{
  nativeBuildInputs = with pkgs; [
    meson
    ninja
    pkg-config
    ccache
    gdb
    valgrind
  ] ++ lib.optionals pkgs.stdenv.hostPlatform.isLinux [
    mold
  ];

  buildInputs = with pkgs; [
  ];
}