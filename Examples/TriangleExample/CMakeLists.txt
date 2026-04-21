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
    "Assets/Shaders/Triangle.vert"
    "Assets/Shaders/Triangle.frag"
)
