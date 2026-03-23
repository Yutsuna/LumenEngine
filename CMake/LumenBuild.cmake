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
    ApplicationCore
    RHI
    VulkanRHI
    Launch
)

LumenAddModules(
    ${CMAKE_CURRENT_SOURCE_DIR}/Examples/BaseExample
)

###########################################################
