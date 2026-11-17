/**
 * @file RHIValidationResources.cpp
 * @brief Implements resource forwarding and validation lifetime checks.
 */
#include "RHIValidationResources.hpp"

Lumen::FRHIValidationBuffer::FRHIValidationBuffer ( TSharedRef<IRHIBuffer> InUnderlying,
                                                    TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->TrackResource( *Underlying, ERHIValidationObjectKind::Buffer );
}

Lumen::FRHIValidationBuffer::~FRHIValidationBuffer () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationBuffer )

const Lumen::FRHIBufferDesc &Lumen::FRHIValidationBuffer::GetDesc () const noexcept
{
    return Underlying->GetDesc();
}

const Lumen::FRHIResourceRequirements &Lumen::FRHIValidationBuffer::GetRequirements () const noexcept
{
    return Underlying->GetRequirements();
}

Lumen::ERHIResourceUsage Lumen::FRHIValidationBuffer::GetUsage () const noexcept
{
    return Underlying->GetUsage();
}

Lumen::ERHIResourceState Lumen::FRHIValidationBuffer::GetInitialState () const noexcept
{
    return Underlying->GetInitialState();
}

Lumen::FRHIResourceLifetime Lumen::FRHIValidationBuffer::GetLifetime () const noexcept
{
    return Underlying->GetLifetime();
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::FRHIError> Lumen::FRHIValidationBuffer::Map ( const UInt64 InOffset,
                                                                                                 const UInt64 InSize ) noexcept
{
    if ( bMapped )
    {
        return TUnexpected<FRHIError>( State->GetReporter()
                                           ->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Resource,
                                                     "Buffer is already mapped", this )
                                           .Error() );
    }
    auto Result = Underlying->Map( InOffset, InSize );
    if ( Result )
    {
        bMapped = true;
    }
    return Result;
}

void Lumen::FRHIValidationBuffer::Unmap () noexcept
{
    Underlying->Unmap();
    bMapped = false;
}

Lumen::FRHIValidationTexture::FRHIValidationTexture ( TSharedRef<IRHITexture> InUnderlying,
                                                      TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->TrackResource( *Underlying, ERHIValidationObjectKind::Texture );
}

Lumen::FRHIValidationTexture::~FRHIValidationTexture () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationTexture )

const Lumen::FRHITextureDesc &Lumen::FRHIValidationTexture::GetDesc () const noexcept
{
    return Underlying->GetDesc();
}

const Lumen::FRHIResourceRequirements &Lumen::FRHIValidationTexture::GetRequirements () const noexcept
{
    return Underlying->GetRequirements();
}

Lumen::ERHIResourceUsage Lumen::FRHIValidationTexture::GetUsage () const noexcept
{
    return Underlying->GetUsage();
}

Lumen::ERHIResourceState Lumen::FRHIValidationTexture::GetInitialState () const noexcept
{
    return Underlying->GetInitialState();
}

Lumen::FRHIResourceLifetime Lumen::FRHIValidationTexture::GetLifetime () const noexcept
{
    return Underlying->GetLifetime();
}

Lumen::FRHIValidationTextureView::FRHIValidationTextureView ( TSharedRef<IRHITextureView> InUnderlying,
                                                              TSharedPtr<IRHITexture> InParent,
                                                              TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), Parent( std::move( InParent ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::TextureView );
}

Lumen::FRHIValidationTextureView::~FRHIValidationTextureView () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationTextureView )

const Lumen::FRHITextureViewDesc &Lumen::FRHIValidationTextureView::GetDesc () const noexcept
{
    return Underlying->GetDesc();
}

Lumen::TSharedPtr<Lumen::IRHITexture> Lumen::FRHIValidationTextureView::GetTexture () const noexcept
{
    return Parent;
}

Lumen::FRHIValidationSampler::FRHIValidationSampler ( TSharedRef<IRHISampler> InUnderlying,
                                                      TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::Sampler );
}

Lumen::FRHIValidationSampler::~FRHIValidationSampler () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationSampler )

const Lumen::FRHISamplerDesc &Lumen::FRHIValidationSampler::GetDesc () const noexcept
{
    return Underlying->GetDesc();
}
