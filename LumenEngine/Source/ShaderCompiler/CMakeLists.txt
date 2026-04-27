LumenFindPackage(GLSLang)
LumenFindPackage(SPIRV-Tools)
LumenFindPackage(SPIRV-Cross)

###########################################################

if(NOT GLSLang_FOUND)
    message(STATUS "[Lumen] ShaderCompiler: skipped (Missing GLSLang)")
    return()
endif()

if(NOT SPIRV-Tools_FOUND)
    message(STATUS "[Lumen] ShaderCompiler: skipped (Missing SPIRV-Tools)")
    return()
endif()

if(NOT SPIRV-Cross_FOUND)
    message(STATUS "[Lumen] ShaderCompiler: skipped (Missing SPIRV-Cross)")
    return()
endif()

###########################################################

LumenModule(
    NAME            ShaderCompiler
    TYPE            shared_library
    SOURCES         "Private/**/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Core
    LINUX_LIBS      glslang::glslang
                    SPIRV-Tools-opt
                    spirv-cross-c
)
