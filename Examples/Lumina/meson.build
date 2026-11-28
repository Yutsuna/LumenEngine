#{
# Example/Lumina/meson.build
# Example Lumen Game Module
#}

LuminaShaderInstallDir = get_option('datadir') / 'Lumina' / 'Shaders'
install_data(
    'Shaders/Fullscreen.vert',
    'Shaders/PathTrace.frag',
    'Shaders/BloomExtract.frag',
    'Shaders/BloomBlur.frag',
    'Shaders/Display.frag',
    install_dir : LuminaShaderInstallDir,
)

ModuleDefinition = {
    'Name' : 'Lumina',
    'Type' : 'Executable',
    'SourceDir' : meson.current_source_dir(),
    'Private' : 'Private',
    'Public' : 'Public',
    'Depends' : ['Core','Maths','Scene','AppCore','RHI','Renderer','RHIOpenGL','Launch'],
}
