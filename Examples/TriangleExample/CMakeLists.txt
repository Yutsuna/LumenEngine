LumenModule(
    NAME            TriangleExample
    TYPE            executable
    SOURCES         "Private/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Launch ApplicationCore Renderer MathCore Engine
)

LumenCompileShaders(TriangleExample "Shaders"
    SOURCES
    "Shaders/Triangle.vert"
    "Shaders/Triangle.frag"
)
