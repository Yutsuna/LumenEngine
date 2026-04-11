/**
 * @file VulkanPipeline.cpp
 * @brief Implementation of the FVulkanPipeline class.
 */

#include "Vulkan/VulkanPipeline.hpp"
#include "Maths/Vertex.hpp"
#include "Vulkan/VulkanCore.hpp"
#include "Vulkan/VulkanShader.hpp"

LumenEngine::Bool LumenEngine::VulkanRHI::FVulkanPipeline::Initialize ( VkDevice InDevice,
                                                                        VkFormat InColorFormat,
                                                                        const LumenEngine::FString &InVertexPath,
                                                                        const LumenEngine::FString &InFragmentPath,
                                                                        VkDescriptorSetLayout InGlobalSetLayout )
{
    LumenEngine::VulkanRHI::FVulkanShader VertexShader;

    if ( not VertexShader.CompileFromFile( InDevice, InVertexPath, VK_SHADER_STAGE_VERTEX_BIT ) )
    {
        return false;
    }

    LumenEngine::VulkanRHI::FVulkanShader FragmentShader;
    if ( not FragmentShader.CompileFromFile( InDevice, InFragmentPath, VK_SHADER_STAGE_FRAGMENT_BIT ) )
    {
        VertexShader.Cleanup( InDevice );
        return false;
    }

    VkPipelineShaderStageCreateInfo ShaderStages[2] = {};
    ShaderStages[0].sType                           = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[0].stage                           = VK_SHADER_STAGE_VERTEX_BIT;
    ShaderStages[0].module                          = VertexShader.GetModule();
    ShaderStages[0].pName                           = "main";

    ShaderStages[1].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    ShaderStages[1].stage  = VK_SHADER_STAGE_FRAGMENT_BIT;
    ShaderStages[1].module = FragmentShader.GetModule();
    ShaderStages[1].pName  = "main";

    VkVertexInputBindingDescription BindingDescription{};
    BindingDescription.binding   = 0;
    BindingDescription.stride    = sizeof( LumenEngine::Maths::FVertex );
    BindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription AttributeDescriptions[4] = {};
    AttributeDescriptions[0].binding                           = 0;
    AttributeDescriptions[0].location                          = 0;
    AttributeDescriptions[0].format                            = VK_FORMAT_R32G32B32_SFLOAT;
    AttributeDescriptions[0].offset                            = offsetof( LumenEngine::Maths::FVertex, Position );

    AttributeDescriptions[1].binding  = 0;
    AttributeDescriptions[1].location = 1;
    AttributeDescriptions[1].format   = VK_FORMAT_R32G32B32_SFLOAT;
    AttributeDescriptions[1].offset   = offsetof( LumenEngine::Maths::FVertex, Normal );

    AttributeDescriptions[2].binding  = 0;
    AttributeDescriptions[2].location = 2;
    AttributeDescriptions[2].format   = VK_FORMAT_R32G32_SFLOAT;
    AttributeDescriptions[2].offset   = offsetof( LumenEngine::Maths::FVertex, UV );

    AttributeDescriptions[3].binding  = 0;
    AttributeDescriptions[3].location = 3;
    AttributeDescriptions[3].format   = VK_FORMAT_R32G32B32_SFLOAT;
    AttributeDescriptions[3].offset   = offsetof( LumenEngine::Maths::FVertex, Tangent );

    VkPipelineVertexInputStateCreateInfo VertexInputInfo{};
    VertexInputInfo.sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    VertexInputInfo.vertexBindingDescriptionCount   = 1;
    VertexInputInfo.pVertexBindingDescriptions      = &BindingDescription;
    VertexInputInfo.vertexAttributeDescriptionCount = 4;
    VertexInputInfo.pVertexAttributeDescriptions    = AttributeDescriptions;

    VkPipelineInputAssemblyStateCreateInfo InputAssembly{};
    InputAssembly.sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    InputAssembly.topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    InputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo ViewportState{};
    ViewportState.sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    ViewportState.viewportCount = 1;
    ViewportState.scissorCount  = 1;

    VkPipelineRasterizationStateCreateInfo Rasterizer{};
    Rasterizer.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    Rasterizer.depthClampEnable        = VK_FALSE;
    Rasterizer.rasterizerDiscardEnable = VK_FALSE;
    Rasterizer.polygonMode             = VK_POLYGON_MODE_FILL;
    Rasterizer.lineWidth               = 1.0f;
    Rasterizer.cullMode                = VK_CULL_MODE_NONE; // We disable culling for the example presentation to easily see all sides.
    Rasterizer.frontFace               = VK_FRONT_FACE_CLOCKWISE;
    Rasterizer.depthBiasEnable         = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo Multisampling{};
    Multisampling.sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    Multisampling.sampleShadingEnable  = VK_FALSE;
    Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
    ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    ColorBlendAttachment.blendEnable    = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo ColorBlending{};
    ColorBlending.sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    ColorBlending.logicOpEnable   = VK_FALSE;
    ColorBlending.logicOp         = VK_LOGIC_OP_COPY;
    ColorBlending.attachmentCount = 1;
    ColorBlending.pAttachments    = &ColorBlendAttachment;

    const VkDynamicState DynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    VkPipelineDynamicStateCreateInfo DynamicState{};
    DynamicState.sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    DynamicState.dynamicStateCount = 2;
    DynamicState.pDynamicStates    = DynamicStates;

    VkPipelineLayoutCreateInfo PipelineLayoutInfo{};
    PipelineLayoutInfo.sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    PipelineLayoutInfo.setLayoutCount         = 1;
    PipelineLayoutInfo.pSetLayouts            = &InGlobalSetLayout;
    PipelineLayoutInfo.pushConstantRangeCount = 0;

    LUMEN_VK_CHECK( vkCreatePipelineLayout( InDevice, &PipelineLayoutInfo, nullptr, &PipelineLayout ) );

    VkPipelineRenderingCreateInfo PipelineRenderingInfo{};
    PipelineRenderingInfo.sType                   = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    PipelineRenderingInfo.colorAttachmentCount    = 1;
    PipelineRenderingInfo.pColorAttachmentFormats = &InColorFormat;

    VkGraphicsPipelineCreateInfo PipelineInfo{};
    PipelineInfo.sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    PipelineInfo.pNext               = &PipelineRenderingInfo;
    PipelineInfo.stageCount          = 2;
    PipelineInfo.pStages             = ShaderStages;
    PipelineInfo.pVertexInputState   = &VertexInputInfo;
    PipelineInfo.pInputAssemblyState = &InputAssembly;
    PipelineInfo.pViewportState      = &ViewportState;
    PipelineInfo.pRasterizationState = &Rasterizer;
    PipelineInfo.pMultisampleState   = &Multisampling;
    PipelineInfo.pColorBlendState    = &ColorBlending;
    PipelineInfo.pDynamicState       = &DynamicState;
    PipelineInfo.layout              = PipelineLayout;
    PipelineInfo.renderPass          = VK_NULL_HANDLE;
    PipelineInfo.subpass             = 0;

    LUMEN_VK_CHECK( vkCreateGraphicsPipelines( InDevice, VK_NULL_HANDLE, 1, &PipelineInfo, nullptr, &Pipeline ) );

    VertexShader.Cleanup( InDevice );
    FragmentShader.Cleanup( InDevice );

    return true;
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