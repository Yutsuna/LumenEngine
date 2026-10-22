#{
# Source/AppCore/meson.build
# Source/AppCore meson entry-point
#}

PlatformDeps = []

if OSName == 'linux'
    PlatformDeps += 'SDL3'
elif OSName == 'darwin'
    PlatformDeps += 'Cocoa'
elif OSName == 'windows'
    PlatformDeps += 'Win32'
endif

ModuleDefinition = {
    'Name' : 'AppCore',
    'Type' : 'SharedLibrary',
    'SourceDir' : meson.current_source_dir(),
    'Private' : 'Private',
    'Public' : 'Public',
    'Tests' : 'Tests',
    'Depends' : ['Core', 'Maths'] + PlatformDeps
}
