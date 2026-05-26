/**
 * @file VulkanPipeline.cpp
 * @brief Implementation of the FVulkanPipeline class using SPIR-V blobs.
 */

#include "Vulkan/VulkanPipeline.hpp"
#include "Vulkan/VulkanCore.hpp"

#include "Container/Array.hpp"
#include "Maths/Matrix.hpp"
#include "Maths/Vertex.hpp"

#include <cstddef>
#include <span>

namespace
{

[[nodiscard]] VkShaderModule CreateShaderModule ( VkDevice InDevice, const LumenEngine::RHI::FShaderByteCode &InSpirV )
{
    VkShaderModuleCreateInfo ShaderCI{ .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                       .pNext    = nullptr,
                                       .flags    = 0,
                                       .codeSize = InSpirV.size() * sizeof( LumenEngine::UInt32 ),
                                       .pCode    = InSpirV.data() };

    VkShaderModule Module = VK_NULL_HANDLE;
    LUMEN_VK_CHECK( vkCreateShaderModule( InDevice, &ShaderCI, nullptr, &Module ) );
    return Module;
}

struct FPipelineBuildState final
{
    LumenEngine::TArray<VkPipelineShaderStageCreateInfo, 2> ShaderStages            = {};
    VkVertexInputBindingDescription BindingDescription                              = {};
    LumenEngine::TArray<VkVertexInputAttributeDescription, 4> AttributeDescriptions = {};
    LumenEngine::TVector<VkPushConstantRange> PushConstantRanges;
    VkPipelineVertexInputStateCreateInfo VertexInputState     = {};
    VkPipelineInputAssemblyStateCreateInfo InputAssembly      = {};
    VkPipelineViewportStateCreateInfo ViewportState           = {};
    VkPipelineRasterizationStateCreateInfo RasterizationState = {};
    VkPipelineMultisampleStateCreateInfo MultisampleState     = {};
    VkPipelineColorBlendAttachmentState ColorBlendAttachment  = {};
    VkPipelineColorBlendStateCreateInfo ColorBlendState       = {};
    VkPipelineDynamicStateCreateInfo DynamicState             = {};
    VkPipelineDepthStencilStateCreateInfo DepthStencilState   = {};
    VkPipelineLayoutCreateInfo PipelineLayoutInfo             = {};
    VkPipelineRenderingCreateInfo RenderingInfo               = {};
};

[[nodiscard]] VkVertexInputBindingDescription CreateVertexBindingDescription ( const LumenEngine::VulkanRHI::FPipelineVertexInputDescription &InDescription )
{
    return VkVertexInputBindingDescription{
        .binding   = 0,
        .stride    = sizeof( LumenEngine::Maths::FVertex ),
        .inputRate = InDescription.InputRate,
    };
}

[[nodiscard]] LumenEngine::TArray<VkVertexInputAttributeDescription, 4> CreateVertexAttributeDescriptions ()
{
    return LumenEngine::TArray<VkVertexInputAttributeDescription, 4>{
        VkVertexInputAttributeDescription{
            .location = 0,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof( LumenEngine::Maths::FVertex, Position ),
        },
        VkVertexInputAttributeDescription{
            .location = 1,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof( LumenEngine::Maths::FVertex, Normal ),
        },
        VkVertexInputAttributeDescription{
            .location = 2,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32_SFLOAT,
            .offset   = offsetof( LumenEngine::Maths::FVertex, UV ),
        },
        VkVertexInputAttributeDescription{
            .location = 3,
            .binding  = 0,
            .format   = VK_FORMAT_R32G32B32_SFLOAT,
            .offset   = offsetof( LumenEngine::Maths::FVertex, Tangent ),
        },
    };
}

[[nodiscard]] VkPipelineShaderStageCreateInfo CreateShaderStage ( VkShaderModule InModule, const VkShaderStageFlagBits InStage )
{
    return VkPipelineShaderStageCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .pNext               = nullptr,
        .flags               = 0,
        .stage               = InStage,
        .module              = InModule,
        .pName               = "main",
        .pSpecializationInfo = nullptr,
    };
}

[[nodiscard]] VkPipelineVertexInputStateCreateInfo CreateVertexInputState ( const VkVertexInputBindingDescription &InBindingDescription,
                                                                            const std::span<const VkVertexInputAttributeDescription> InAttributes )
{
    return VkPipelineVertexInputStateCreateInfo{
        .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext                           = nullptr,
        .flags                           = 0,
        .vertexBindingDescriptionCount   = 1,
        .pVertexBindingDescriptions      = &InBindingDescription,
        .vertexAttributeDescriptionCount = static_cast<LumenEngine::UInt32>( InAttributes.size() ),
        .pVertexAttributeDescriptions    = InAttributes.data(),
    };
}

[[nodiscard]] VkPipelineInputAssemblyStateCreateInfo CreateInputAssemblyState ( const LumenEngine::VulkanRHI::FPipelineInputAssemblyDescription &InDescription )
{
    return VkPipelineInputAssemblyStateCreateInfo{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .topology               = InDescription.Topology,
        .primitiveRestartEnable = InDescription.bEnablePrimitiveRestart ? VK_TRUE : VK_FALSE,
    };
}

[[nodiscard]] VkPipelineViewportStateCreateInfo CreateViewportState ()
{
    return VkPipelineViewportStateCreateInfo{
        .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext         = nullptr,
        .flags         = 0,
        .viewportCount = 1,
        .pViewports    = nullptr,
        .scissorCount  = 1,
        .pScissors     = nullptr,
    };
}

[[nodiscard]] VkPipelineRasterizationStateCreateInfo CreateRasterizationState ( const LumenEngine::VulkanRHI::FPipelineRasterizationDescription &InDescription )
{
    return VkPipelineRasterizationStateCreateInfo{
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext                   = nullptr,
        .flags                   = 0,
        .depthClampEnable        = InDescription.bEnableDepthClamp ? VK_TRUE : VK_FALSE,
        .rasterizerDiscardEnable = InDescription.bEnableRasterizerDiscard ? VK_TRUE : VK_FALSE,
        .polygonMode             = InDescription.PolygonMode,
        .cullMode                = InDescription.CullMode,
        .frontFace               = InDescription.FrontFace,
        .depthBiasEnable         = InDescription.bEnableDepthBias ? VK_TRUE : VK_FALSE,
        .depthBiasConstantFactor = 0.0F,
        .depthBiasClamp          = 0.0F,
        .depthBiasSlopeFactor    = 0.0F,
        .lineWidth               = InDescription.LineWidth,
    };
}

[[nodiscard]] VkPipelineMultisampleStateCreateInfo CreateMultisampleState ( const LumenEngine::VulkanRHI::FPipelineMultisampleDescription &InDescription )
{
    return VkPipelineMultisampleStateCreateInfo{
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .rasterizationSamples  = InDescription.RasterizationSamples,
        .sampleShadingEnable   = InDescription.bEnableSampleShading ? VK_TRUE : VK_FALSE,
        .minSampleShading      = 0.0F,
        .pSampleMask           = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable      = VK_FALSE,
    };
}

[[nodiscard]] VkPipelineColorBlendAttachmentState CreateColorBlendAttachmentState ( const LumenEngine::VulkanRHI::FPipelineColorBlendDescription &InDescription )
{
    return VkPipelineColorBlendAttachmentState{
        .blendEnable         = InDescription.bEnableBlend ? VK_TRUE : VK_FALSE,
        .srcColorBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstColorBlendFactor = VK_BLEND_FACTOR_ZERO,
        .colorBlendOp        = VK_BLEND_OP_ADD,
        .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
        .dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
        .alphaBlendOp        = VK_BLEND_OP_ADD,
        .colorWriteMask      = InDescription.WriteMask,
    };
}

[[nodiscard]] VkPipelineColorBlendStateCreateInfo CreateColorBlendState ( const LumenEngine::VulkanRHI::FPipelineColorBlendDescription &InDescription,
                                                                          const VkPipelineColorBlendAttachmentState &InAttachment )
{
    return VkPipelineColorBlendStateCreateInfo{
        .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext           = nullptr,
        .flags           = 0,
        .logicOpEnable   = InDescription.bEnableLogicOp ? VK_TRUE : VK_FALSE,
        .logicOp         = InDescription.LogicOp,
        .attachmentCount = 1,
        .pAttachments    = &InAttachment,
        .blendConstants  = { 0.0F, 0.0F, 0.0F, 0.0F },
    };
}

[[nodiscard]] VkPipelineDynamicStateCreateInfo CreateDynamicState ( const std::span<const VkDynamicState> InDynamicStates )
{
    return VkPipelineDynamicStateCreateInfo{
        .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .pNext             = nullptr,
        .flags             = 0,
        .dynamicStateCount = static_cast<LumenEngine::UInt32>( InDynamicStates.size() ),
        .pDynamicStates    = InDynamicStates.data(),
    };
}

[[nodiscard]] VkPipelineLayoutCreateInfo CreatePipelineLayoutInfo ()
{
    return VkPipelineLayoutCreateInfo{
        .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .pNext                  = nullptr,
        .flags                  = 0,
        .setLayoutCount         = 1,
        .pSetLayouts            = nullptr,
        .pushConstantRangeCount = 0,
        .pPushConstantRanges    = nullptr,
    };
}

[[nodiscard]] VkPipelineRenderingCreateInfo CreatePipelineRenderingInfo ()
{
    return VkPipelineRenderingCreateInfo{
        .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
        .pNext                   = nullptr,
        .viewMask                = 0,
        .colorAttachmentCount    = 1,
        .pColorAttachmentFormats = nullptr,
        .depthAttachmentFormat   = VK_FORMAT_UNDEFINED,
        .stencilAttachmentFormat = VK_FORMAT_UNDEFINED,
    };
}

[[nodiscard]] VkPipelineDepthStencilStateCreateInfo CreateDepthStencilState ( const LumenEngine::VulkanRHI::FPipelineDepthStencilDescription &InDescription )
{
    return VkPipelineDepthStencilStateCreateInfo{
        .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext                 = nullptr,
        .flags                 = 0,
        .depthTestEnable       = InDescription.bEnableDepthTest ? VK_TRUE : VK_FALSE,
        .depthWriteEnable      = InDescription.bEnableDepthWrite ? VK_TRUE : VK_FALSE,
        .depthCompareOp        = InDescription.DepthCompareOp,
        .depthBoundsTestEnable = InDescription.bEnableDepthBoundsTest ? VK_TRUE : VK_FALSE,
        .stencilTestEnable     = InDescription.bEnableStencilTest ? VK_TRUE : VK_FALSE,
        .front                 = VkStencilOpState{ .failOp      = VK_STENCIL_OP_KEEP,
                                                   .passOp      = VK_STENCIL_OP_KEEP,
                                                   .depthFailOp = VK_STENCIL_OP_KEEP,
                                                   .compareOp   = VK_COMPARE_OP_ALWAYS,
                                                   .compareMask = 0,
                                                   .writeMask   = 0,
                                                   .reference   = 0 },
        .back                  = VkStencilOpState{ .failOp      = VK_STENCIL_OP_KEEP,
                                                   .passOp      = VK_STENCIL_OP_KEEP,
                                                   .depthFailOp = VK_STENCIL_OP_KEEP,
                                                   .compareOp   = VK_COMPARE_OP_ALWAYS,
                                                   .compareMask = 0,
                                                   .writeMask   = 0,
                                                   .reference   = 0 },
        .minDepthBounds        = InDescription.MinDepthBounds,
        .maxDepthBounds        = InDescription.MaxDepthBounds,
    };
}

[[nodiscard]] LumenEngine::TVector<VkPushConstantRange>
CreatePushConstantRanges ( const LumenEngine::TVector<LumenEngine::VulkanRHI::FPipelinePushConstantRangeDescription> &InRanges )
{
    LumenEngine::TVector<VkPushConstantRange> PushRanges;
    PushRanges.reserve( InRanges.size() );

    for ( const LumenEngine::VulkanRHI::FPipelinePushConstantRangeDescription &Range : InRanges )
    {
        PushRanges.emplace_back( VkPushConstantRange{
            .stageFlags = Range.StageFlags,
            .offset     = Range.Offset,
            .size       = Range.Size,
        } );
    }

    return PushRanges;
}

[[nodiscard]] VkGraphicsPipelineCreateInfo CreateGraphicsPipelineInfo ( const std::span<const VkPipelineShaderStageCreateInfo> InShaderStages,
                                                                        const VkPipelineVertexInputStateCreateInfo &InVertexInputState,
                                                                        const VkPipelineInputAssemblyStateCreateInfo &InInputAssembly,
                                                                        const VkPipelineViewportStateCreateInfo &InViewportState,
                                                                        const VkPipelineRasterizationStateCreateInfo &InRasterization,
                                                                        const VkPipelineMultisampleStateCreateInfo &InMultisample,
                                                                        const VkPipelineDepthStencilStateCreateInfo &InDepthStencil,
                                                                        const VkPipelineColorBlendStateCreateInfo &InColorBlend,
                                                                        const VkPipelineDynamicStateCreateInfo &InDynamicState,
                                                                        VkPipelineLayout InPipelineLayout,
                                                                        const VkPipelineRenderingCreateInfo &InRenderingInfo )
{
    return VkGraphicsPipelineCreateInfo{
        .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext               = &InRenderingInfo,
        .flags               = 0,
        .stageCount          = static_cast<LumenEngine::UInt32>( InShaderStages.size() ),
        .pStages             = InShaderStages.data(),
        .pVertexInputState   = &InVertexInputState,
        .pInputAssemblyState = &InInputAssembly,
        .pTessellationState  = nullptr,
        .pViewportState      = &InViewportState,
        .pRasterizationState = &InRasterization,
        .pMultisampleState   = &InMultisample,
        .pDepthStencilState  = &InDepthStencil,
        .pColorBlendState    = &InColorBlend,
        .pDynamicState       = &InDynamicState,
        .layout              = InPipelineLayout,
        .renderPass          = VK_NULL_HANDLE,
        .subpass             = 0,
        .basePipelineHandle  = VK_NULL_HANDLE,
        .basePipelineIndex   = -1,
    };
}

[[nodiscard]] VkGraphicsPipelineCreateInfo
CreateGraphicsPipelineInfoFromState ( const FPipelineBuildState &InState, VkPipelineLayout InPipelineLayout, const VkPipelineRenderingCreateInfo &InRenderingInfo )
{
    return CreateGraphicsPipelineInfo( std::span<const VkPipelineShaderStageCreateInfo>( InState.ShaderStages ), InState.VertexInputState, InState.InputAssembly,
                                       InState.ViewportState, InState.RasterizationState, InState.MultisampleState, InState.DepthStencilState, InState.ColorBlendState,
                                       InState.DynamicState, InPipelineLayout, InRenderingInfo );
}

void BuildPipelineState ( FPipelineBuildState &OutState,
                          const LumenEngine::VulkanRHI::FPipelineDescription &InDescription,
                          VkShaderModule InVertexModule,
                          VkShaderModule InFragmentModule )
{
    OutState.ShaderStages = { CreateShaderStage( InVertexModule, VK_SHADER_STAGE_VERTEX_BIT ), CreateShaderStage( InFragmentModule, VK_SHADER_STAGE_FRAGMENT_BIT ) };
    OutState.BindingDescription    = CreateVertexBindingDescription( InDescription.VertexInput );
    OutState.AttributeDescriptions = CreateVertexAttributeDescriptions();
    OutState.VertexInputState =
        CreateVertexInputState( OutState.BindingDescription, std::span<const VkVertexInputAttributeDescription>( OutState.AttributeDescriptions ) );
    OutState.InputAssembly        = CreateInputAssemblyState( InDescription.InputAssembly );
    OutState.ViewportState        = CreateViewportState();
    OutState.RasterizationState   = CreateRasterizationState( InDescription.Rasterization );
    OutState.MultisampleState     = CreateMultisampleState( InDescription.Multisample );
    OutState.DepthStencilState    = CreateDepthStencilState( InDescription.DepthStencil );
    OutState.ColorBlendAttachment = CreateColorBlendAttachmentState( InDescription.ColorBlend );
    OutState.ColorBlendState      = CreateColorBlendState( InDescription.ColorBlend, OutState.ColorBlendAttachment );
    OutState.DynamicState         = CreateDynamicState( std::span<const VkDynamicState>( InDescription.DynamicState.States ) );

    OutState.PushConstantRanges                        = CreatePushConstantRanges( InDescription.PushConstantRanges );
    OutState.PipelineLayoutInfo                        = CreatePipelineLayoutInfo();
    OutState.PipelineLayoutInfo.pSetLayouts            = &InDescription.GlobalSetLayout;
    OutState.PipelineLayoutInfo.pushConstantRangeCount = static_cast<LumenEngine::UInt32>( OutState.PushConstantRanges.size() );
    OutState.PipelineLayoutInfo.pPushConstantRanges    = OutState.PushConstantRanges.empty() ? nullptr : OutState.PushConstantRanges.data();

    OutState.RenderingInfo                         = CreatePipelineRenderingInfo();
    OutState.RenderingInfo.pColorAttachmentFormats = &InDescription.ColorFormat;
    OutState.RenderingInfo.depthAttachmentFormat   = InDescription.DepthStencil.DepthAttachmentFormat;
    OutState.RenderingInfo.stencilAttachmentFormat = InDescription.DepthStencil.StencilAttachmentFormat;
}
} // namespace

LumenEngine::VulkanRHI::FPipelineDescription LumenEngine::VulkanRHI::FVulkanPipeline::CreateDefaultDescription ( const VkFormat InColorFormat,
                                                                                                                 VkDescriptorSetLayout InGlobalSetLayout,
                                                                                                                 VkSampleCountFlagBits InSamples )
{
    LumenEngine::VulkanRHI::FPipelineDescription Description;
    Description.ColorFormat                      = InColorFormat;
    Description.GlobalSetLayout                  = InGlobalSetLayout;
    Description.Multisample.RasterizationSamples = InSamples;

    LumenEngine::VulkanRHI::FPipelinePushConstantRangeDescription PushConstant;
    PushConstant.StageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
    PushConstant.Offset     = 0;
    PushConstant.Size       = sizeof( LumenEngine::Maths::FMatrix4x4f );
    Description.PushConstantRanges.push_back( PushConstant );

    return Description;
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::VulkanRHI::FVulkanPipeline::Initialize ( VkDevice InDevice,
                                                                                                                  const FPipelineDescription &InDescription,
                                                                                                                  const RHI::FShaderByteCode &InVertexSpirV,
                                                                                                                  const RHI::FShaderByteCode &InFragmentSpirV )
{
    Cleanup( InDevice );

    if ( InDevice == VK_NULL_HANDLE or InDescription.GlobalSetLayout == VK_NULL_HANDLE or InVertexSpirV.empty() or InFragmentSpirV.empty() )
    {
        return std::unexpected( LumenEngine::EErrorCode::InvalidArgument );
    }

    // Cache state to support visual config shifts or context recreations
    DescriptionCapped = InDescription;
    VertexSpirV       = InVertexSpirV;
    FragmentSpirV     = InFragmentSpirV;

    VkShaderModule VertexModule   = CreateShaderModule( InDevice, InVertexSpirV );
    VkShaderModule FragmentModule = CreateShaderModule( InDevice, InFragmentSpirV );

    FPipelineBuildState PipelineState;
    BuildPipelineState( PipelineState, InDescription, VertexModule, FragmentModule );

    LUMEN_VK_CHECK( vkCreatePipelineLayout( InDevice, &PipelineState.PipelineLayoutInfo, nullptr, &PipelineLayout ) );

    const VkGraphicsPipelineCreateInfo PipelineInfo = CreateGraphicsPipelineInfoFromState( PipelineState, PipelineLayout, PipelineState.RenderingInfo );
    LUMEN_VK_CHECK( vkCreateGraphicsPipelines( InDevice, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline ) );

    vkDestroyShaderModule( InDevice, VertexModule, nullptr );
    vkDestroyShaderModule( InDevice, FragmentModule, nullptr );

    return {};
}

LumenEngine::TExpected<void, LumenEngine::EErrorCode::Type> LumenEngine::VulkanRHI::FVulkanPipeline::Recreate ( VkDevice InDevice, VkSampleCountFlagBits InSamples )
{
    // Take dynamic local copies of the bytecode vectors to prevent self-overwriting
    const RHI::FShaderByteCode TempVertex     = VertexSpirV;
    const RHI::FShaderByteCode TempFragment   = FragmentSpirV;
    FPipelineDescription TempDesc             = DescriptionCapped;
    TempDesc.Multisample.RasterizationSamples = InSamples;

    return Initialize( InDevice, TempDesc, TempVertex, TempFragment );
}

void LumenEngine::VulkanRHI::FVulkanPipeline::Cleanup ( VkDevice InDevice ) noexcept
{
    if ( Pipeline != VK_NULL_HANDLE )
    {
        vkDestroyPipeline( InDevice, Pipeline, nullptr );
        Pipeline = VK_NULL_HANDLE;
    }

    if ( PipelineLayout != VK_NULL_HANDLE )
    {
        vkDestroyPipelineLayout( InDevice, PipelineLayout, nullptr );
        PipelineLayout = VK_NULL_HANDLE;
    }
}

void LumenEngine::VulkanRHI::FVulkanPipeline::Bind ( VkCommandBuffer InCommandBuffer ) const noexcept
{
    vkCmdBindPipeline( InCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, Pipeline );
}

VkPipelineLayout LumenEngine::VulkanRHI::FVulkanPipeline::GetLayout () const noexcept
{
    return PipelineLayout;
}
