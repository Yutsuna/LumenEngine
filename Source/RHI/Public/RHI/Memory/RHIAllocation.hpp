/**
 * @file RHIAllocation.hpp
 * @brief Defines a placed memory range.
 */
#pragma once

#include <RHI/Memory/RHIMemoryClass.hpp>
#include <RHI/Types/RHIIdentifiers.hpp>
#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    class IRHIHeap;
    /**
     * @struct FRHIAllocation
     * @brief Non-owning description of an allocated heap range.
     */
    struct RHI_EXPORT FRHIAllocation
    {
        FRHIAllocationId Id;
        UInt64 Offset               = 0ULL;
        UInt64 Size                 = 0ULL;
        UInt64 Alignment            = 1ULL;
        ERHIMemoryClass MemoryClass = ERHIMemoryClass::DeviceLocal;
        IRHIHeap *Heap              = nullptr;
        /** @return True for a nonempty identified range. */
        [[nodiscard]] constexpr Bool IsValid () const noexcept
        {
            return Id.IsValid() and Size > 0ULL;
        }
        /** @return True when the half-open range lies inside this allocation. */
        [[nodiscard]] Bool Contains ( UInt64 InOffset, UInt64 InSize ) const noexcept;
    };
} // namespace RHI

} // namespace Lumen
