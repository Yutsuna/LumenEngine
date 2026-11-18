/**
 * @file RHIValidationDefragmenter.hpp
 * @brief Declares defragmentation plan validation.
 */
#pragma once

#include "RHIValidationState.hpp"
#include <Memory/IRHIDefragmenter.hpp>

namespace Lumen
{

/**
 * @class FRHIValidationDefragmenter
 * @brief Validates defragmentation plans before scheduling them on the backend.
 */
class RHI_EXPORT FRHIValidationDefragmenter final : public IRHIDefragmenter, public IRHIValidationObject
{
public:

    FRHIValidationDefragmenter ( TSharedRef<IRHIDefragmenter> InUnderlying, TSharedPtr<FRHIValidationState> InState ) noexcept;
    ~FRHIValidationDefragmenter () noexcept override;
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    [[nodiscard]] FString GetDebugName () const noexcept override;
    void SetDebugName ( FStringView InName ) noexcept override;
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
    [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
    [[nodiscard]] TExpected<FRHIDefragPlan, FRHIError> BuildPlan ( TSpan<const FRHIAllocation> InAllocations ) noexcept override;
    [[nodiscard]] TExpected<void, FRHIError> Execute ( const FRHIDefragPlan &InPlan ) noexcept override;

private:

    TSharedRef<IRHIDefragmenter> Underlying;
    TSharedPtr<FRHIValidationState> State;
};

} // namespace Lumen
