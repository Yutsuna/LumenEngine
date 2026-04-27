###########################################################

function(LumenRegisterPackage NAME TARGET PKGCONF)
    string(TOUPPER "${NAME}" NAME_UPPER)
    set_property(GLOBAL PROPERTY LUMEN_PKG_${NAME_UPPER}_TARGET  "${TARGET}")
    set_property(GLOBAL PROPERTY LUMEN_PKG_${NAME_UPPER}_PKGCONF "${PKGCONF}")
endfunction()

###########################################################

LumenRegisterPackage(Vulkan                 Vulkan::Vulkan                  "vulkan")
LumenRegisterPackage(SDL3                   SDL3::SDL3                      "sdl3")
LumenRegisterPackage(VulkanMemoryAllocator  GPUOpen::VulkanMemoryAllocator  "")

LumenRegisterPackage(GLSLang                glslang::glslang                "glslang")
LumenRegisterPackage(SPIRV-Tools            SPIRV-Tools-opt                 "SPIRV-Tools")
LumenRegisterPackage(SPIRV-Cross-Core       spirv-cross-core                "spirv-cross-core")
LumenRegisterPackage(SPIRV-Cross-GLSL       spirv-cross-glsl                "spirv-cross-glsl")

###########################################################

function(LumenGetProp NAME KEY OUT_VAR)
    get_property(VALUE GLOBAL PROPERTY LUMEN_PKG_${NAME}_${KEY})
    set(${OUT_VAR} "${VALUE}" PARENT_SCOPE)
endfunction()

###########################################################

function(LumenFindPackage NAME)
    string(TOUPPER "${NAME}" NAME_UPPER)
    LumenGetProp(${NAME_UPPER} TARGET  TARGET_NAME)
    LumenGetProp(${NAME_UPPER} PKGCONF PKG_NAME)

    find_package(${NAME} QUIET CONFIG)

    if(NOT ${NAME}_FOUND)
        find_package(${NAME} QUIET MODULE)
    endif()

    if(NOT ${NAME}_FOUND AND PKG_NAME)
        find_package(PkgConfig QUIET)
        pkg_check_modules(${NAME} QUIET IMPORTED_TARGET "${PKG_NAME}")
        if(${NAME}_FOUND AND NOT TARGET ${TARGET_NAME})
            add_library(${TARGET_NAME} ALIAS PkgConfig::${NAME})
        endif()
    endif()

    if(${NAME}_FOUND)
        message(STATUS "[Lumen] ${NAME}: found")
    else()
        message(STATUS "[Lumen] ${NAME}: not found")
    endif()

    set(${NAME}_FOUND ${${NAME}_FOUND} PARENT_SCOPE)
endfunction()

###########################################################
