/**
 * @file NullBindings.hpp
 * @brief Declares Null binding and descriptor objects.
 */

#pragma once
#include "Types/RHIObjectForward.hpp"
#include <HAL/PlatformMutex.hpp>
#include <RHI/Bindings/IRHIBindingSet.hpp>
#include <RHI/Bindings/RHIDescriptorAllocatorBase.hpp>

namespace Lumen
{

namespace RHI
{

    namespace Tests
    {

        /**
         * @class FNullBindingLayout
         * @brief Immutable Null layout.
         */
        class FNullBindingLayout final : public IRHIBindingLayout
        {
        public:

            FNullBindingLayout ( const FRHIBindingLayoutDesc &InDesc, FRHIObjectId InId ) noexcept;
            LUMEN_RHI_OBJECT_DECLARATIONS;
            const FRHIBindingLayoutDesc &GetDesc () const noexcept override;

        private:

            FRHIObjectBase Identity;
            FRHIBindingLayoutDesc Desc;
        };

        /**
         * @class FNullBindingSet
         * @brief Null set storing validated writes and a dirty flag.
         */
        class FNullBindingSet final : public IRHIBindingSet
        {
        public:

            FNullBindingSet ( TSharedRef<IRHIBindingLayout> InLayout, FRHIObjectId InId ) noexcept;
            LUMEN_RHI_OBJECT_DECLARATIONS;
            TSharedRef<IRHIBindingLayout> GetLayout () const noexcept override;
            TExpected<void, FRHIError> Write ( TSpan<const FRHIBindingWrite> InWrites ) noexcept override;
            Bool IsDirty () const noexcept override;
            void ClearDirty () noexcept override;

        private:

            FRHIObjectBase Identity;
            TSharedRef<IRHIBindingLayout> Layout;
            TVector<FRHIBindingWrite> Writes;
            Bool bDirty = false;
            mutable HAL::FPlatformRecursiveSharedMutex Mutex;
        };

        /**
         * @class FNullDescriptorPoolBackend
         * @brief Deterministic native-pool token provider.
         */
        class FNullDescriptorPoolBackend final : public IRHIDescriptorPoolBackend
        {
        public:

            FNullDescriptorPoolBackend () noexcept;
            TExpected<UInt64, FRHIError> CreatePool ( UInt32 InCapacity ) noexcept override;
            void ReleasePool ( UInt64 InPool ) noexcept override;

        private:

            UInt64 NextPool = 1ULL;
        };

        /**
         * @class FNullDescriptorAllocator
         * @brief Descriptor allocator using the public common base.
         */
        class FNullDescriptorAllocator final : public FRHIDescriptorAllocatorBase
        {
        public:

            FNullDescriptorAllocator ( const FRHIDescriptorAllocatorDesc &InDesc,
                                       IRHIDescriptorPoolBackend &InBackend,
                                       FRHIObjectId InId ) noexcept;
            LUMEN_RHI_OBJECT_DECLARATIONS;

        private:

            FRHIObjectBase Identity;
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
