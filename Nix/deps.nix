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
      mesa
    ];

  buildInputs = with pkgs; [
    sdl3
    vulkan-loader
    vulkan-headers
  ];

  devInputs = with pkgs; [
    valgrind
    gdb
    clang-tools
    jq
    nil
    nixd
    vulkan-tools
    vulkan-validation-layers
  ];
}
