/**
 * @file AppCoreError.hpp
 * @brief Declaration of the EAppCoreError error type for fallible AppCore operations.
 */

#pragma once

#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace EAppCoreError
{

    /**
     * @enum EAppCoreError::Type
     * @brief Errors reported by AppCore operations.
     */
    enum class Type : UInt8
    {
        None = 0,
        WindowCreationFailed
    };

} // namespace EAppCoreError

} // namespace Lumen
