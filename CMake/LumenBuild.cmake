###########################################################

function(LumenAddModules BASE_DIR)
    foreach(MODULE IN LISTS ARGN)
        add_subdirectory(${BASE_DIR}/${MODULE})
    endforeach()
endfunction()

###########################################################

LumenAddModules(
    ${CMAKE_CURRENT_SOURCE_DIR}/LumenEngine/Source
    Core
    MathCore
    Parallel
    ApplicationCore
    ShaderCompiler
    RHI
    VulkanRHI
    Lumen
    Renderer
    Engine
    Launch
)

LumenAddModules(
    ${CMAKE_CURRENT_SOURCE_DIR}/Examples
    TriangleExample
)

###########################################################
