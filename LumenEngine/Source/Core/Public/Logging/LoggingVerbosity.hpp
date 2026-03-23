/**
 * @file LoggingVerbosity.hpp
 * @brief Definition of the logging verbosity levels.
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace ELogVerbosity
{

    enum Type : UInt8
    {
        /** Crashes the engine immediately after logging the message. */
        Fatal,
        /** Red */
        Error,
        /** Yellow */
        Warning,
        /** White */
        Display,
        /** Grey */
        Info,
        /** Blue */
        Verbose,
        /** Cyan */
        VeryVerbose
    };

    static inline constexpr const AnsiChar *ToColor ( const Type InVerbosity ) noexcept
    {
        switch ( InVerbosity )
        {
        case Fatal:
            return "\033[31m";
        case Error:
            return "\033[31m";
        case Warning:
            return "\033[33m";
        case Display:
            return "\033[37m";
        case Info:
            return "\033[90m";
        case Verbose:
            return "\033[34m";
        case VeryVerbose:
            return "\033[36m";
        default:
            return "\033[0m";
        }
    }

    static inline constexpr const AnsiChar *ToString ( const Type InVerbosity ) noexcept
    {
        switch ( InVerbosity )
        {
        case Fatal:
            return "Fatal";
        case Error:
            return "Error";
        case Warning:
            return "Warning";
        case Display:
            return "Display";
        case Info:
            return "Info";
        case Verbose:
            return "Verbose";
        case VeryVerbose:
            return "VeryVerbose";
        default:
            return "Unknown";
        }
    }

} // namespace ELogVerbosity

} // namespace LumenEngine
