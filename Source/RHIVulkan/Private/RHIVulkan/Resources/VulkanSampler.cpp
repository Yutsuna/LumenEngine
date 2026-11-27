/**
 * @file VulkanSampler.cpp
 * @brief Implements immutable Vulkan sampler creation.
 */

#include <RHIVulkan/Resources/VulkanSampler.hpp>

#include <RHIVulkan/Types/VulkanObject.hpp>
#include <RHIVulkan/Types/VulkanTypeMapping.hpp>
#include <RHIVulkan/Utilities/VulkanResult.hpp>

/**
 * Public
 */

Lumen::TExpected<Lumen::TSharedRef<Lumen::RHIVulkan::FVulkanSampler>, Lumen::RHI::FRHIError>
Lumen::RHIVulkan::FVulkanSampler::Create ( TSharedRef<FVulkanDeviceContext> InContext,
                                           const RHI::FRHISamplerDesc &InDesc,
                                           RHI::FRHIObjectId InId ) noexcept
{
    LUMEN_EXPECT( Validation, InDesc.Validate() );
    const TExpected<VkFilter, RHI::FRHIError> MinFilter = ToVulkanFilter( InDesc.MinFilter );
    LUMEN_EXPECT( NativeMinFilter, MinFilter );
    const TExpected<VkFilter, RHI::FRHIError> MagFilter = ToVulkanFilter( InDesc.MagFilter );
    LUMEN_EXPECT( NativeMagFilter, MagFilter );
    const TExpected<VkSamplerMipmapMode, RHI::FRHIError> MipmapMode = ToVulkanMipmapMode( InDesc.MipFilter );
    LUMEN_EXPECT( NativeMipmapMode, MipmapMode );
    const TExpected<VkSamplerAddressMode, RHI::FRHIError> AddressU = ToVulkanAddressMode( InDesc.AddressU );
    LUMEN_EXPECT( NativeAddressU, AddressU );
    const TExpected<VkSamplerAddressMode, RHI::FRHIError> AddressV = ToVulkanAddressMode( InDesc.AddressV );
    LUMEN_EXPECT( NativeAddressV, AddressV );
    const TExpected<VkSamplerAddressMode, RHI::FRHIError> AddressW = ToVulkanAddressMode( InDesc.AddressW );
    LUMEN_EXPECT( NativeAddressW, AddressW );

    VkSamplerCreateInfo SamplerInfo{};
    SamplerInfo.sType            = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    SamplerInfo.minFilter        = NativeMinFilter;
    SamplerInfo.magFilter        = NativeMagFilter;
    SamplerInfo.mipmapMode       = NativeMipmapMode;
    SamplerInfo.addressModeU     = NativeAddressU;
    SamplerInfo.addressModeV     = NativeAddressV;
    SamplerInfo.addressModeW     = NativeAddressW;
    SamplerInfo.minLod           = InDesc.MinLod;
    SamplerInfo.maxLod           = InDesc.MaxLod;
    SamplerInfo.anisotropyEnable = InDesc.MaxAnisotropy > 1U ? VK_TRUE : VK_FALSE;
    SamplerInfo.maxAnisotropy    = static_cast<Float32>( InDesc.MaxAnisotropy );
    SamplerInfo.borderColor      = VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
    SamplerInfo.compareEnable    = InDesc.CompareFunction.HasValue() ? VK_TRUE : VK_FALSE;
    if ( InDesc.CompareFunction.HasValue() )
    {
        const TExpected<VkCompareOp, RHI::FRHIError> CompareOp = ToVulkanCompareOp( *InDesc.CompareFunction );
        LUMEN_EXPECT( NativeCompareOp, CompareOp );
        SamplerInfo.compareOp = NativeCompareOp;
    }

    TSharedRef<FVulkanSampler> Sampler =
        MakeShared<FVulkanSampler>( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ), InDesc, InId );
    LUMEN_VK_TRY(
        vkCreateSampler( Sampler->Context->GetDevice(), &SamplerInfo, nullptr,
                         Sampler->Sampler.ResetAndGetAddressOf( FVulkanSamplerDeleter{ Sampler->Context->GetDevice() } ) ),
        RHI::ERHIErrorCondition::Resource );
    return Sampler;
}

Lumen::RHIVulkan::FVulkanSampler::FVulkanSampler ( TSharedRef<FVulkanDeviceContext> InContext,
                                                   const RHI::FRHISamplerDesc &InDesc,
                                                   RHI::FRHIObjectId InId ) noexcept
    : Identity( MakeVulkanObjectBase<FVulkanSampler>( InId ) ),
      Context( static_cast<TSharedRef<FVulkanDeviceContext> &&>( InContext ) ), Desc( InDesc )
{
    /* */
}

Lumen::RHIVulkan::FVulkanSampler::~FVulkanSampler () noexcept
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( RHIVulkan, FVulkanSampler )

const Lumen::RHI::FRHISamplerDesc &Lumen::RHIVulkan::FVulkanSampler::GetDesc () const noexcept
{
    return Desc;
}
