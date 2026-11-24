/**
 * @file IRHIAllocator.hpp
 * @brief Declares the address allocator SPI.
 */
#pragma once

#include <RHI/Memory/RHIAllocation.hpp>
#include <RHI/Memory/RHIAllocationDesc.hpp>
#include <RHI/Memory/RHIAllocatorStats.hpp>
#include <Templates/Expected.hpp>

#include <RHIExport.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class IRHIAllocator
     * @brief Backend-independent address allocation contract.
     */
    class RHI_EXPORT IRHIAllocator
    {
    public:

        virtual ~IRHIAllocator () noexcept = default;

        /** @brief Allocates a non-overlapping range satisfying InDesc. */
        [[nodiscard]] virtual TExpected<FRHIAllocation, FRHIError> Allocate ( const FRHIAllocationDesc &InDesc ) noexcept = 0;

        /** @brief Releases a range returned by this allocator. */
        [[nodiscard]] virtual FRHIExpected Free ( const FRHIAllocation &InAllocation ) noexcept = 0;

        /** @brief Releases all ranges where the allocator supports global reset. */
        virtual void Reset () noexcept = 0;

        /** @return Current statistics snapshot. */
        [[nodiscard]] virtual FRHIAllocatorStats GetStats () const noexcept = 0;
    };

} // namespace RHI

} // namespace Lumen
