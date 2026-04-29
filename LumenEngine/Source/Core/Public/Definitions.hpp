/**
 * @file Definitions.hpp
 * @brief General definitions and macros for LumenEngine
 */

#pragma once

#if defined( _WIN32 ) || defined( _WIN64 )
    #define LUMEN_ENGINE_PLATFORM_WINDOWS

#elif defined( __linux__ )
    #define LUMEN_ENGINE_PLATFORM_LINUX

#elif defined( __APPLE__ ) && defined( __MACH__ )
    #define LUMEN_ENGINE_PLATFORM_MACOS

#else
    #define LUMEN_ENGINE_PATH_SEPARATOR "/"
    #define LUMEN_ENGINE_NEWLINE "\n"
    #define LUMEN_ENGINE_SHARED_LIB_EXTENSION ""
#endif

#if defined( LUMEN_ENGINE_PLATFORM_WINDOWS )
    #define LUMEN_ENGINE_PATH_SEPARATOR "\\"
    #define LUMEN_ENGINE_NEWLINE "\r\n"
    #define LUMEN_ENGINE_SHARED_LIB_EXTENSION ".dll"

    #if defined( LUMEN_ENGINE_ENABLE_WINDOWS_DLL )
        #if defined( LUMEN_ENGINE_BUILDING_DLL )
            #define LUMEN_ENGINE_API __declspec( dllexport )
        #else
            #define LUMEN_ENGINE_API __declspec( dllimport )
        #endif
    #else
        #define LUMEN_ENGINE_API
    #endif

#elif defined( LUMEN_ENGINE_PLATFORM_LINUX ) || defined( LUMEN_ENGINE_PLATFORM_MACOS )
    #define LUMEN_ENGINE_PATH_SEPARATOR "/"
    #define LUMEN_ENGINE_NEWLINE "\n"
    #define LUMEN_ENGINE_SHARED_LIB_EXTENSION ".so"
    #define LUMEN_ENGINE_API __attribute__( ( visibility( "default" ) ) )
    #define LUMEN_UNUSED __attribute__( ( unused ) )

#else

#endif

#ifndef LUMEN_ENGINE_API
    #define LUMEN_ENGINE_API
#endif

#if defined( __clang__ )
    #define LUMEN_ENGINE_COMPILER_CLANG

#elif defined( __GNUC__ )
    #define LUMEN_ENGINE_COMPILER_GCC

#endif

#if defined( __clang__ ) || defined( __GNUC__ )
    #define LUMEN_DISABLE_UBSAN __attribute__( ( no_sanitize( "undefined" ) ) )
#else
    #define LUMEN_DISABLE_UBSAN
#endif

#define LUMEN_SHADER_CACHE_MAGIC_NUMBER 0X4C534843 //<< LSHC = Lumen Shader Cache Header
#define LUMEN_ASSET_CACHE_MAGIC_NUMBER 0X4C4D4153  //<< LMAS = Lumen Asset Cache
