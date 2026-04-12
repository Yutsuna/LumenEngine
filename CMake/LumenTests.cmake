###########################################################

if(NOT LUMEN_ENABLE_TESTING)
    return()
endif()

enable_testing()

###########################################################

find_package(GTest REQUIRED)
include(GoogleTest)

###########################################################

function(LumenAddTestTarget)
    set(options)
    set(oneValueArgs NAME MODULE_TARGET TYPE)
    set(multiValueArgs SOURCES DEPS)
    cmake_parse_arguments(T "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT T_SOURCES)
        return()
    endif()

    set(TEST_EXE_NAME "Test_${T_MODULE_TARGET}_${T_TYPE}")

    add_executable(${TEST_EXE_NAME} ${T_SOURCES})

    target_link_libraries(${TEST_EXE_NAME} 
        PRIVATE 
            Lumen::${T_MODULE_TARGET}
            GTest::gtest
            GTest::gtest_main
            ${T_DEPS}
    )

    if(T_TYPE STREQUAL "Functional")
        target_include_directories(${TEST_EXE_NAME} PRIVATE 
            "${CMAKE_CURRENT_SOURCE_DIR}/Private"
        )
        target_compile_definitions(${TEST_EXE_NAME} PRIVATE LUMEN_FUNCTIONAL_TEST=1)
    endif()

    gtest_discover_tests(${TEST_EXE_NAME}
        WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
        PROPERTIES LABELS "${T_MODULE_TARGET};${T_TYPE}"
    )

    message(STATUS "[Lumen] Added ${T_TYPE} tests for ${T_MODULE_TARGET}")
endfunction()

###########################################################
