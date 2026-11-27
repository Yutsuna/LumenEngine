/**
 * @file RHITransition.hpp
 * @brief Defines buffer and texture state barriers.
 */

#pragma once
#include <Memory/SharedPtr.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/RHISubresourceRange.hpp>
#include <RHI/Types/RHIQueueType.hpp>
#include <RHIExport.hpp>
#include <Templates/Variant.hpp>
namespace Lumen
{

namespace RHI
{
    struct RHI_EXPORT FRHIBufferTransition
    {
        TSharedPtr<IRHIBuffer> Buffer;
        ERHIResourceState Before       = ERHIResourceState::Undefined;
        ERHIResourceState After        = ERHIResourceState::Undefined;
        ERHIQueueType SourceQueue      = ERHIQueueType::Graphics;
        ERHIQueueType DestinationQueue = ERHIQueueType::Graphics;
        /**
         * @brief Validates the buffer barrier states and queue transfer.
         * @return Success, or a structured FRHIError with the relevant condition.
         */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };
    struct RHI_EXPORT FRHITextureTransition
    {
        TSharedPtr<IRHITexture> Texture;
        FRHISubresourceRange Range;
        ERHIResourceState Before       = ERHIResourceState::Undefined;
        ERHIResourceState After        = ERHIResourceState::Undefined;
        ERHIQueueType SourceQueue      = ERHIQueueType::Graphics;
        ERHIQueueType DestinationQueue = ERHIQueueType::Graphics;
        /**
         * @brief Validates the texture barrier states, subresources and queue transfer.
         * @return Success, or a structured FRHIError with the relevant condition.
         */
        [[nodiscard]] FRHIExpected Validate () const noexcept;
    };
    using FRHITransition = TVariant<FRHIBufferTransition, FRHITextureTransition>;
    /** @return Success when the active barrier is valid. */
    [[nodiscard]] RHI_EXPORT FRHIExpected ValidateRHITransition ( const FRHITransition &InTransition ) noexcept;
} // namespace RHI

} // namespace Lumen
