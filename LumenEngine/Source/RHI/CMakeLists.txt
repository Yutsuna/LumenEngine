LumenModule(
    NAME            RHI
    TYPE            shared_library
    SOURCES         "Private/**/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Core MathCore
)

