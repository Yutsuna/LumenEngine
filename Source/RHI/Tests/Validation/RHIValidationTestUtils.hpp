/**
 * @file RHIValidationTestUtils.hpp
 * @brief Provides deterministic validated Null objects for validation tests.
 */
#pragma once

#include <Debug/RecordingDebugSink.hpp>
#include <Device/NullDevice.hpp>
#include <RHI/Presentation/RHISwapchainDesc.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Validation/RHIValidation.hpp>

namespace Lumen
{

namespace RHI
{

    namespace Tests
    {

        namespace Validation
        {
            inline TSharedRef<IRHIDevice> MakeDevice ( IRHIDebugSink &Sink, FRHIValidationDesc Desc = {} ) noexcept
            {
                Desc.Sink                      = &Sink;
                TSharedRef<IRHIDevice> Backend = MakeShared<FNullDevice>();
                return *WrapRHIDevice( Backend, Desc );
            }

            inline TSharedRef<IRHIBuffer>
            MakeBuffer ( const TSharedRef<IRHIDevice> &Device,
                         const UInt64 InSize                    = 64ULL,
                         const ERHIResourceUsage InUsage        = ERHIResourceUsage::ShaderResource,
                         const ERHIResourceState InInitialState = ERHIResourceState::Common ) noexcept
            {
                FRHIBufferDesc Desc;
                Desc.Size         = InSize;
                Desc.Usage        = InUsage;
                Desc.InitialState = InInitialState;
                return *Device->CreateBuffer( Desc );
            }

            inline TSharedRef<IRHITexture> MakeTexture ( const TSharedRef<IRHIDevice> &Device,
                                                         const UInt32 InMipCount   = 1U,
                                                         const UInt32 InLayerCount = 1U ) noexcept
            {
                FRHITextureDesc Desc;
                Desc.Extent     = { 16U, 16U, 1U };
                Desc.Format     = ERHIFormat::RGBA8Unorm;
                Desc.MipCount   = InMipCount;
                Desc.LayerCount = InLayerCount;
                Desc.Usage = ERHIResourceUsage::ShaderResource | ERHIResourceUsage::RenderTarget | ERHIResourceUsage::CopySource |
                             ERHIResourceUsage::CopyDestination;
                return *Device->CreateTexture( Desc );
            }

            inline FRHISwapchainDesc MakeSwapchainDesc () noexcept
            {
                FRHISwapchainDesc Desc;
                Desc.Surface.Type          = ERHISurfaceType::Null;
                Desc.Surface.NativeWindow  = 1U;
                Desc.Surface.InitialExtent = { 640U, 480U, 1U };
                Desc.Extent                = Desc.Surface.InitialExtent;
                Desc.ImageUsage            = ERHIResourceUsage::RenderTarget | ERHIResourceUsage::CopySource;
                return Desc;
            }
        } // namespace Validation

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
