/**
 * @file ErrorCodes.hpp
 * @brief Common error codes used across the engine
 */

#pragma once

#include "CoreTypes.hpp"

namespace LumenEngine
{

namespace EErrorCode
{

    /**
     * @brief Enumeration of common error codes used across the engine
     */
    // NOLINTNEXTLINE(performance-enum-size)
    enum Type : Int32
    {
        /** No error, operation succeeded */
        Success = 0,
        /** Generic failure, unspecified error */
        Failure = 1,
        /** Invalid argument provided to a function */
        InvalidArgument = 2,
        /** Resource not found (e.g., file, asset) */
        NotFound = 3,
        /** Operation timed out */
        Timeout = 4,
        /** Permission denied for the requested operation */
        PermissionDenied = 5,
        /** Out of memory or insufficient resources */
        OutOfMemory = 6,
        /** Unsupported operation or feature */
        Unsupported = 7,
        /** Operation was cancelled by the user or system */
        Cancelled = 8,
    };

} // namespace EErrorCode

} // namespace LumenEngine
