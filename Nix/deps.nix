{
  pkgs,
  ...
}:
let
  bIsLinux = pkgs.stdenv.hostPlatform.isLinux;
in
{
  nativeBuildInputs =
    with pkgs;
    [
      gcc16
      ccache
      meson
      ninja
      pkg-config
      shaderc
      spirv-cross
      spirv-tools
    ]
    ++ lib.optionals bIsLinux [
      mold
      mesa
      libglvnd
    ];

  buildInputs = with pkgs; [
    sdl3
    vulkan-loader
    vulkan-headers
  ] ++ lib.optionals bIsLinux [
    libglvnd
    mesa
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
    muon
  ];
}
