/**
 * @file RHIRenderingDesc.cpp
 * @brief Implements dynamic-rendering descriptor validation.
 */

#include <Commands/RHIRenderingDesc.hpp>
#include <Types/RHIFormatInfo.hpp>
#include <variant>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIRenderingAttachment::Validate ( const Bool bDepthStencil ) const noexcept
{
    const auto Texture = View ? View->GetTexture() : TSharedPtr<IRHITexture>{};
    if ( not Texture or LoadOp > ERHIRenderingLoadOp::DontCare or StoreOp > ERHIRenderingStoreOp::DontCare )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Invalid rendering attachment" ) );
    }
    ERHIFormat Format = View->GetDesc().Format;
    if ( Format == ERHIFormat::Unknown )
    {
        Format = Texture->GetDesc().Format;
    }
    const auto Info         = TryGetRHIFormatInfo( Format );
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
    for ( const auto &Attachment : ColorAttachments )
    {
        const auto Valid = Attachment.Validate( false );
        if ( not Valid )
        {
            return Valid;
        }
    }
    if ( DepthStencilAttachment.View )
    {
        const auto Valid = DepthStencilAttachment.Validate( true );
        if ( not Valid )
        {
            return Valid;
        }
    }
    const FRHIRenderingAttachment *First = ColorAttachments.empty() ? &DepthStencilAttachment : ColorAttachments.data();
    const auto Texture                   = First->View->GetTexture();
    const auto MipExtent                 = Texture->GetDesc().Extent;
    const UInt32 Mip                     = First->View->GetDesc().Range.BaseMip;
    const UInt32 Width                   = MipExtent.Width > Mip ? ( MipExtent.Width >> Mip ) : 1U;
    const UInt32 Height                  = MipExtent.Height > Mip ? ( MipExtent.Height >> Mip ) : 1U;
    if ( Area.X < 0 or Area.Y < 0 or static_cast<UInt32>( Area.X ) + Area.Width > Width or
         static_cast<UInt32>( Area.Y ) + Area.Height > Height )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Rendering area exceeds attachment extent" ) );
    }
    for ( const auto &Attachment : ColorAttachments )
    {
        if ( Attachment.View->GetTexture()->GetDesc().SampleCount != Texture->GetDesc().SampleCount )
        {
            return TUnexpected<FRHIError>(
                MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Attachment sample counts differ" ) );
        }
    }
    if ( DepthStencilAttachment.View and
         DepthStencilAttachment.View->GetTexture()->GetDesc().SampleCount != Texture->GetDesc().SampleCount )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Attachment sample counts differ" ) );
    }
    return {};
}
