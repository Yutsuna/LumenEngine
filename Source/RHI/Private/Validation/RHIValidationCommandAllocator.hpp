/**
 * @file RHIValidationCommandAllocator.hpp
 * @brief Declares command allocator decoration and pending-list recycling checks.
 */

#pragma once

#include "RHIValidationState.hpp"
#include <RHI/Commands/IRHICommandAllocator.hpp>

namespace Lumen
{

namespace RHI
{

    class FRHIValidationCommandList;

    /**
     * @class FRHIValidationCommandAllocator
     * @brief Tracks decorated lists while recycling the underlying allocator.
     */
    class FRHIValidationCommandAllocator final : public IRHICommandAllocator, public IRHIValidationObject
    {
    public:

        FRHIValidationCommandAllocator ( TSharedRef<IRHICommandAllocator> InUnderlying,
                                         TSharedPtr<FRHIValidationState> InState ) noexcept;

        ~FRHIValidationCommandAllocator () noexcept override;

    public:

        [[nodiscard]] FRHIObjectId GetId () const noexcept override;

        [[nodiscard]] FString GetDebugName () const noexcept override;

        void SetDebugName ( FStringView InName ) noexcept override;

        [[nodiscard]] UInt64 GetBackendTypeTag () const noexcept override;

        [[nodiscard]] IRHIObject &GetValidationWrappedObject () const noexcept override;

        [[nodiscard]] TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept override;

        [[nodiscard]] ERHIQueueType GetQueueType () const noexcept override;

        [[nodiscard]] FRHIExpected Reset ( FRHITimelineValue InCompleted ) noexcept override;

        /** @brief Adds/removes a decorator whose underlying list belongs to this allocator. */
        void RegisterList ( FRHIValidationCommandList &InList ) noexcept;

        void UnregisterList ( FRHIValidationCommandList &InList ) noexcept;

        [[nodiscard]] USize GetTrackedListCount () const noexcept;

    private:

        TSharedRef<IRHICommandAllocator> Underlying;
        TSharedPtr<FRHIValidationState> State;
        TVector<FRHIValidationCommandList *> Lists;
        mutable HAL::FPlatformRecursiveSharedMutex Mutex;
    };

} // namespace RHI

} // namespace Lumen
