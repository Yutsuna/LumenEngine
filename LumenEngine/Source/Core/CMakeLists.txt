LumenModule(
    NAME            Core
    TYPE            shared_library
    SOURCES         "Private/**/*.cpp"
    PUBLIC_INCLUDES "Public"
    DEFINES         LUMEN_ENGINE LUMEN_SHOW_LOG_SHADER_TRACE
)
