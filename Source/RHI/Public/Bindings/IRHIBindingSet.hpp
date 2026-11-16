/**
 * @file IRHIBindingSet.hpp
 * @brief Declares mutable resources conforming to an immutable layout.
 */
#pragma once
#include <Bindings/IRHIBindingLayout.hpp>
#include <Bindings/RHIBindingWrite.hpp>
#include <Memory/Span.hpp>
namespace Lumen
{

/**
 * @class IRHIBindingSet
 * @brief Binding instance with explicit dirty tracking.
 */
class IRHIBindingSet : public IRHIObject
{
public:

    /** @return Layout accepted by Write. */
    [[nodiscard]] virtual TSharedRef<IRHIBindingLayout> GetLayout () const noexcept = 0;
    /** @brief Validates and applies all writes atomically. */
    [[nodiscard]] virtual TExpected<void, FRHIError> Write ( TSpan<const FRHIBindingWrite> InWrites ) noexcept = 0;
    /** @return True when writes occurred since ClearDirty. */
    [[nodiscard]] virtual Bool IsDirty () const noexcept = 0;
    /** @brief Clears the dirty flag after backend synchronization. */
    virtual void ClearDirty () noexcept = 0;
};
} // namespace Lumen
