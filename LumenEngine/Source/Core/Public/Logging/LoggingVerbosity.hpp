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
        Log,
        /** Blue */
        Verbose,
        /** Cyan */
        VeryVerbose
    };

    static inline constexpr const AnsiChar *const ToString ( const Type InVerbosity ) noexcept
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
        case Log:
            return "Log";
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
