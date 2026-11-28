/**
 * @file LoggingVerbosity.hpp
 * @brief Definition of the logging verbosity levels.
 */

#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
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
        VeryVerbose,
    };

    /** INFO: Use Meta::EnumName( Type ) for the textual name; reflection removes the need for a hand-maintained mapping. */

} // namespace ELogVerbosity

} // namespace Lumen
