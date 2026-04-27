LumenFindPackage(GLSLang)
LumenFindPackage(SPIRV-Tools)
LumenFindPackage(spirv_cross_core)
LumenFindPackage(spirv_cross_glsl)

###########################################################

if(NOT GLSLang_FOUND)
    message(STATUS "[Lumen] ShaderCompiler: skipped (Missing GLSLang)")
    return()
endif()

if(NOT SPIRV-Tools_FOUND)
    message(STATUS "[Lumen] ShaderCompiler: skipped (Missing SPIRV-Tools)")
    return()
endif()

if(NOT spirv_cross_core_FOUND OR NOT spirv_cross_glsl_FOUND)
    message(STATUS "[Lumen] ShaderCompiler: skipped (Missing SPIRV-Cross C++ Components)")
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
                    glslang::glslang-default-resource-limits
                    glslang::SPIRV
                    SPIRV-Tools
                    SPIRV-Tools-opt
                    spirv-cross-core
                    spirv-cross-glsl
)
