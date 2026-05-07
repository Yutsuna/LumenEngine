LumenModule(
    NAME            TriangleExample
    TYPE            executable
    SOURCES         "Private/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Launch Renderer Engine Compiler
)

###########################################################

target_compile_definitions(
    TriangleExample
    PRIVATE
        LUMEN_EXAMPLE_TRIANGLE_ASSET_PATH="${CMAKE_SOURCE_DIR}/Examples/TriangleExample/Assets"
)
