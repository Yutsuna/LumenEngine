/**
 * @file RHIValidation.hpp
 * @brief Declares validation device decoration and safe backend unwrapping.
 */

#pragma once

#include <Device/IRHIDevice.hpp>
#include <RHIExport.hpp>
#include <Templates/Optional.hpp>
#include <Validation/RHIValidationDesc.hpp>

namespace Lumen
{

/**
 * Wrap
 */

/**
 * @brief Wraps a conforming device and its validated children.
 * @return The original device when validation is disabled, or a decorator preserving backend failures.
 */
[[nodiscard]] RHI_EXPORT TExpected<TSharedRef<IRHIDevice>, FRHIError>
WrapRHIDevice ( const TSharedRef<IRHIDevice> &InDevice, const FRHIValidationDesc &InDesc = {} ) noexcept;

/**
 * Unwarp
 */

/** @return The innermost backend object, or null for a null input. */
[[nodiscard]] RHI_EXPORT IRHIObject *UnwrapRHIObject ( IRHIObject *InObject ) noexcept;

/** @return The innermost const backend object, or null for a null input. */
[[nodiscard]] RHI_EXPORT const IRHIObject *UnwrapRHIObject ( const IRHIObject *InObject ) noexcept;

/** @return A shared reference to the innermost backend object with unchanged ownership. */
[[nodiscard]] RHI_EXPORT TSharedRef<IRHIObject> UnwrapRHISharedObject ( const TSharedRef<IRHIObject> &InObject ) noexcept;

/** @return A shared pointer to the innermost backend object, or null for a null input. */
[[nodiscard]] RHI_EXPORT TSharedPtr<IRHIObject> UnwrapRHISharedObject ( const TSharedPtr<IRHIObject> &InObject ) noexcept;

/**
 * Unwrap template
 */

/** @return The innermost object with the same static interface type. */
template <typename ObjectType> [[nodiscard]] ObjectType *UnwrapRHI ( ObjectType *InObject ) noexcept;

/** @return The innermost const object with the same static interface type. */
template <typename ObjectType> [[nodiscard]] const ObjectType *UnwrapRHI ( const ObjectType *InObject ) noexcept;

/** @return The innermost object; the supplied shared reference retains the decorator chain. */
template <typename ObjectType> [[nodiscard]] ObjectType *UnwrapRHI ( const TSharedRef<ObjectType> &InObject ) noexcept;

/** @return A recursively unwrapped shared reference with unchanged ownership. */
template <typename ObjectType>
[[nodiscard]] TSharedRef<ObjectType> UnwrapRHIShared ( const TSharedRef<ObjectType> &InObject ) noexcept;

/** @return A recursively unwrapped shared pointer with unchanged ownership. */
template <typename ObjectType>
[[nodiscard]] TSharedPtr<ObjectType> UnwrapRHIShared ( const TSharedPtr<ObjectType> &InObject ) noexcept;

/** @return The concrete backend after recursive unwrapping and backend-tag verification, or empty when either check fails. */
template <typename BackendType> [[nodiscard]] TOptional<BackendType *> TryUnwrapRHIBackend ( IRHIObject *InObject ) noexcept;

} // namespace Lumen

#include "Inline/RHIValidation.inl"
