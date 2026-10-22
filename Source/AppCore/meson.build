#{
# Source/AppCore/meson.build
# Source/AppCore meson entry-point
#}

PlatformDeps = []

if OSName == 'linux'
    PlatformDeps += dependency('sdl3', required : true)
endif

ModuleDefinition = {
    'Name' : 'AppCore',
    'Type' : 'SharedLibrary',
    'SourceDir' : meson.current_source_dir(),
    'Private' : 'Private',
    'Public' : 'Public',
    'Tests' : 'Tests',
    'Depends' : ['Core', 'Maths'],
    'ExternalDeps' : PlatformDeps,
}
