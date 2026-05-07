LumenFindPackage(GLSLang)
LumenFindPackage(SPIRV-Tools)
LumenFindPackage(spirv_cross_core)
LumenFindPackage(spirv_cross_glsl)

###########################################################

if(NOT GLSLang_FOUND)
    message(STATUS "[Lumen] Compiler: skipped (Missing GLSLang)")
    return()
endif()

if(NOT SPIRV-Tools_FOUND)
    message(STATUS "[Lumen] Compiler: skipped (Missing SPIRV-Tools)")
    return()
endif()

if(NOT spirv_cross_core_FOUND OR NOT spirv_cross_glsl_FOUND)
    message(STATUS "[Lumen] Compiler: skipped (Missing SPIRV-Cross C++ Components)")
    return()
endif()

###########################################################

LumenModule(
    NAME            Compiler
    TYPE            shared_library
    SOURCES         "Private/*.cpp"
                    "Private/**/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Core
                    MathCore
                    RHI
                    Renderer
    LINUX_LIBS      glslang::glslang
                    glslang::glslang-default-resource-limits
                    glslang::SPIRV
                    SPIRV-Tools
                    SPIRV-Tools-opt
                    spirv-cross-core
                    spirv-cross-glsl
)
