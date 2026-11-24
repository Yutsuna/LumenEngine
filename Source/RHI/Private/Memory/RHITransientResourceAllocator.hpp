/**
 * @file RHITransientResourceAllocator.hpp
 * @brief Declares the transient batch allocator implementation.
 */
#pragma once

#include <RHI/Memory/IRHITransientResourceAllocator.hpp>
#include <RHI/Memory/RHITlsfAllocator.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

namespace Lumen
{

namespace RHI
{

    /**
     * @class FRHITransientResourceAllocator
     * @brief Places batches and aliases disjoint lifetimes.
     */
    class RHI_EXPORT FRHITransientResourceAllocator final : public IRHITransientResourceAllocator
    {
    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        /** @brief Creates a transient arena with InCapacity bytes. */
        explicit FRHITransientResourceAllocator ( FRHIObjectId InId, UInt64 InCapacity = 64ULL * 1024ULL * 1024ULL ) noexcept;

        void BeginBatch () noexcept override;

        TExpected<TVector<FRHITransientAllocation>, FRHIError> Allocate ( const FRHITransientBatch &InBatch ) noexcept override;

        void EndBatch () noexcept override;

    private:

        FRHIObjectBase Identity;
        FRHITlsfAllocator Allocator;
        Bool bInBatch = false;
    };

} // namespace RHI

} // namespace Lumen
