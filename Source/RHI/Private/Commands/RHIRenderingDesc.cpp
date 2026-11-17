/**
 * @file RHIRenderingDesc.cpp
 * @brief Implements dynamic-rendering descriptor validation.
 */

#include <Commands/RHIRenderingDesc.hpp>
#include <Types/RHIFormatInfo.hpp>

#include <Algo/ForEach.hpp>

#include <variant>

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIRenderingAttachment::Validate ( const Bool bDepthStencil ) const noexcept
{
    const TSharedPtr<IRHITexture> Texture = View ? View->GetTexture() : TSharedPtr<IRHITexture>{};
    if ( not Texture or LoadOp > ERHIRenderingLoadOp::DontCare or StoreOp > ERHIRenderingStoreOp::DontCare )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid rendering attachment" ) );
    }
    ERHIFormat Format = View->GetDesc().Format;
    if ( Format == ERHIFormat::Unknown )
    {
        Format = Texture->GetDesc().Format;
    }

    const Lumen::TOptional<Lumen::FRHIFormatInfo> Info = TryGetRHIFormatInfo( Format );
    const Bool bAspectValid = Info and ( bDepthStencil ? ( Info->HasDepth() or Info->HasStencil() ) : Info->HasColor() );
    const Bool bClearValid  = bDepthStencil ? std::holds_alternative<FRHIDepthStencilClearValue>( ClearValue )
                                            : std::holds_alternative<FRHIColorClearValue>( ClearValue );
    if ( not bAspectValid or not bClearValid )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Attachment format or clear type is invalid" ) );
    }
    return {};
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIRenderingDesc::Validate () const noexcept
{
    if ( not Area.IsValid() or ( ColorAttachments.empty() and not DepthStencilAttachment.View ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Rendering area or attachments are empty" ) );
    }
    Algo::ForEach( ColorAttachments,
                   [] ( const FRHIRenderingAttachment &Attachment ) noexcept -> Lumen::TExpected<void, Lumen::FRHIError>
    {
        const Lumen::TExpected<void, Lumen::FRHIError> Valid = Attachment.Validate( false );
        if ( not Valid )
        {
            return Valid;
        }
    } );
    if ( DepthStencilAttachment.View )
    {
        const Lumen::TExpected<void, Lumen::FRHIError> Valid = DepthStencilAttachment.Validate( true );
        if ( not Valid )
        {
            return Valid;
        }
    }

    const FRHIRenderingAttachment *First  = ColorAttachments.empty() ? &DepthStencilAttachment : ColorAttachments.data();
    const TSharedPtr<IRHITexture> Texture = First->View->GetTexture();
    const FRHIExtent3D MipExtent          = Texture->GetDesc().Extent;
    const UInt32 Mip                      = First->View->GetDesc().Range.BaseMip;
    const UInt32 Width                    = MipExtent.Width > Mip ? ( MipExtent.Width >> Mip ) : 1U;
    const UInt32 Height                   = MipExtent.Height > Mip ? ( MipExtent.Height >> Mip ) : 1U;

    if ( Area.Position.X < 0 or Area.Position.Y < 0 or static_cast<UInt32>( Area.Position.X ) + Area.Size.Width > Width or
         static_cast<UInt32>( Area.Position.Y ) + Area.Size.Height > Height )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Rendering area exceeds attachment extent" ) );
    }

    Algo::ForEach( ColorAttachments, [&] ( const auto &Attachment ) noexcept -> TExpected<void, FRHIError>
    {
        if ( Attachment.View->GetTexture()->GetDesc().SampleCount != Texture->GetDesc().SampleCount )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Attachment sample counts differ" ) );
        }
    } );

    if ( DepthStencilAttachment.View and
         DepthStencilAttachment.View->GetTexture()->GetDesc().SampleCount != Texture->GetDesc().SampleCount )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Attachment sample counts differ" ) );
    }
    return {};
}
