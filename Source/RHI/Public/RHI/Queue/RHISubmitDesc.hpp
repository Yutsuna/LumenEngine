/**
 * @file RHISubmitDesc.hpp
 * @brief Defines command, semaphore, and completion submission data.
 */

#pragma once

#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Synchronization/IRHISemaphore.hpp>
#include <RHIExport.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

namespace RHI
{

    struct FRHISemaphoreOperation
    {
        TSharedPtr<IRHISemaphore> Semaphore;
        UInt64 Value = 0ULL;
    };

    struct RHI_EXPORT FRHISubmitDesc
    {
        TVector<TSharedRef<IRHICommandList>> CommandLists;
        TVector<FRHISemaphoreOperation> Waits;
        TVector<FRHISemaphoreOperation> Signals;
        FRHITimelineValue Completion;

        /**
         * @brief Validates command lists, semaphore operations and completion value.
         * @return Success, or a structured FRHIError with the relevant condition.
         */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };

} // namespace RHI

} // namespace Lumen
