{
  stdenv,
  lib,
  cmake,
  ninja,
  pkg-config,
  mold,
  sdl3,
  vulkan-headers,
  vulkan-loader,
  vulkan-memory-allocator,
  vk-bootstrap,
  version,
}:

stdenv.mkDerivation {
  pname = "LumenEngine";
  inherit version;
  src = ./..;

  preConfigure = ''
    export CCACHE_DIR="~/$HOME/.cache/ccache-nix"
    mkdir -p "$CCACHE_DIR"
  '';

  nativeBuildInputs = [
    cmake
    ninja
    pkg-config
    mold
  ];

  buildInputs = [
    sdl3
    vulkan-headers
    vulkan-loader
    vulkan-memory-allocator 
    vk-bootstrap
  ];

  cmakeFlags = [
    "-GNinja"
    "-DCMAKE_BUILD_TYPE=Release"
    "-DCMAKE_SKIP_BUILD_RPATH=OFF"
    "-DCMAKE_BUILD_WITH_INSTALL_RPATH=ON"
    "-DCMAKE_INSTALL_RPATH=$out/lib"
  ];

  installPhase = ''
    runHook preInstall
    mkdir -p $out/bin $out/lib
    [ -f Examples/BaseExample/BaseExample ] && cp Examples/BaseExample/BaseExample $out/bin/
    find . -name "*.so" -exec cp {} $out/lib/ \;
    runHook postInstall
  '';
}
