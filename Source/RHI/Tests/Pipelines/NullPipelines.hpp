/**
 * @file NullPipelines.hpp
 * @brief Declares Null pipeline layouts and pipelines.
 */

#pragma once

#include <Types/RHIObjectForward.hpp>

#include <RHI/Pipelines/IRHIPipeline.hpp>
#include <RHI/Pipelines/IRHIPipelineLayout.hpp>
#include <RHI/Pipelines/RHIComputePipelineDesc.hpp>
#include <RHI/Pipelines/RHIGraphicsPipelineDesc.hpp>

namespace Lumen
{

namespace RHI
{

    namespace Tests
    {

        /**
         * @class FNullPipelineLayout
         * @brief Immutable Null layout.
         */
        class FNullPipelineLayout final : public IRHIPipelineLayout
        {
        public:

            FNullPipelineLayout ( const FRHIPipelineLayoutDesc &InDesc, FRHIObjectId InId ) noexcept;
            LUMEN_RHI_OBJECT_DECLARATIONS;
            const FRHIPipelineLayoutDesc &GetDesc () const noexcept override;
            /** @return Hash of the ordered binding layouts. */
            [[nodiscard]] UInt64 GetHash () const noexcept;

        private:

            FRHIObjectBase Identity;
            FRHIPipelineLayoutDesc Desc;
        };

        /**
         * @class FNullPipeline
         * @brief Immutable Null graphics/compute pipeline.
         */
        class FNullPipeline final : public IRHIPipeline
        {
        public:

            FNullPipeline ( ERHIPipelineType InType, FRHIPipelineCompatibility InCompatibility, FRHIObjectId InId ) noexcept;
            LUMEN_RHI_OBJECT_DECLARATIONS;
            ERHIPipelineType GetType () const noexcept override;
            const FRHIPipelineCompatibility &GetCompatibility () const noexcept override;

        private:

            FRHIObjectBase Identity;
            ERHIPipelineType Type;
            FRHIPipelineCompatibility Compatibility;
        };

    } // namespace Tests

} // namespace RHI

} // namespace Lumen
