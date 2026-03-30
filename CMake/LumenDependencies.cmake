###########################################################

function(LumenRegisterPackage NAME TARGET PKGCONF)
    set_property(GLOBAL PROPERTY LUMEN_PKG_${NAME}_TARGET  "${TARGET}")
    set_property(GLOBAL PROPERTY LUMEN_PKG_${NAME}_PKGCONF "${PKGCONF}")
endfunction()

LumenRegisterPackage(Vulkan Vulkan::Vulkan "")
LumenRegisterPackage(SDL3   SDL3::SDL3      sdl3)
LumenRegisterPackage(VulkanMemoryAllocator VulkanMemoryAllocator::VMA "") 

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
        if(PkgConfig_FOUND)
            pkg_check_modules(${NAME} QUIET IMPORTED_TARGET "${PKG_NAME}")

            if(${NAME}_FOUND AND TARGET_NAME AND NOT TARGET ${TARGET_NAME})
                add_library(LUMEN_${NAME} INTERFACE)
                target_link_libraries(LUMEN_${NAME}
                    INTERFACE PkgConfig::${NAME}
                )
                add_library(${TARGET_NAME} ALIAS LUMEN_${NAME})
            endif()
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

function(LumenResolveLib RAW_NAME OUT_TARGET)
    string(TOUPPER "${RAW_NAME}" NAME_UPPER)

    LumenGetProp(${NAME_UPPER} TARGET TARGET_NAME)

    if(TARGET "${TARGET_NAME}")
        set(${OUT_TARGET} "${TARGET_NAME}" PARENT_SCOPE)
    else()
        set(${OUT_TARGET} "${RAW_NAME}" PARENT_SCOPE)
    endif()
endfunction()
