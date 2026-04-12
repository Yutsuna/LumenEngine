LumenModule(
    NAME            TriangleExample
    TYPE            executable
    SOURCES         "Private/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Launch ApplicationCore Renderer MathCore RHI
)

LumenCompileShaders(TriangleExample "Shaders"
    SOURCES
    "Private/Shaders/Triangle.vert"
    "Private/Shaders/Triangle.frag"
)
