#{
# Source/Launch/meson.build
# Source/Launch meson entry-point
#}

PlatformDepends = []

if OSName == 'linux'
    PlatformDepends += ['RHIOpenGL']
endif

ModuleDefinition = {
    'Name' : 'Launch',
    'Type' : 'SharedLibrary',
    'SourceDir' : meson.current_source_dir(),
    'Private' : 'Private',
    'Public' : 'Public',
    'Tests' : 'Tests',
    'Depends' : ['Core', 'AppCore', 'Scene', 'RHI', 'Renderer'] + PlatformDepends,
}
