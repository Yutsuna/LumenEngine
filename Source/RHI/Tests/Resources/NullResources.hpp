/**
 * @file NullResources.hpp
 * @brief Declares simulated buffers, textures, views, and samplers.
 */
#pragma once

#include <RHI/Memory/RHIAllocation.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/IRHISampler.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/IRHITextureView.hpp>
#include <RHI/Types/RHIObjectForward.hpp>

#include <Memory/UniquePtr.hpp>

namespace Lumen
{

namespace RHI
{
    namespace Tests
    {
        /**
         * @class FNullBuffer
         * @brief CPU-backed buffer with explicit mapping rules and optional placed storage.
         */
        class FNullBuffer final : public IRHIBuffer
        {
        public:

            LUMEN_RHI_OBJECT_DECLARATIONS;
            FNullBuffer ( const FRHIBufferDesc &InDesc,
                          FRHIObjectId InId,
                          TSharedPtr<IRHIHeap> InHeap = {},
                          FRHIAllocation InAllocation = {} ) noexcept;
            const FRHIBufferDesc &GetDesc () const noexcept override;
            TExpected<TSpan<Byte>, FRHIError> Map ( UInt64 InOffset, UInt64 InSize ) noexcept override;
            void Unmap () noexcept override;
            const FRHIResourceRequirements &GetRequirements () const noexcept override;
            ERHIResourceUsage GetUsage () const noexcept override;
            ERHIResourceState GetInitialState () const noexcept override;
            FRHIResourceLifetime GetLifetime () const noexcept override;

        private:

            FRHIObjectBase Identity;
            FRHIBufferDesc Desc;
            FRHIResourceRequirements Requirements;
            TSharedPtr<IRHIHeap> Heap;
            FRHIAllocation Allocation;
            TUniquePtr<Byte[]> Storage;
        };

        /**
         * @class FNullTexture
         * @brief Immutable simulated texture with optional retained heap placement.
         */
        class FNullTexture final : public IRHITexture
        {
        public:

            LUMEN_RHI_OBJECT_DECLARATIONS;
            FNullTexture ( const FRHITextureDesc &InDesc,
                           FRHIResourceRequirements InRequirements,
                           FRHIObjectId InId,
                           TSharedPtr<IRHIHeap> InHeap = {},
                           FRHIAllocation InAllocation = {} ) noexcept;
            const FRHITextureDesc &GetDesc () const noexcept override;
            const FRHIResourceRequirements &GetRequirements () const noexcept override;
            ERHIResourceUsage GetUsage () const noexcept override;
            ERHIResourceState GetInitialState () const noexcept override;
            FRHIResourceLifetime GetLifetime () const noexcept override;

        private:

            FRHIObjectBase Identity;
            FRHITextureDesc Desc;
            FRHIResourceRequirements Requirements;
            TSharedPtr<IRHIHeap> Heap;
            FRHIAllocation Allocation;
        };

        /**
         * @class FNullTextureView
         * @brief View retaining its parent texture.
         */
        class FNullTextureView final : public IRHITextureView
        {
        public:

            LUMEN_RHI_OBJECT_DECLARATIONS;
            FNullTextureView ( const FRHITextureViewDesc &InDesc, FRHIObjectId InId ) noexcept;
            const FRHITextureViewDesc &GetDesc () const noexcept override;
            TSharedPtr<IRHITexture> GetTexture () const noexcept override;

        private:

            FRHIObjectBase Identity;
            FRHITextureViewDesc Desc;
        };

        /**
         * @class FNullSampler
         * @brief Immutable Null sampler.
         */
        class FNullSampler final : public IRHISampler
        {
        public:

            LUMEN_RHI_OBJECT_DECLARATIONS;
            FNullSampler ( const FRHISamplerDesc &InDesc, FRHIObjectId InId ) noexcept;
            const FRHISamplerDesc &GetDesc () const noexcept override;

        private:

            FRHIObjectBase Identity;
            FRHISamplerDesc Desc;
        };
    } // namespace Tests
} // namespace RHI

} // namespace Lumen
