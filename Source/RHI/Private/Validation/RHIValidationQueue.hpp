/**
 * @file RHIValidationQueue.hpp
 * @brief Declares queue submission decoration and timeline checks.
 */

#pragma once

#include "RHIValidationState.hpp"
#include <RHI/Queue/IRHIQueue.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class FRHIValidationQueue
     * @brief Validates command ownership, queue compatibility, and monotonic submissions.
     */
    class FRHIValidationQueue final : public IRHIQueue, public IRHIValidationObject
    {
    public:

        FRHIValidationQueue ( TSharedRef<IRHIQueue> InUnderlying, TSharedPtr<FRHIValidationState> InState ) noexcept;
        ~FRHIValidationQueue () noexcept override;
        [[nodiscard]] FRHIObjectId GetId () const noexcept override;
        [[nodiscard]] FString GetDebugName () const noexcept override;
        void SetDebugName ( FStringView InName ) noexcept override;
        [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;
        [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;
        [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;
        [[nodiscard]] ERHIQueueType GetType () const noexcept override;
        [[nodiscard]] FRHITimelineValue GetCompletedValue () const noexcept override;
        [[nodiscard]] FRHIExpected Submit ( const FRHISubmitDesc &InDesc ) noexcept override;
        [[nodiscard]] FRHIExpected WaitIdle () noexcept override;
        [[nodiscard]] FRHIExpected Wait ( FRHITimelineValue InValue, Float64 InTimeoutSeconds ) noexcept override;

    private:

        TSharedRef<IRHIQueue> Underlying;
        TSharedPtr<FRHIValidationState> State;
    };

} // namespace RHI

} // namespace Lumen
