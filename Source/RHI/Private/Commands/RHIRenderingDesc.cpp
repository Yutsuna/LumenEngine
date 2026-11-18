/**
 * @file RHIRenderingDesc.cpp
 * @brief Implements dynamic-rendering descriptor validation.
 */

#include <RHI/Commands/RHIRenderingDesc.hpp>
#include <RHI/Types/RHIFormatInfo.hpp>

#include <variant>

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIRenderingAttachment::Validate ( const Bool bDepthStencil ) const noexcept
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

    const Lumen::TOptional<Lumen::RHI::FRHIFormatInfo> Info = TryGetRHIFormatInfo( Format );
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

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIRenderingDesc::Validate () const noexcept
{
    if ( not Area.IsValid() or ( ColorAttachments.empty() and not DepthStencilAttachment.View ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Rendering area or attachments are empty" ) );
    }

    for ( const FRHIRenderingAttachment &Attachment : ColorAttachments )
    {
        const Lumen::TExpected<void, Lumen::RHI::FRHIError> Valid = Attachment.Validate( false );
        if ( not Valid )
        {
            return Valid;
        }
    }
    if ( DepthStencilAttachment.View )
    {
        const Lumen::TExpected<void, Lumen::RHI::FRHIError> Valid = DepthStencilAttachment.Validate( true );
        if ( not Valid )
        {
            return Valid;
        }
    }

    const FRHIRenderingAttachment *First  = ColorAttachments.empty() ? &DepthStencilAttachment : ColorAttachments.data();
    const TSharedPtr<IRHITexture> Texture = First->View->GetTexture();
    const FRHITextureDesc &TextureDesc    = Texture->GetDesc();
    const FRHITextureViewDesc &ViewDesc   = First->View->GetDesc();

    /** NOTE: GetMaxMipLevelCount() is at most 32, so BaseMip < 32 here and both shifts below are well-defined. */
    if ( ViewDesc.Range.BaseMip >= TextureDesc.MipCount or ViewDesc.Range.BaseMip >= TextureDesc.Extent.GetMaxMipLevelCount() or
         not ViewDesc.Range.IsContainedIn( TextureDesc.MipCount, TextureDesc.LayerCount ) )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Attachment mip level is outside the texture mip chain" ) );
    }

    const UInt32 Mip    = ViewDesc.Range.BaseMip;
    const UInt32 Width  = TextureDesc.Extent.Width > Mip ? ( TextureDesc.Extent.Width >> Mip ) : 1U;
    const UInt32 Height = TextureDesc.Extent.Height > Mip ? ( TextureDesc.Extent.Height >> Mip ) : 1U;

    if ( Area.Position.X < 0 or Area.Position.Y < 0 or static_cast<UInt64>( Area.Position.X ) + Area.Size.Width > Width or
         static_cast<UInt64>( Area.Position.Y ) + Area.Size.Height > Height )
    {
        return TUnexpected<FRHIError>(
            MakeRHIInvalidArgument( ERHIErrorCondition::Command, "Rendering area exceeds attachment extent" ) );
    }

    for ( const FRHIRenderingAttachment &Attachment : ColorAttachments )
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
