/**
 * @file RHIValidationTransientResourceAllocator.hpp
 * @brief Declares transient batch and allocation-lifetime validation.
 */
#pragma once

#include "RHIValidationState.hpp"
#include <Memory/IRHITransientResourceAllocator.hpp>

namespace Lumen
{

/**
 * @class FRHIValidationTransientResourceAllocator
 * @brief Checks batch scopes, aliasing, and use after batch completion.
 */
class RHI_EXPORT FRHIValidationTransientResourceAllocator final : public IRHITransientResourceAllocator,
                                                                  public IRHIValidationObject
{
public:

    FRHIValidationTransientResourceAllocator ( TSharedRef<IRHITransientResourceAllocator> InUnderlying,
                                               TSharedPtr<FRHIValidationState> InState ) noexcept;
    ~FRHIValidationTransientResourceAllocator () noexcept override;
    [[nodiscard]] FRHIObjectId GetId () const noexcept override;
    [[nodiscard]] FString GetDebugName () const noexcept override;
    void SetDebugName ( FStringView InName ) noexcept override;
    [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
    [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
    [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
    void BeginBatch () noexcept override;
    [[nodiscard]] TExpected<TVector<FRHITransientAllocation>, FRHIError>
    Allocate ( const FRHITransientBatch &InBatch ) noexcept override;
    void EndBatch () noexcept override;

    /** @return Success only while the allocation is active and InFrame lies in its half-open lifetime. */
    [[nodiscard]] TExpected<void, FRHIError> ValidateResourceUse ( const FRHITransientAllocation &InAllocation,
                                                                   UInt64 InFrame ) const noexcept;

private:

    TSharedRef<IRHITransientResourceAllocator> Underlying;
    TSharedPtr<FRHIValidationState> State;
    TVector<FRHITransientAllocation> ActiveAllocations;
    Bool bInBatch = false;
};

} // namespace Lumen
