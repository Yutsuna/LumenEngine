/**
 * @file NullMemory.hpp
 * @brief Declares the simulated Null heap.
 */
#pragma once

#include <Memory/UniquePtr.hpp>
#include <RHI/Memory/IRHIHeap.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

namespace Lumen
{

namespace RHI
{

    namespace Tests
    {

        /**
         * @class FNullHeap
         * @brief Heap backed by deterministic CPU storage.
         */
        class FNullHeap final : public IRHIHeap
        {
        public:

            LUMEN_RHI_OBJECT_DECLARATIONS;
            /** @brief Creates a heap from a validated descriptor. */
            FNullHeap ( const FRHIHeapDesc &InDesc, FRHIObjectId InId, FRHIObjectId InOwnerId = {} ) noexcept;
            const FRHIHeapDesc &GetDesc () const noexcept override;
            [[nodiscard]] FRHIObjectId GetOwnerId () const noexcept;

            TExpected<TSpan<Byte>, FRHIError> Map ( UInt64 InOffset, UInt64 InSize ) noexcept override;
            void Unmap () noexcept override
            {
                /* */
            }

        private:

            FRHIObjectBase Identity;
            FRHIObjectId OwnerId;
            FRHIHeapDesc Desc;
            TUniquePtr<Byte[]> Storage;
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
