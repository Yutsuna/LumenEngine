/**
 * @file RHIMemoryBudget.hpp
 * @brief Declares a thread-safe byte budget.
 */
#pragma once

#include <Core/CoreTypes.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class FRHIMemoryBudget
     * @brief Atomic admission and accounting for one memory category.
     */
    class RHI_EXPORT FRHIMemoryBudget final
    {
    public:

        /** @brief Creates a budget with InCapacity bytes. */
        explicit FRHIMemoryBudget ( UInt64 InCapacity ) noexcept;
        /** @return True and reserves bytes when capacity permits. */
        [[nodiscard]] Bool TryReserve ( UInt64 InSize ) noexcept;
        /** @brief Releases a previously reserved byte count. */
        void Release ( UInt64 InSize ) noexcept;
        /** @return Currently reserved bytes. */
        [[nodiscard]] UInt64 GetUsed () const noexcept;
        /** @return Maximum bytes. */
        [[nodiscard]] constexpr UInt64 GetCapacity () const noexcept
        {
            return Capacity;
        }

    private:

        UInt64 Capacity;
        TAtomic<UInt64> Used;
    };
} // namespace RHI

} // namespace Lumen
