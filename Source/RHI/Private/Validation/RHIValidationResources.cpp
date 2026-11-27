/**
 * @file RHIValidationResources.cpp
 * @brief Implements resource forwarding and validation lifetime checks.
 */

#include "RHIValidationResources.hpp"

/**
 * ValidationHeap
 */

Lumen::RHI::FRHIValidationHeap::FRHIValidationHeap ( TSharedRef<IRHIHeap> InUnderlying,
                                                     TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::Heap );
}

Lumen::RHI::FRHIValidationHeap::~FRHIValidationHeap () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationHeap )

const Lumen::RHI::FRHIHeapDesc &Lumen::RHI::FRHIValidationHeap::GetDesc () const noexcept
{
    return Underlying->GetDesc();
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationHeap::Map ( const UInt64 InOffset, const UInt64 InSize ) noexcept
{
    return Underlying->Map( InOffset, InSize );
}

void Lumen::RHI::FRHIValidationHeap::Unmap () noexcept
{
    Underlying->Unmap();
}

/**
 * ValidationBuffer
 */

Lumen::RHI::FRHIValidationBuffer::FRHIValidationBuffer ( TSharedRef<IRHIBuffer> InUnderlying,
                                                         TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->TrackResource( *Underlying, ERHIValidationObjectKind::Buffer );
}

Lumen::RHI::FRHIValidationBuffer::~FRHIValidationBuffer () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationBuffer )

const Lumen::RHI::FRHIBufferDesc &Lumen::RHI::FRHIValidationBuffer::GetDesc () const noexcept
{
    return Underlying->GetDesc();
}

const Lumen::RHI::FRHIResourceRequirements &Lumen::RHI::FRHIValidationBuffer::GetRequirements () const noexcept
{
    return Underlying->GetRequirements();
}

Lumen::RHI::ERHIResourceUsage Lumen::RHI::FRHIValidationBuffer::GetUsage () const noexcept
{
    return Underlying->GetUsage();
}

Lumen::RHI::ERHIResourceState Lumen::RHI::FRHIValidationBuffer::GetInitialState () const noexcept
{
    return Underlying->GetInitialState();
}

Lumen::RHI::FRHIResourceLifetime Lumen::RHI::FRHIValidationBuffer::GetLifetime () const noexcept
{
    return Underlying->GetLifetime();
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationBuffer::Map ( const UInt64 InOffset, const UInt64 InSize ) noexcept
{
    if ( bMapped )
    {
        return TUnexpected<FRHIError>( State->GetReporter()
                                           ->Reject( ERHIValidationMessageCode::InvalidCommandState, ERHIErrorCondition::Resource,
                                                     "Buffer is already mapped", this )
                                           .Error() );
    }
    return Underlying->Map( InOffset, InSize )
        .Inspect( [this] ( const TSpan<Byte> & ) noexcept
    {
        bMapped = true;
    } );
}

void Lumen::RHI::FRHIValidationBuffer::Unmap () noexcept
{
    Underlying->Unmap();
    bMapped = false;
}

/**
 * ValidationTexture
 */

Lumen::RHI::FRHIValidationTexture::FRHIValidationTexture ( TSharedRef<IRHITexture> InUnderlying,
                                                           TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->TrackResource( *Underlying, ERHIValidationObjectKind::Texture );
}

Lumen::RHI::FRHIValidationTexture::~FRHIValidationTexture () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationTexture )

const Lumen::RHI::FRHITextureDesc &Lumen::RHI::FRHIValidationTexture::GetDesc () const noexcept
{
    return Underlying->GetDesc();
}

const Lumen::RHI::FRHIResourceRequirements &Lumen::RHI::FRHIValidationTexture::GetRequirements () const noexcept
{
    return Underlying->GetRequirements();
}

Lumen::RHI::ERHIResourceUsage Lumen::RHI::FRHIValidationTexture::GetUsage () const noexcept
{
    return Underlying->GetUsage();
}

Lumen::RHI::ERHIResourceState Lumen::RHI::FRHIValidationTexture::GetInitialState () const noexcept
{
    return Underlying->GetInitialState();
}

Lumen::RHI::FRHIResourceLifetime Lumen::RHI::FRHIValidationTexture::GetLifetime () const noexcept
{
    return Underlying->GetLifetime();
}

/**
 * ValidationTextureView
 */

Lumen::RHI::FRHIValidationTextureView::FRHIValidationTextureView ( TSharedRef<IRHITextureView> InUnderlying,
                                                                   TSharedPtr<IRHITexture> InParent,
                                                                   TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), Parent( std::move( InParent ) ), State( std::move( InState ) ),
      Desc( Underlying->GetDesc() )
{
    Desc.Texture = Parent;
    State->Track( *Underlying, ERHIValidationObjectKind::TextureView );
}

Lumen::RHI::FRHIValidationTextureView::~FRHIValidationTextureView () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationTextureView )

const Lumen::RHI::FRHITextureViewDesc &Lumen::RHI::FRHIValidationTextureView::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TSharedPtr<Lumen::RHI::IRHITexture> Lumen::RHI::FRHIValidationTextureView::GetTexture () const noexcept
{
    return Parent;
}

Lumen::RHI::FRHIValidationSampler::FRHIValidationSampler ( TSharedRef<IRHISampler> InUnderlying,
                                                           TSharedPtr<FRHIValidationState> InState ) noexcept
    : Underlying( std::move( InUnderlying ) ), State( std::move( InState ) )
{
    State->Track( *Underlying, ERHIValidationObjectKind::Sampler );
}

Lumen::RHI::FRHIValidationSampler::~FRHIValidationSampler () noexcept
{
    State->Untrack( *Underlying );
}

LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( FRHIValidationSampler )

const Lumen::RHI::FRHISamplerDesc &Lumen::RHI::FRHIValidationSampler::GetDesc () const noexcept
{
    return Underlying->GetDesc();
}
