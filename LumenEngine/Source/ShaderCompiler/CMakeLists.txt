LumenFindPackage(GLSLang)
LumenFindPackage(SPIRV-Tools)
LumenFindPackage(SPIRV-Cross-Core)
LumenFindPackage(SPIRV-Cross-GLSL)

###########################################################

if(NOT GLSLang_FOUND)
    message(STATUS "[Lumen] ShaderCompiler: skipped (Missing GLSLang)")
    return()
endif()

if(NOT SPIRV-Tools_FOUND)
    message(STATUS "[Lumen] ShaderCompiler: skipped (Missing SPIRV-Tools)")
    return()
endif()

if(NOT SPIRV-Cross-Core_FOUND)
    message(STATUS "[Lumen] ShaderCompiler: skipped (Missing SPIRV-Cross-Core)")
    return()
endif()

if(NOT SPIRV-Cross-GLSL_FOUND)
    message(STATUS "[Lumen] ShaderCompiler: skipped (Missing SPIRV-Cross-GLSL)")
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
                    GLSLang::GLSLang
                    SPIRV-Tools::SPIRV-Tools
                    SPIRV-Cross-Core::SPIRV-Cross-Core
                    SPIRV-Cross-GLSL::SPIRV-Cross-GLSL
)
