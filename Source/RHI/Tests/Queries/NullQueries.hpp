/**
 * @file NullQueries.hpp
 * @brief Declares deterministic Null query pools.
 */
#pragma once

#include <Device/NullFaultPlan.hpp>
#include <HAL/PlatformMutex.hpp>
#include <Memory/SharedPtr.hpp>
#include <RHI/Queries/IRHIQueryPool.hpp>
#include <RHI/Types/RHIObjectForward.hpp>
#include <Templates/Array.hpp>

namespace Lumen
{

namespace RHI
{

    namespace Tests
    {

        /**
         * @class FNullQueryPool
         * @brief Query storage with controllable values and availability timelines.
         */
        class FNullQueryPool final : public IRHIQueryPool
        {
        public:

            FNullQueryPool ( const FRHIQueryPoolDesc &InDesc,
                             TSharedPtr<FNullFaultPlan> InFaultPlan,
                             FRHIObjectId InId ) noexcept;
            LUMEN_RHI_OBJECT_DECLARATIONS;

            const FRHIQueryPoolDesc &GetDesc () const noexcept override;
            void Reset () noexcept override;
            FRHIExpected ReadResults ( UInt32 InFirstQuery,
                                       UInt32 InQueryCount,
                                       TSpan<UInt64> InDestination,
                                       FRHITimelineValue InCompleted ) const noexcept override;

            [[nodiscard]] FRHIExpected Begin ( UInt32 InIndex ) noexcept;
            [[nodiscard]] FRHIExpected End ( UInt32 InIndex ) noexcept;
            void SetNextValue ( UInt64 InValue ) noexcept;
            void SetNextAvailability ( FRHITimelineValue InValue ) noexcept;
            [[nodiscard]] FRHIExpected
            SetQueryValue ( UInt32 InIndex, UInt64 InValue, FRHITimelineValue InAvailability = {} ) noexcept;
            [[nodiscard]] Bool IsActive ( UInt32 InIndex ) const noexcept;

        private:

            struct FEntry
            {
                TArray<UInt64, 11U> Values = {};
                FRHITimelineValue Availability;
                Bool bAvailable = false;
                Bool bActive    = false;
            };

            [[nodiscard]] TOptional<FRHIError> CheckFault ( FStringView InOperation ) noexcept;

        private:

            FRHIObjectBase Identity;
            FRHIQueryPoolDesc Desc;
            TSharedPtr<FNullFaultPlan> FaultPlan;
            TVector<FEntry> Entries;
            UInt64 NextValue = 1ULL;
            FRHITimelineValue NextAvailability;
            mutable HAL::FPlatformRecursiveSharedMutex Mutex;
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
