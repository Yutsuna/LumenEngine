#Requires -Version 5.1

$LumenConfig = @{
    MsysUrl  = 'https://github.com/msys2/msys2-installer/releases/download/nightly-x86_64/msys2-base-x86_64-latest.sfx.exe'
    Packages = @{
        NativeBuildInputs = @(
            'mingw-w64-ucrt-x86_64-gcc'
            'mingw-w64-ucrt-x86_64-ccache'
            'mingw-w64-ucrt-x86_64-meson'
            'mingw-w64-ucrt-x86_64-ninja'
            'mingw-w64-ucrt-x86_64-pkgconf'
        )
        BuildInputs = @(
            'mingw-w64-ucrt-x86_64-sdl3'
            'mingw-w64-ucrt-x86_64-vulkan-headers'
            'mingw-w64-ucrt-x86_64-vulkan-loader'
        )
        DevInputs = @(
            'mingw-w64-ucrt-x86_64-vulkan-validation-layers'
            'mingw-w64-ucrt-x86_64-gdb'
            'mingw-w64-ucrt-x86_64-clang-tools-extra'
            'mingw-w64-ucrt-x86_64-jq'
            'mingw-w64-ucrt-x86_64-muon'
        )
    }
}
