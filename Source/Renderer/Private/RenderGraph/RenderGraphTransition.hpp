/**
 * @file RenderGraphTransition.hpp
 * @brief Shared materialization of an RHI barrier from graph desc and binding variants.
 */

#pragma once

#include <RenderGraph/RenderGraphTypes.hpp>

#include <Meta/Overloaded.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Unexpected.hpp>

#include <variant>

namespace Lumen
{

/**
 * @brief Builds and validates one RHI transition from a resource's creation desc and binding.
 * @details Keeps the desc/binding type pairing in one place: a buffer desc paired with a texture
 *          binding (or vice versa) is reported as an error instead of throwing from std::get.
 */
[[nodiscard]] inline TExpected<RHI::FRHITransition, FRenderError>
MakeBoundTransition ( const FRenderResourceHandle InResource,
                      const FRenderResourceDesc &InDesc,
                      const FRenderResourceBinding &InBinding,
                      const RHI::FRHISubresourceRange &InRange,
                      const RHI::ERHIResourceState InBefore,
                      const RHI::ERHIResourceState InAfter,
                      const RHI::ERHIQueueType InSourceQueue,
                      const RHI::ERHIQueueType InDestinationQueue,
                      const ERenderErrorCode InValidationErrorCode ) noexcept
{
    const auto MakeError = [InResource] ( const ERenderErrorCode InCode ) noexcept -> TUnexpected<FRenderError>
    {
        return MakeUnexpected( FRenderError{ .Code = InCode, .ResourceIndex = InResource.Index } );
    };

    return std::visit(
        Meta::FOverloaded{
            [&] ( const RHI::FRHIBufferDesc &,
                  const TSharedPtr<RHI::IRHIBuffer> &InBuffer ) noexcept -> TExpected<RHI::FRHITransition, FRenderError>
    {
        if ( not InBuffer )
        {
            return MakeError( ERenderErrorCode::TransientResourceNotBound );
        }
        const RHI::FRHIBufferTransition Transition{
            .Buffer           = InBuffer,
            .Before           = InBefore,
            .After            = InAfter,
            .SourceQueue      = InSourceQueue,
            .DestinationQueue = InDestinationQueue,
        };
        if ( not Transition.Validate() )
        {
            return MakeError( InValidationErrorCode );
        }
        return RHI::FRHITransition( Transition );
    },
            [&] ( const RHI::FRHITextureDesc &,
                  const TSharedPtr<RHI::IRHITexture> &InTexture ) noexcept -> TExpected<RHI::FRHITransition, FRenderError>
    {
        if ( not InTexture )
        {
            return MakeError( ERenderErrorCode::TransientResourceNotBound );
        }
        const RHI::FRHITextureTransition Transition{
            .Texture          = InTexture,
            .Range            = InRange,
            .Before           = InBefore,
            .After            = InAfter,
            .SourceQueue      = InSourceQueue,
            .DestinationQueue = InDestinationQueue,
        };
        if ( not Transition.Validate() )
        {
            return MakeError( InValidationErrorCode );
        }
        return RHI::FRHITransition( Transition );
    },
            [&] ( const auto &, const auto & ) noexcept -> TExpected<RHI::FRHITransition, FRenderError>
    {
        return MakeError( ERenderErrorCode::ResourceTypeMismatch );
    },
        },
        InDesc.Creation, InBinding.Resource );
}

} // namespace Lumen
