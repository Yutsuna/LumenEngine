###########################################################

function(LumenModule)

    set(MODULE_OPTIONS "")
    set(MODULE_ONE_VALUE NAME TYPE)
    set(MODULE_MULTI_VALUE
        SOURCES
        PUBLIC_INCLUDES
        PRIVATE_INCLUDES
        DEFINES
        PRIVATE_DEFINES
        DEPS
        LINUX_LIBS
        WINDOWS_LIBS
        MACOS_LIBS
    )
    cmake_parse_arguments(M
        "${MODULE_OPTIONS}"
        "${MODULE_ONE_VALUE}"
        "${MODULE_MULTI_VALUE}"
        ${ARGN}
    )

###########################################################

    if(NOT M_NAME)
        message(FATAL_ERROR "[LumenModule] NAME is required")
    endif()

    if(NOT M_TYPE)
        set(M_TYPE "shared_library")
    endif()

###########################################################

    set(ALL_SOURCES "")
    foreach(PATTERN IN LISTS M_SOURCES)
        file(GLOB_RECURSE MATCHED
            CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/${PATTERN}"
        )
        list(APPEND ALL_SOURCES ${MATCHED})
    endforeach()

    if(NOT ALL_SOURCES)
        message(WARNING "[LumenModule] ${M_NAME}: no sources matched, skipping")
        return()
    endif()

###########################################################

    string(TOLOWER "${M_TYPE}" TYPE_LOWER)

    if(TYPE_LOWER STREQUAL "executable")
        add_executable(${M_NAME} ${ALL_SOURCES})
    elseif(TYPE_LOWER MATCHES "^shared")
        add_library(${M_NAME} SHARED ${ALL_SOURCES})
        add_library(Lumen::${M_NAME} ALIAS ${M_NAME})
        set_target_properties(${M_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    elseif(TYPE_LOWER MATCHES "^static")
        add_library(${M_NAME} STATIC ${ALL_SOURCES})
        add_library(Lumen::${M_NAME} ALIAS ${M_NAME})
    else()
        message(FATAL_ERROR "[LumenModule] ${M_NAME}: unknown type '${M_TYPE}'")
    endif()

    set_target_properties(${M_NAME} PROPERTIES
        OUTPUT_NAME  "${M_NAME}"
        DEBUG_POSTFIX "_d"
    )

###########################################################

    target_link_libraries(${M_NAME} PRIVATE Lumen::Compiler)

###########################################################

    foreach(DIR IN LISTS M_PUBLIC_INCLUDES)
        target_include_directories(${M_NAME} PUBLIC
            "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/${DIR}>"
            "$<INSTALL_INTERFACE:include/${M_NAME}>"
        )
    endforeach()

    foreach(DIR IN LISTS M_PRIVATE_INCLUDES)
        target_include_directories(${M_NAME} PRIVATE
            "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}"
        )
    endforeach()

###########################################################

    foreach(DEF IN LISTS M_DEFINES)
        target_compile_definitions(${M_NAME} PUBLIC ${DEF})
    endforeach()

    foreach(DEF IN LISTS M_PRIVATE_DEFINES)
        target_compile_definitions(${M_NAME} PRIVATE ${DEF})
    endforeach()

###########################################################

    foreach(DEP IN LISTS M_DEPS)
        if(DEP MATCHES "^Lumen::")
            target_link_libraries(${M_NAME} PUBLIC ${DEP})
        else()
            target_link_libraries(${M_NAME} PUBLIC Lumen::${DEP})
        endif()
    endforeach()

###########################################################

    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        set(PLATFORM_LIBS ${M_LINUX_LIBS})
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
        set(PLATFORM_LIBS ${M_WINDOWS_LIBS})
    elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
        set(PLATFORM_LIBS ${M_MACOS_LIBS})
    else()
        set(PLATFORM_LIBS "")
    endif()

    foreach(LIB IN LISTS PLATFORM_LIBS)
        LumenResolveLib(${LIB} RESOLVED_TARGET)
        if(RESOLVED_TARGET)
            target_link_libraries(${M_NAME} PRIVATE ${RESOLVED_TARGET})
        else()
            message(WARNING "[LumenModule] ${M_NAME}: '${LIB}' not found, skipping")
        endif()
    endforeach()

###########################################################

    message(STATUS "[Lumen] Module: ${M_NAME} (${M_TYPE})")

endfunction()

###########################################################
