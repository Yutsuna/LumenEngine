{
  pkgs,
  ...
}:
{
  nativeBuildInputs =
    with pkgs;
    [
      gcc16
      meson
      ninja
      pkg-config
    ]
    ++ lib.optionals pkgs.stdenv.hostPlatform.isLinux [
      mold
    ];

  buildInputs = with pkgs; [
    sdl3
  ];

  devInputs = with pkgs; [
    valgrind
    gdb
    clang-tools
    jq
    nil
    nixd
  ];
}
