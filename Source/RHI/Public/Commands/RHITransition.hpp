/**
 * @file RHITransition.hpp
 * @brief Defines buffer and texture state barriers.
 */

#pragma once
#include <Memory/SharedPtr.hpp>
#include <Resources/IRHIBuffer.hpp>
#include <Resources/IRHITexture.hpp>
#include <Resources/RHISubresourceRange.hpp>
#include <Templates/Variant.hpp>
#include <Types/RHIQueueType.hpp>
namespace Lumen
{
struct RHI_EXPORT FRHIBufferTransition
{
    TSharedPtr<IRHIBuffer> Buffer;
    ERHIResourceState Before       = ERHIResourceState::Undefined;
    ERHIResourceState After        = ERHIResourceState::Undefined;
    ERHIQueueType SourceQueue      = ERHIQueueType::Graphics;
    ERHIQueueType DestinationQueue = ERHIQueueType::Graphics;
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
struct RHI_EXPORT FRHITextureTransition
{
    TSharedPtr<IRHITexture> Texture;
    FRHISubresourceRange Range;
    ERHIResourceState Before       = ERHIResourceState::Undefined;
    ERHIResourceState After        = ERHIResourceState::Undefined;
    ERHIQueueType SourceQueue      = ERHIQueueType::Graphics;
    ERHIQueueType DestinationQueue = ERHIQueueType::Graphics;
    [[nodiscard]] TExpected<void, FRHIError> Validate () const noexcept;
};
using FRHITransition = TVariant<FRHIBufferTransition, FRHITextureTransition>;
/** @return Success when the active barrier is valid. */
[[nodiscard]] RHI_EXPORT TExpected<void, FRHIError> ValidateRHITransition ( const FRHITransition &InTransition ) noexcept;
} // namespace Lumen
