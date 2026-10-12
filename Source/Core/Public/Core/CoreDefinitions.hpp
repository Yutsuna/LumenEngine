///
/// @file CoreDefinitions.hpp
/// @brief Macros and platform definitions
///

#pragma once

#if defined( _WIN32 ) || defined( _WIN64 )
    #define LUMEN_PLATFORM_WINDOWS

#elif defined( __linux__ )
    #define LUMEN_PLATFORM_LINUX

#elif defined( __APPLE__ ) && defined( __MACH__ )
    #define LUMEN_PLATFORM_MACOS

#else
    #define LUMEN_PATH_SEPARATOR "/"
    #define LUMEN_NEWLINE "\n"
    #define LUMEN_SHARED_LIB_EXTENSION ""
#endif

#if defined( LUMEN_PLATFORM_WINDOWS )
    #define LUMEN_PATH_SEPARATOR "\\"
    #define LUMEN_NEWLINE "\r\n"
    #define LUMEN_SHARED_LIB_EXTENSION ".dll"

#elif defined( LUMEN_PLATFORM_LINUX ) || defined( LUMEN_PLATFORM_MACOS )
    #define LUMEN_PATH_SEPARATOR "/"
    #define LUMEN_NEWLINE "\n"
    #define LUMEN_SHARED_LIB_EXTENSION ".so"
    #define LUMEN_UNUSED __attribute__( ( unused ) )

#else

#endif

#if defined( __clang__ )
    #define LUMEN_COMPILER_CLANG

#elif defined( __GNUC__ )
    #define LUMEN_COMPILER_GCC

#endif

#if defined( __clang__ ) || defined( __GNUC__ )
    #define LUMEN_DISABLE_UBSAN __attribute__( ( no_sanitize( "undefined" ) ) )
#else
    #define LUMEN_DISABLE_UBSAN
#endif

#define LUMEN_SHADER_CACHE_MAGIC_NUMBER 0X4C534843 //<< LSHC = Lumen Shader Cache Header
#define LUMEN_ASSET_CACHE_MAGIC_NUMBER 0X4C4D4153  //<< LMAS = Lumen Asset Cache
