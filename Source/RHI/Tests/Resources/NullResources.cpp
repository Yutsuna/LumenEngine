/**
 * @file NullResources.cpp
 * @brief Implements simulated RHI resources.
 */

#include <Resources/NullResources.hpp>
#include <Templates/Hash.hpp>

/**
 * Public
 */

Lumen::RHI::Tests::FNullBuffer::FNullBuffer ( const FRHIBufferDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullBuffer>() ), Desc( InDesc ),
      Requirements{
          .Size = InDesc.Size, .Alignment = 16ULL, .PreferredMemoryClass = TOptional<ERHIMemoryClass>( InDesc.MemoryClass ) },
      Storage( MakeUnique<Byte[]>( static_cast<USize>( InDesc.Size ) ) )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullBuffer )
const Lumen::RHI::FRHIBufferDesc &Lumen::RHI::Tests::FNullBuffer::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::RHI::FRHIError>
Lumen::RHI::Tests::FNullBuffer::Map ( const UInt64 InOffset, const UInt64 InSize ) noexcept
{
    if ( not IsRHICpuAccessible( Desc.MemoryClass ) or InOffset > Desc.Size or InSize > Desc.Size - InOffset )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidResource( "Invalid Null buffer mapping" ) );
    }
    return TSpan<Byte>( Storage.Get() + static_cast<USize>( InOffset ), static_cast<USize>( InSize ) );
}

const Lumen::RHI::FRHIResourceRequirements &Lumen::RHI::Tests::FNullBuffer::GetRequirements () const noexcept
{
    return Requirements;
}

Lumen::RHI::ERHIResourceUsage Lumen::RHI::Tests::FNullBuffer::GetUsage () const noexcept
{
    return Desc.Usage;
}

Lumen::RHI::ERHIResourceState Lumen::RHI::Tests::FNullBuffer::GetInitialState () const noexcept
{
    return Desc.InitialState;
}

Lumen::RHI::FRHIResourceLifetime Lumen::RHI::Tests::FNullBuffer::GetLifetime () const noexcept
{
    return {};
}

Lumen::RHI::Tests::FNullTexture::FNullTexture ( const FRHITextureDesc &InDesc,
                                                const FRHIResourceRequirements InRequirements,
                                                const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullTexture>() ), Desc( InDesc ), Requirements( InRequirements )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullTexture )
const Lumen::RHI::FRHITextureDesc &Lumen::RHI::Tests::FNullTexture::GetDesc () const noexcept
{
    return Desc;
}

const Lumen::RHI::FRHIResourceRequirements &Lumen::RHI::Tests::FNullTexture::GetRequirements () const noexcept
{
    return Requirements;
}

Lumen::RHI::ERHIResourceUsage Lumen::RHI::Tests::FNullTexture::GetUsage () const noexcept
{
    return Desc.Usage;
}

Lumen::RHI::ERHIResourceState Lumen::RHI::Tests::FNullTexture::GetInitialState () const noexcept
{
    return Desc.InitialState;
}

Lumen::RHI::FRHIResourceLifetime Lumen::RHI::Tests::FNullTexture::GetLifetime () const noexcept
{
    return {};
}

Lumen::RHI::Tests::FNullTextureView::FNullTextureView ( const FRHITextureViewDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullTextureView>() ), Desc( InDesc )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullTextureView )
const Lumen::RHI::FRHITextureViewDesc &Lumen::RHI::Tests::FNullTextureView::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TSharedPtr<Lumen::RHI::IRHITexture> Lumen::RHI::Tests::FNullTextureView::GetTexture () const noexcept
{
    return Desc.Texture;
}

Lumen::RHI::Tests::FNullSampler::FNullSampler ( const FRHISamplerDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullSampler>() ), Desc( InDesc )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullSampler )
const Lumen::RHI::FRHISamplerDesc &Lumen::RHI::Tests::FNullSampler::GetDesc () const noexcept
{
    return Desc;
}
