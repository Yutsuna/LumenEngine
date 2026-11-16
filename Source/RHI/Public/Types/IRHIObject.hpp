/**
 * @file IRHIObject.hpp
 * @brief Declares the common interface of every RHI object.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <String/String.hpp>
#include <String/StringView.hpp>
#include <Types/RHIIdentifiers.hpp>

namespace Lumen
{

/**
 * @class IRHIObject
 * @brief Identity, debug naming, and backend type contract.
 */
class IRHIObject
{
public:

    /** @brief Allows polymorphic destruction without exceptions. */
    virtual ~IRHIObject () noexcept = default;

    /** @return Stable device-scoped object identifier. */
    [[nodiscard]] virtual FRHIObjectId GetId () const noexcept = 0;
    /** @return Owned copy of the current debug name. */
    [[nodiscard]] virtual FString GetDebugName () const noexcept = 0;
    /** @brief Replaces the debug name; safe on shared objects. */
    virtual void SetDebugName ( FStringView InName ) noexcept = 0;
    /** @return Concrete backend type tag, normally FHash::TypeHash<T>(). */
    [[nodiscard]] virtual UInt64 GetBackendTypeTag () const noexcept = 0;
};

} // namespace Lumen
