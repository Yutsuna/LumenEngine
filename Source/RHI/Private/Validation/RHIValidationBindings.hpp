/**
 * @file RHIValidationBindings.hpp
 * @brief Declares binding layout/set decorators and typed write checks.
 */

#pragma once

#include "RHIValidationState.hpp"
#include <Bindings/IRHIBindingSet.hpp>

namespace Lumen
{

/**
 * @class FRHIValidationBindingLayout
 * @brief Tracks an immutable backend layout.
 */
class FRHIValidationBindingLayout final : public IRHIBindingLayout, public IRHIValidationObject
{
public:

    FRHIValidationBindingLayout ( TSharedRef<IRHIBindingLayout> InUnderlying, TSharedPtr<FRHIValidationState> InState ) noexcept;
    ~FRHIValidationBindingLayout () noexcept override;
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    [[nodiscard]] FString GetDebugName () const noexcept override;
    void SetDebugName ( FStringView InName ) noexcept override;
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
    [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
    [[nodiscard]] const FRHIBindingLayoutDesc &GetDesc () const noexcept override;

private:

    TSharedRef<IRHIBindingLayout> Underlying;
    TSharedPtr<FRHIValidationState> State;
};

/**
 * @class FRHIValidationBindingSet
 * @brief Validates layout, resource category, usage, and range before forwarding writes.
 */
class FRHIValidationBindingSet final : public IRHIBindingSet, public IRHIValidationObject
{
public:

    FRHIValidationBindingSet ( TSharedRef<IRHIBindingSet> InUnderlying,
                               TSharedRef<IRHIBindingLayout> InLayout,
                               TSharedPtr<FRHIValidationState> InState ) noexcept;
    ~FRHIValidationBindingSet () noexcept override;
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    [[nodiscard]] FString GetDebugName () const noexcept override;
    void SetDebugName ( FStringView InName ) noexcept override;
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
    [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
    [[nodiscard]] TSharedRef<IRHIBindingLayout> GetLayout () const noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> Write ( TSpan<const FRHIBindingWrite> InWrites ) noexcept override;
    [[nodiscard]] Bool IsDirty () const noexcept override;
    void ClearDirty () noexcept override;

private:

    [[nodiscard]] TExpected<void, FRHIError> ValidateWrite ( const FRHIBindingWrite &InWrite ) const noexcept;

private:

    TSharedRef<IRHIBindingSet> Underlying;
    TSharedRef<IRHIBindingLayout> Layout;
    TSharedPtr<FRHIValidationState> State;
};

} // namespace Lumen
