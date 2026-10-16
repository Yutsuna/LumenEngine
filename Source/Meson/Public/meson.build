#{
# Source/Meson/Public/meson.build
# Exposes each module's Public/ directory as an include directory.
#
# Reads  : ModuleDefinitions
# Writes : PublicIncludes
#}

foreach ModuleDefinition : ModuleDefinitions
    ModuleName = ModuleDefinition.get('Name')
    ModuleSourceDirRel = ModuleDefinition.get('SourceDirRel')
    PublicDirectory = ModuleDefinition.get('Public', 'Public')

    RelPublicPath = '../../@0@/@1@'.format(ModuleSourceDirRel, PublicDirectory)

    IncludeDirectories = []

    if FileSystem.is_dir(RelPublicPath)
        IncludeDirectories += include_directories(RelPublicPath)
    endif

    PublicIncludes += { ModuleName : IncludeDirectories }
endforeach