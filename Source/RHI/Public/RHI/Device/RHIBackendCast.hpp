/**
 * @file RHIBackendCast.hpp
 * @brief Declares checked casts to concrete backend objects.
 */
#pragma once

#include <Memory/SharedPtr.hpp>
#include <RHI/Types/IRHIObject.hpp>
#include <Templates/Hash.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHI
{

    /** @return An optional pointer to BackendType when the object's concrete tag matches, otherwise empty. */
    template <typename BackendType> [[nodiscard]] TOptional<BackendType *> TryRHIBackendCast ( IRHIObject &InObject ) noexcept;

    /**
     * @brief Casts a shared reference to a backend type after checking its tag.
     * @param InObject Object whose backend type tag is checked.
     * @return A shared pointer to BackendType when the tag matches, otherwise empty.
     */
    template <typename BackendType>
    [[nodiscard]] TOptional<TSharedPtr<BackendType>> TryRHIBackendCast ( const TSharedRef<IRHIObject> &InObject ) noexcept;

} // namespace RHI

} // namespace Lumen

#include "Inline/RHIBackendCast.inl"
