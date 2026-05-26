/**
 * @file VulkanPipeline.hpp
 * @brief Declaration of the FVulkanPipeline class for Vulkan Graphics Pipeline management.
 */

#pragma once

#include "CoreTypes.hpp"
#include "Definitions.hpp"

#include "Container/String.hpp"
#include "Container/Vector.hpp"

#include "Container/Expected.hpp"
#include "ErrorCodes.hpp"

#include "RHI/RHITypes.hpp"

#include <vulkan/vulkan_core.h>

namespace LumenEngine
{

namespace VulkanRHI
{

    struct FPipelineShaderDescription final
    {
        /** Optional metadata for debugging */
        FString VertexName;
        FString FragmentName;
    };

    struct FPipelineVertexInputDescription final
    {
        VkVertexInputRate InputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    };

    struct FPipelineInputAssemblyDescription final
    {
        VkPrimitiveTopology Topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        Bool bEnablePrimitiveRestart = false;
    };

    struct FPipelineRasterizationDescription final
    {
        VkPolygonMode PolygonMode     = VK_POLYGON_MODE_FILL;
        VkCullModeFlags CullMode      = VK_CULL_MODE_NONE;
        VkFrontFace FrontFace         = VK_FRONT_FACE_CLOCKWISE;
        Float32 LineWidth             = 1.0F;
        Bool bEnableDepthClamp        = false;
        Bool bEnableRasterizerDiscard = false;
        Bool bEnableDepthBias         = false;
    };

    struct FPipelineMultisampleDescription final
    {
        VkSampleCountFlagBits RasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        Bool bEnableSampleShading                  = false;
    };

    struct FPipelineColorBlendDescription final
    {
        Bool bEnableBlend               = false;
        Bool bEnableLogicOp             = false;
        VkLogicOp LogicOp               = VK_LOGIC_OP_COPY;
        VkColorComponentFlags WriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    };

    struct FPipelineDynamicStateDescription final
    {
        TVector<VkDynamicState> States = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
    };

    struct FPipelineDepthStencilDescription final
    {
        Bool bEnableDepthTest            = false;
        Bool bEnableDepthWrite           = false;
        Bool bEnableDepthBoundsTest      = false;
        Bool bEnableStencilTest          = false;
        VkCompareOp DepthCompareOp       = VK_COMPARE_OP_LESS;
        Float32 MinDepthBounds           = 0.0F;
        Float32 MaxDepthBounds           = 1.0F;
        VkFormat DepthAttachmentFormat   = VK_FORMAT_UNDEFINED;
        VkFormat StencilAttachmentFormat = VK_FORMAT_UNDEFINED;
    };

    struct FPipelinePushConstantRangeDescription final
    {
        VkShaderStageFlags StageFlags = VK_SHADER_STAGE_ALL_GRAPHICS;
        UInt32 Offset                 = 0;
        UInt32 Size                   = 0;
    };

    struct FPipelineDescription final
    {
        VkFormat ColorFormat                  = VK_FORMAT_B8G8R8A8_SRGB;
        VkDescriptorSetLayout GlobalSetLayout = VK_NULL_HANDLE;
        FPipelineShaderDescription Shader;
        FPipelineVertexInputDescription VertexInput;
        FPipelineInputAssemblyDescription InputAssembly;
        FPipelineRasterizationDescription Rasterization;
        FPipelineMultisampleDescription Multisample;
        FPipelineColorBlendDescription ColorBlend;
        FPipelineDynamicStateDescription DynamicState;
        FPipelineDepthStencilDescription DepthStencil;
        TVector<FPipelinePushConstantRangeDescription> PushConstantRanges;
    };

    /**
     * @class FVulkanPipeline
     * @brief Encapsulates Vulkan graphics pipeline creation, management, and cleanup.
     */
    class LUMEN_ENGINE_API FVulkanPipeline final
    {

    public:

        FVulkanPipeline () noexcept  = default;
        ~FVulkanPipeline () noexcept = default;

    public:

        /**
         * @brief Initializes the Vulkan pipeline.
         * @param InDevice The Vulkan device.
         * @param InDescription Logical state description used to build the graphics pipeline.
         * @param InVertexSpirV The SPIR-V bytecode for the vertex shader.
         * @param InFragmentSpirV The SPIR-V bytecode for the fragment shader.
         * @return Success or a detailed initialization error code.
         */
        [[nodiscard]] TExpected<void, EErrorCode::Type> Initialize ( VkDevice InDevice,
                                                                     const FPipelineDescription &InDescription,
                                                                     const RHI::FShaderByteCode &InVertexSpirV,
                                                                     const RHI::FShaderByteCode &InFragmentSpirV );

        /** @brief Recreates the pipeline with a new sample count. */
        TExpected<void, EErrorCode::Type> Recreate ( VkDevice InDevice, VkSampleCountFlagBits InSamples );

        /** @brief Recreates the pipeline with a new sample count and returns the old handles for deferred cleanup. */
        TExpected<void, EErrorCode::Type> Recreate ( VkDevice InDevice, VkSampleCountFlagBits InSamples, VkPipeline &OutOldPipeline, VkPipelineLayout &OutOldLayout );

        /** @brief Convenience factory for a default pipeline description. */
        [[nodiscard]] static FPipelineDescription
        CreateDefaultDescription ( VkFormat InColorFormat, VkDescriptorSetLayout InGlobalSetLayout, VkSampleCountFlagBits InSamples = VK_SAMPLE_COUNT_1_BIT );

        /**
         * @brief Cleans up the Vulkan pipeline.
         * @param InDevice The Vulkan device.
         */
        void Cleanup ( VkDevice InDevice ) noexcept;

        /**
         * @brief Binds the Vulkan pipeline.
         * @param InCommandBuffer The command buffer.
         */
        void Bind ( VkCommandBuffer InCommandBuffer ) const noexcept;

    public:

        /** @brief Retrieve the layout to bind descriptor sets. */
        [[nodiscard]] VkPipelineLayout GetLayout () const noexcept;

        /** @brief Retrieve the Vulkan pipeline handle for drawing. */
        [[nodiscard]] VkPipeline GetPipelineHandle () const noexcept;

    private:

        VkPipeline Pipeline             = VK_NULL_HANDLE;
        VkPipelineLayout PipelineLayout = VK_NULL_HANDLE;

        /** Caching descriptors and shader bytecodes to support seamless on-demand recreation */
        FPipelineDescription DescriptionCapped;
        RHI::FShaderByteCode VertexSpirV;
        RHI::FShaderByteCode FragmentSpirV;
    };

} // namespace VulkanRHI

} // namespace LumenEngine
