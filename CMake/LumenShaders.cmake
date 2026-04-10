###########################################################

function(LumenCompileShaders TARGET DEST_DIR)


    set(MULTI SOURCES)


    cmake_parse_arguments(ARG "" "" "${MULTI}" ${ARGN})


    find_program(LUMEN_GLSLC glslc)
    if(NOT LUMEN_GLSLC)
        message(WARNING "[Lumen] glslc not found, skipping shader compilation")
        return()
    endif()


    set(OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/${DEST_DIR}")
    file(MAKE_DIRECTORY "${OUTPUT_DIR}")

    set(COMPILED_SHADERS "")


    foreach(SHADER IN LISTS ARG_SOURCES)
        get_filename_component(SHADER_NAME "${SHADER}" NAME)

        if(IS_ABSOLUTE "${SHADER}")
            set(INPUT_PATH "${SHADER}")
        else()
            set(INPUT_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${SHADER}")
        endif()

        if(NOT EXISTS "${INPUT_PATH}")
            message(FATAL_ERROR "[Lumen] Shader source not found: ${INPUT_PATH}")
        endif()

        set(OUTPUT_PATH "${OUTPUT_DIR}/${SHADER_NAME}.spv")

        add_custom_command(
            OUTPUT "${OUTPUT_PATH}"
            COMMAND ${LUMEN_GLSLC} "${INPUT_PATH}" -o "${OUTPUT_PATH}"
            DEPENDS "${INPUT_PATH}"
            COMMENT "[Lumen] Compiling shader: ${SHADER_NAME} -> ${SHADER_NAME}.spv"
            VERBATIM
        )

        list(APPEND COMPILED_SHADERS "${OUTPUT_PATH}")
    endforeach()


    if(COMPILED_SHADERS)
        set(CUSTOM_TARGET_NAME "${TARGET}_Shaders")
        add_custom_target(${CUSTOM_TARGET_NAME} DEPENDS ${COMPILED_SHADERS})
        add_dependencies(${TARGET} ${CUSTOM_TARGET_NAME})
    endif()


endfunction()

###########################################################
