#{
# Source/Core/meson.build
# Source/Core meson entry-point
#}

CoreExternalDependencies = []

if host_machine.system() == 'windows'
    # MinGW-w64 keeps WaitOnAddress/WakeByAddress* (used by WindowsFutex.cpp) in
    # libsynchronization, and GCC 14+ keeps std::print's helpers (used by Logger.cpp)
    # in libstdc++exp. Both are implicit on MSVC/Linux.
    CoreCompiler = meson.get_compiler('cpp')
    CoreExternalDependencies += CoreCompiler.find_library('synchronization', required : true)
    CoreExternalDependencies += CoreCompiler.find_library('stdc++exp', required : true)
endif

ModuleDefinition = {
    'Name' : 'Core',
    'Type' : 'SharedLibrary',
    'SourceDir' : meson.current_source_dir(),
    'Private' : 'Private',
    'Public' : 'Public',
    'Tests' : 'Tests',
    'Depends' : [],
    'ExternalDeps' : CoreExternalDependencies,
}
