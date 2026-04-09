LumenModule(
    NAME            BaseExample
    TYPE            executable
    SOURCES         "Private/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Launch ApplicationCore Renderer
)

LumenCompileShaders(BaseExample "Shaders"
    SOURCES
    "Private/Shaders/Triangle.vert"
    "Private/Shaders/Triangle.frag"
)