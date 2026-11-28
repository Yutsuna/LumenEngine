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

    /**
     * @brief The textual name of a verbosity is obtained with Meta::EnumName( Type ), which uses
     *        C++26 static reflection and needs no hand-maintained mapping here.
     */

} // namespace ELogVerbosity

} // namespace Lumen
