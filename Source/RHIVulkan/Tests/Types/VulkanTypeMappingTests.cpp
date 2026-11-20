/**
 * @file VulkanTypeMappingTests.cpp
 * @brief Functional tests for RHI-to-Vulkan type mappings.
 */

#include <Tests/UnitTest.hpp>
#include <RHIVulkan/Types/VulkanTypeMapping.hpp>

namespace Lumen
{

namespace RHIVulkan
{

    LUMEN_TEST( VulkanTypeMapping, FormatRoundTripsForKnownFormats )
    {
        constexpr RHI::ERHIFormat Formats[] = { RHI::ERHIFormat::R8Unorm, RHI::ERHIFormat::RGBA8Unorm, RHI::ERHIFormat::BGRA8Srgb,
                                                RHI::ERHIFormat::R32Float, RHI::ERHIFormat::D32FloatS8Uint };

        for ( const RHI::ERHIFormat Format : Formats )
        {
            const TExpected<VkFormat, RHI::FRHIError> VulkanFormat = ToVulkanFormat( Format );
            LUMEN_CHECK_TRUE( VulkanFormat.HasValue() );
            LUMEN_CHECK_EQ( ToRHIFormat( *VulkanFormat ), Format );
        }
    }

    LUMEN_TEST( VulkanTypeMapping, UnknownFormatIsRejected )
    {
        const TExpected<VkFormat, RHI::FRHIError> Concrete = ToVulkanFormat( RHI::ERHIFormat::Unknown );
        LUMEN_CHECK_HAS_ERROR( Concrete );
    }

    LUMEN_TEST( VulkanTypeMapping, UnknownFormatMapsToUndefinedWhenAllowed )
    {
        const TExpected<VkFormat, RHI::FRHIError> Undefined = ToVulkanFormatOrUndefined( RHI::ERHIFormat::Unknown );
        LUMEN_CHECK_TRUE( Undefined.HasValue() );
        LUMEN_CHECK_EQ( *Undefined, VK_FORMAT_UNDEFINED );
    }

    LUMEN_TEST( VulkanTypeMapping, UnmappedVulkanFormatBecomesUnknown )
    {
        LUMEN_CHECK_EQ( ToRHIFormat( VK_FORMAT_ASTC_4x4_UNORM_BLOCK ), RHI::ERHIFormat::Unknown );
    }

    LUMEN_TEST( VulkanTypeMapping, DepthStencilFormatDetection )
    {
        LUMEN_CHECK_TRUE( IsVulkanDepthStencilFormat( VK_FORMAT_D16_UNORM ) );
        LUMEN_CHECK_TRUE( IsVulkanDepthStencilFormat( VK_FORMAT_D32_SFLOAT_S8_UINT ) );
        LUMEN_CHECK_FALSE( IsVulkanDepthStencilFormat( VK_FORMAT_R8G8B8A8_UNORM ) );
    }

    LUMEN_TEST( VulkanTypeMapping, ImageAspectMapping )
    {
        const TExpected<VkImageAspectFlags, RHI::FRHIError> ColorDepth =
            ToVulkanImageAspect( RHI::ERHIFormatAspect::Color | RHI::ERHIFormatAspect::Depth );
        LUMEN_CHECK_TRUE( ColorDepth.HasValue() );
        LUMEN_CHECK_EQ( *ColorDepth, VK_IMAGE_ASPECT_COLOR_BIT | VK_IMAGE_ASPECT_DEPTH_BIT );

        LUMEN_CHECK_HAS_ERROR( ToVulkanImageAspect( RHI::ERHIFormatAspect::None ) );
    }

    LUMEN_TEST( VulkanTypeMapping, FullImageAspectDerivedFromFormat )
    {
        const TExpected<VkImageAspectFlags, RHI::FRHIError> DepthStencil =
            GetVulkanFullImageAspect( RHI::ERHIFormat::D24UnormS8Uint );
        LUMEN_CHECK_TRUE( DepthStencil.HasValue() );
        LUMEN_CHECK_EQ( *DepthStencil, VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT );

        LUMEN_CHECK_HAS_ERROR( GetVulkanFullImageAspect( RHI::ERHIFormat::Unknown ) );
    }

    LUMEN_TEST( VulkanTypeMapping, BufferUsageRejectsImageUsages )
    {
        LUMEN_CHECK_HAS_ERROR( ToVulkanBufferUsage( RHI::ERHIResourceUsage::RenderTarget ) );
    }

    LUMEN_TEST( VulkanTypeMapping, BufferUsageMapsTransferAndVertex )
    {
        const TExpected<VkBufferUsageFlags, RHI::FRHIError> Flags =
            ToVulkanBufferUsage( RHI::ERHIResourceUsage::CopySource | RHI::ERHIResourceUsage::VertexBuffer );
        LUMEN_CHECK_TRUE( Flags.HasValue() );
        LUMEN_CHECK_EQ( *Flags, VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT );
    }

    LUMEN_TEST( VulkanTypeMapping, ImageUsageRejectsBufferUsages )
    {
        LUMEN_CHECK_HAS_ERROR( ToVulkanImageUsage( RHI::ERHIResourceUsage::VertexBuffer ) );
    }

    LUMEN_TEST( VulkanTypeMapping, ImageUsageMapsRenderTarget )
    {
        const TExpected<VkImageUsageFlags, RHI::FRHIError> Flags =
            ToVulkanImageUsage( RHI::ERHIResourceUsage::RenderTarget | RHI::ERHIResourceUsage::ShaderResource );
        LUMEN_CHECK_TRUE( Flags.HasValue() );
        LUMEN_CHECK_EQ( *Flags, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT );
    }

    LUMEN_TEST( VulkanTypeMapping, BufferStateRejectsImageStates )
    {
        LUMEN_CHECK_HAS_ERROR( ToVulkanBufferState( RHI::ERHIResourceState::RenderTarget ) );
    }

    LUMEN_TEST( VulkanTypeMapping, BufferStateMapsVertexBuffer )
    {
        const TExpected<FVulkanResourceBarrierState, RHI::FRHIError> State =
            ToVulkanBufferState( RHI::ERHIResourceState::VertexBuffer );
        LUMEN_CHECK_TRUE( State.HasValue() );
        LUMEN_CHECK_EQ( State->StageMask, VK_PIPELINE_STAGE_2_VERTEX_ATTRIBUTE_INPUT_BIT );
        LUMEN_CHECK_EQ( State->AccessMask, VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT );
    }

    LUMEN_TEST( VulkanTypeMapping, ImageStateRejectsBufferStates )
    {
        LUMEN_CHECK_HAS_ERROR( ToVulkanImageState( RHI::ERHIResourceState::VertexBuffer ) );
    }

    LUMEN_TEST( VulkanTypeMapping, ImageRenderTargetRequiresNonDepth )
    {
        const TExpected<FVulkanResourceBarrierState, RHI::FRHIError> State =
            ToVulkanImageState( RHI::ERHIResourceState::RenderTarget, false );
        LUMEN_CHECK_TRUE( State.HasValue() );
        LUMEN_CHECK_EQ( State->ImageLayout, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL );
        LUMEN_CHECK_HAS_ERROR( ToVulkanImageState( RHI::ERHIResourceState::RenderTarget, true ) );
    }

    LUMEN_TEST( VulkanTypeMapping, ImageDepthWriteRequiresDepthStencil )
    {
        LUMEN_CHECK_HAS_ERROR( ToVulkanImageState( RHI::ERHIResourceState::DepthWrite, false ) );
        const TExpected<FVulkanResourceBarrierState, RHI::FRHIError> State =
            ToVulkanImageState( RHI::ERHIResourceState::DepthWrite, true );
        LUMEN_CHECK_TRUE( State.HasValue() );
        LUMEN_CHECK_EQ( State->ImageLayout, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL );
    }

    LUMEN_TEST( VulkanTypeMapping, SampleCountMapping )
    {
        LUMEN_CHECK_EQ( ToVulkanSampleCount( RHI::ERHISampleCount::Count1 ).ValueOr( VK_SAMPLE_COUNT_1_BIT ),
                        VK_SAMPLE_COUNT_1_BIT );
        LUMEN_CHECK_EQ( ToVulkanSampleCount( RHI::ERHISampleCount::Count4 ).ValueOr( VK_SAMPLE_COUNT_1_BIT ),
                        VK_SAMPLE_COUNT_4_BIT );
        LUMEN_CHECK_HAS_ERROR( ToVulkanSampleCount( static_cast<RHI::ERHISampleCount>( 3U ) ) );
    }

    LUMEN_TEST( VulkanTypeMapping, ShaderStageFlagsMapping )
    {
        const TExpected<VkShaderStageFlags, RHI::FRHIError> Flags =
            ToVulkanShaderStageFlags( RHI::ERHIShaderStage::Vertex | RHI::ERHIShaderStage::Pixel );
        LUMEN_CHECK_TRUE( Flags.HasValue() );
        LUMEN_CHECK_EQ( *Flags, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT );

        LUMEN_CHECK_HAS_ERROR( ToVulkanShaderStageFlags( RHI::ERHIShaderStage::None ) );
    }

    LUMEN_TEST( VulkanTypeMapping, SingleShaderStageMapping )
    {
        LUMEN_CHECK_EQ( ToVulkanSingleShaderStage( RHI::ERHIShaderStage::Compute ).ValueOr( VK_SHADER_STAGE_ALL ),
                        VK_SHADER_STAGE_COMPUTE_BIT );
        LUMEN_CHECK_HAS_ERROR( ToVulkanSingleShaderStage( RHI::ERHIShaderStage::Vertex | RHI::ERHIShaderStage::Pixel ) );
    }

    LUMEN_TEST( VulkanTypeMapping, DescriptorTypeMapping )
    {
        LUMEN_CHECK_EQ( ToVulkanDescriptorType( RHI::ERHIBindingType::ConstantBuffer ).ValueOr( VK_DESCRIPTOR_TYPE_MAX_ENUM ),
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER );
        LUMEN_CHECK_EQ( ToVulkanDescriptorType( RHI::ERHIBindingType::Sampler ).ValueOr( VK_DESCRIPTOR_TYPE_MAX_ENUM ),
                        VK_DESCRIPTOR_TYPE_SAMPLER );
        LUMEN_CHECK_HAS_ERROR( ToVulkanDescriptorType( static_cast<RHI::ERHIBindingType>( 255U ) ) );
    }

    LUMEN_TEST( VulkanTypeMapping, IndexTypeMapping )
    {
        LUMEN_CHECK_EQ( ToVulkanIndexType( RHI::ERHIIndexFormat::Index16 ).ValueOr( VK_INDEX_TYPE_MAX_ENUM ),
                        VK_INDEX_TYPE_UINT16 );
        LUMEN_CHECK_EQ( ToVulkanIndexType( RHI::ERHIIndexFormat::Index32 ).ValueOr( VK_INDEX_TYPE_MAX_ENUM ),
                        VK_INDEX_TYPE_UINT32 );
    }

    LUMEN_TEST( VulkanTypeMapping, ImageViewTypeMapping )
    {
        LUMEN_CHECK_EQ( ToVulkanImageViewType( RHI::ERHITextureViewType::Texture2D ).ValueOr( VK_IMAGE_VIEW_TYPE_MAX_ENUM ),
                        VK_IMAGE_VIEW_TYPE_2D );
        LUMEN_CHECK_EQ( ToVulkanImageViewType( RHI::ERHITextureViewType::TextureCube ).ValueOr( VK_IMAGE_VIEW_TYPE_MAX_ENUM ),
                        VK_IMAGE_VIEW_TYPE_CUBE );
    }

    LUMEN_TEST( VulkanTypeMapping, PrimitiveTopologyMapping )
    {
        LUMEN_CHECK_EQ(
            ToVulkanPrimitiveTopology( RHI::ERHIPrimitiveTopology::TriangleList ).ValueOr( VK_PRIMITIVE_TOPOLOGY_MAX_ENUM ),
            VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST );
    }

    LUMEN_TEST( VulkanTypeMapping, RasterizerStateMappings )
    {
        LUMEN_CHECK_EQ( ToVulkanPolygonMode( RHI::ERHIFillMode::Wireframe ).ValueOr( VK_POLYGON_MODE_MAX_ENUM ),
                        VK_POLYGON_MODE_LINE );
        LUMEN_CHECK_EQ( ToVulkanCullMode( RHI::ERHICullMode::Back ).ValueOr( VK_CULL_MODE_FLAG_BITS_MAX_ENUM ),
                        VK_CULL_MODE_BACK_BIT );
        LUMEN_CHECK_EQ( ToVulkanFrontFace( RHI::ERHIFrontFace::Clockwise ).ValueOr( VK_FRONT_FACE_MAX_ENUM ),
                        VK_FRONT_FACE_CLOCKWISE );
    }

    LUMEN_TEST( VulkanTypeMapping, BlendMappings )
    {
        LUMEN_CHECK_EQ( ToVulkanBlendFactor( RHI::ERHIBlendFactor::SourceAlpha ).ValueOr( VK_BLEND_FACTOR_MAX_ENUM ),
                        VK_BLEND_FACTOR_SRC_ALPHA );
        LUMEN_CHECK_EQ( ToVulkanBlendOp( RHI::ERHIBlendOp::ReverseSubtract ).ValueOr( VK_BLEND_OP_MAX_ENUM ),
                        VK_BLEND_OP_REVERSE_SUBTRACT );
    }

    LUMEN_TEST( VulkanTypeMapping, DepthStencilMappings )
    {
        LUMEN_CHECK_EQ( ToVulkanStencilOp( RHI::ERHIStencilOp::IncrementWrap ).ValueOr( VK_STENCIL_OP_MAX_ENUM ),
                        VK_STENCIL_OP_INCREMENT_AND_WRAP );
        LUMEN_CHECK_EQ( ToVulkanCompareOp( RHI::ERHICompareFunction::LessEqual ).ValueOr( VK_COMPARE_OP_MAX_ENUM ),
                        VK_COMPARE_OP_LESS_OR_EQUAL );
    }

    LUMEN_TEST( VulkanTypeMapping, SamplerMappings )
    {
        LUMEN_CHECK_EQ( ToVulkanFilter( RHI::ERHISamplerFilter::Linear ).ValueOr( VK_FILTER_MAX_ENUM ), VK_FILTER_LINEAR );
        LUMEN_CHECK_EQ( ToVulkanMipmapMode( RHI::ERHISamplerFilter::Nearest ).ValueOr( VK_SAMPLER_MIPMAP_MODE_MAX_ENUM ),
                        VK_SAMPLER_MIPMAP_MODE_NEAREST );
        LUMEN_CHECK_EQ(
            ToVulkanAddressMode( RHI::ERHISamplerAddressMode::ClampToEdge ).ValueOr( VK_SAMPLER_ADDRESS_MODE_MAX_ENUM ),
            VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE );
    }

    LUMEN_TEST( VulkanTypeMapping, AttachmentLoadStoreMappings )
    {
        LUMEN_CHECK_EQ( ToVulkanAttachmentLoadOp( RHI::ERHIRenderingLoadOp::Clear ).ValueOr( VK_ATTACHMENT_LOAD_OP_MAX_ENUM ),
                        VK_ATTACHMENT_LOAD_OP_CLEAR );
        LUMEN_CHECK_EQ(
            ToVulkanAttachmentStoreOp( RHI::ERHIRenderingStoreOp::DontCare ).ValueOr( VK_ATTACHMENT_STORE_OP_MAX_ENUM ),
            VK_ATTACHMENT_STORE_OP_DONT_CARE );
    }

    LUMEN_TEST( VulkanTypeMapping, QueryTypeMapping )
    {
        LUMEN_CHECK_EQ( ToVulkanQueryType( RHI::ERHIQueryType::Timestamp ).ValueOr( VK_QUERY_TYPE_MAX_ENUM ),
                        VK_QUERY_TYPE_TIMESTAMP );
    }

    LUMEN_TEST( VulkanTypeMapping, PresentModeRoundTrip )
    {
        LUMEN_CHECK_EQ( ToVulkanPresentMode( RHI::ERHIPresentMode::Fifo ).ValueOr( VK_PRESENT_MODE_IMMEDIATE_KHR ),
                        VK_PRESENT_MODE_FIFO_KHR );
        LUMEN_CHECK_TRUE( ToRHIPresentMode( VK_PRESENT_MODE_FIFO_KHR ).HasValue() );
        LUMEN_CHECK_EQ( *ToRHIPresentMode( VK_PRESENT_MODE_FIFO_KHR ), RHI::ERHIPresentMode::Fifo );
        LUMEN_CHECK_FALSE( ToRHIPresentMode( VK_PRESENT_MODE_SHARED_DEMAND_REFRESH_KHR ).HasValue() );
    }

    LUMEN_TEST( VulkanTypeMapping, ColorSpaceRoundTrip )
    {
        LUMEN_CHECK_EQ( ToVulkanColorSpace( RHI::ERHIColorSpace::SrgbNonLinear ).ValueOr( VK_COLOR_SPACE_MAX_ENUM_KHR ),
                        VK_COLOR_SPACE_SRGB_NONLINEAR_KHR );
        LUMEN_CHECK_TRUE( ToRHIColorSpace( VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ).HasValue() );
        LUMEN_CHECK_EQ( *ToRHIColorSpace( VK_COLOR_SPACE_SRGB_NONLINEAR_KHR ), RHI::ERHIColorSpace::SrgbNonLinear );
        LUMEN_CHECK_FALSE( ToRHIColorSpace( VK_COLOR_SPACE_ADOBERGB_LINEAR_EXT ).HasValue() );
    }

    LUMEN_TEST( VulkanTypeMapping, QueueFamilyCompatibility )
    {
        LUMEN_CHECK_TRUE( IsVulkanQueueFamilyCompatible( VK_QUEUE_GRAPHICS_BIT, RHI::ERHIQueueType::Graphics ) );
        LUMEN_CHECK_FALSE( IsVulkanQueueFamilyCompatible( VK_QUEUE_COMPUTE_BIT, RHI::ERHIQueueType::Graphics ) );
        LUMEN_CHECK_TRUE( IsVulkanQueueFamilyCompatible( VK_QUEUE_GRAPHICS_BIT, RHI::ERHIQueueType::Copy ) );
        LUMEN_CHECK_FALSE( IsVulkanQueueFamilyCompatible( 0U, RHI::ERHIQueueType::Compute ) );
    }

} // namespace RHIVulkan

} // namespace Lumen
