/**
 * @file NullResources.cpp
 * @brief Implements simulated RHI resources.
 */

#include <Resources/NullResources.hpp>
#include <Templates/Hash.hpp>

/**
 * Public
 */

Lumen::Tests::FNullBuffer::FNullBuffer ( const FRHIBufferDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullBuffer>() ), Desc( InDesc ),
      Requirements{
          .Size = InDesc.Size, .Alignment = 16ULL, .PreferredMemoryClass = TOptional<ERHIMemoryClass>( InDesc.MemoryClass ) },
      Storage( MakeUnique<Byte[]>( static_cast<USize>( InDesc.Size ) ) )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullBuffer )
const Lumen::FRHIBufferDesc &Lumen::Tests::FNullBuffer::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TExpected<Lumen::TSpan<Lumen::Byte>, Lumen::FRHIError> Lumen::Tests::FNullBuffer::Map ( const UInt64 InOffset,
                                                                                               const UInt64 InSize ) noexcept
{
    if ( not IsRHICpuAccessible( Desc.MemoryClass ) or InOffset > Desc.Size or InSize > Desc.Size - InOffset )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Resource, "Invalid Null buffer mapping" ) );
    }
    return TSpan<Byte>( Storage.Get() + static_cast<USize>( InOffset ), static_cast<USize>( InSize ) );
}

const Lumen::FRHIResourceRequirements &Lumen::Tests::FNullBuffer::GetRequirements () const noexcept
{
    return Requirements;
}

Lumen::ERHIResourceUsage Lumen::Tests::FNullBuffer::GetUsage () const noexcept
{
    return Desc.Usage;
}

Lumen::ERHIResourceState Lumen::Tests::FNullBuffer::GetInitialState () const noexcept
{
    return Desc.InitialState;
}

Lumen::FRHIResourceLifetime Lumen::Tests::FNullBuffer::GetLifetime () const noexcept
{
    return {};
}

Lumen::Tests::FNullTexture::FNullTexture ( const FRHITextureDesc &InDesc,
                                           const FRHIResourceRequirements InRequirements,
                                           const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullTexture>() ), Desc( InDesc ), Requirements( InRequirements )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullTexture )
const Lumen::FRHITextureDesc &Lumen::Tests::FNullTexture::GetDesc () const noexcept
{
    return Desc;
}

const Lumen::FRHIResourceRequirements &Lumen::Tests::FNullTexture::GetRequirements () const noexcept
{
    return Requirements;
}

Lumen::ERHIResourceUsage Lumen::Tests::FNullTexture::GetUsage () const noexcept
{
    return Desc.Usage;
}

Lumen::ERHIResourceState Lumen::Tests::FNullTexture::GetInitialState () const noexcept
{
    return Desc.InitialState;
}

Lumen::FRHIResourceLifetime Lumen::Tests::FNullTexture::GetLifetime () const noexcept
{
    return {};
}

Lumen::Tests::FNullTextureView::FNullTextureView ( const FRHITextureViewDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullTextureView>() ), Desc( InDesc )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullTextureView )
const Lumen::FRHITextureViewDesc &Lumen::Tests::FNullTextureView::GetDesc () const noexcept
{
    return Desc;
}

Lumen::TSharedPtr<Lumen::IRHITexture> Lumen::Tests::FNullTextureView::GetTexture () const noexcept
{
    return Desc.Texture;
}

Lumen::Tests::FNullSampler::FNullSampler ( const FRHISamplerDesc &InDesc, const FRHIObjectId InId ) noexcept
    : Identity( InId, FHash::TypeHash<FNullSampler>() ), Desc( InDesc )
{
    /* */
}

LUMEN_RHI_OBJECT_FORWARDERS( Tests::FNullSampler )
const Lumen::FRHISamplerDesc &Lumen::Tests::FNullSampler::GetDesc () const noexcept
{
    return Desc;
}
