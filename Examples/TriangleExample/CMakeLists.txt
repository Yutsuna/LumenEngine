LumenModule(
    NAME            TriangleExample
    TYPE            executable
    SOURCES         "Private/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Launch ApplicationCore Renderer MathCore Engine
)

###########################################################

target_compile_definitions(
    TriangleExample
    PRIVATE
        LUMEN_EXAMPLE_TRIANGLE_SHADER_PATH="${CMAKE_SOURCE_DIR}/Examples/TriangleExample/Shaders/Triangle"
)
