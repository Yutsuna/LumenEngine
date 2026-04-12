###########################################################

function(LumenModule)
    set(MULTI SOURCES PUBLIC_INCLUDES PRIVATE_INCLUDES DEFINES PRIVATE_DEFINES DEPS LINUX_LIBS WINDOWS_LIBS MACOS_LIBS)
    cmake_parse_arguments(M "" "NAME;TYPE" "${MULTI}" ${ARGN})

###########################################################

    set(ALL_SOURCES "")
    foreach(PATTERN IN LISTS M_SOURCES)
        file(GLOB_RECURSE MATCHED CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${PATTERN}")
        list(APPEND ALL_SOURCES ${MATCHED})
    endforeach()

    if(NOT ALL_SOURCES)
        return()
    endif()

###########################################################

    string(TOLOWER "${M_TYPE}" TYPE)

    if(TYPE STREQUAL "executable")
        add_executable(${M_NAME} ${ALL_SOURCES})
        add_executable(Lumen::${M_NAME} ALIAS ${M_NAME})
    elseif(TYPE MATCHES "^static")
        add_library(${M_NAME} STATIC ${ALL_SOURCES})
        add_library(Lumen::${M_NAME} ALIAS ${M_NAME})
    else()
        add_library(${M_NAME} SHARED ${ALL_SOURCES})
        set_target_properties(${M_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)
        add_library(Lumen::${M_NAME} ALIAS ${M_NAME})
    endif()

###########################################################

    target_link_libraries(${M_NAME} PRIVATE Lumen::Compiler)

    set_target_properties(${M_NAME} PROPERTIES 
        OUTPUT_NAME "${M_NAME}" 
        DEBUG_POSTFIX "_d"
    )

###########################################################

    target_include_directories(${M_NAME} PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Public>")
    
    foreach(DIR IN LISTS M_PRIVATE_INCLUDES)
        target_include_directories(${M_NAME} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}")
    endforeach()

###########################################################

    target_compile_definitions(${M_NAME} PUBLIC ${M_DEFINES})
    target_compile_definitions(${M_NAME} PRIVATE ${M_PRIVATE_DEFINES})

###########################################################

    foreach(DEP IN LISTS M_DEPS)
        if(DEP MATCHES "^Lumen::")
            target_link_libraries(${M_NAME} PUBLIC ${DEP})
        else()
            target_link_libraries(${M_NAME} PUBLIC Lumen::${DEP})
        endif()
    endforeach()

###########################################################

    set(PLAT_LIBS "")
    if(APPLE)
        set(PLAT_LIBS ${M_MACOS_LIBS})
    elseif(WIN32)
        set(PLAT_LIBS ${M_WINDOWS_LIBS})
    else()
        set(PLAT_LIBS ${M_LINUX_LIBS})
    endif()

    foreach(LIB IN LISTS PLAT_LIBS)
        target_link_libraries(${M_NAME} PRIVATE ${LIB})
    endforeach()

###########################################################

    if(LUMEN_ENABLE_TESTING)
        
        file(GLOB_RECURSE FUNC_TEST_SOURCES 
            CONFIGURE_DEPENDS 
            "${CMAKE_CURRENT_SOURCE_DIR}/Tests/Functional/*.cpp"
        )
        if(FUNC_TEST_SOURCES)
            LumenAddTestTarget(
                NAME "${M_NAME}_Func"
                MODULE_TARGET ${M_NAME}
                TYPE "Functional"
                SOURCES ${FUNC_TEST_SOURCES}
                DEPS ${M_DEPS}
            )
        endif()

        file(GLOB_RECURSE INTEG_TEST_SOURCES 
            CONFIGURE_DEPENDS 
            "${CMAKE_CURRENT_SOURCE_DIR}/Tests/Integration/*.cpp"
        )
        if(INTEG_TEST_SOURCES)
            LumenAddTestTarget(
                NAME "${M_NAME}_Integ"
                MODULE_TARGET ${M_NAME}
                TYPE "Integration"
                SOURCES ${INTEG_TEST_SOURCES}
                DEPS ${M_DEPS}
            )
        endif()

    endif()

###########################################################

    message(STATUS "[Lumen] Module: ${M_NAME}")

endfunction()

###########################################################
