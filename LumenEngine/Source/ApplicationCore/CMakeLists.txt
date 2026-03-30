LumenFindPackage(SDL3)

LumenModule(
    NAME            ApplicationCore
    TYPE            shared_library
    SOURCES         "Private/**/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE
    DEPS            Core MathCore
    LINUX_LIBS      SDL3::SDL3
)
