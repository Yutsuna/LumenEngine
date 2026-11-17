/**
 * @file RHITextureViewDesc.cpp
 * @brief Implements texture-view compatibility validation.
 */
#include <Resources/IRHITexture.hpp>
#include <Resources/RHITextureViewDesc.hpp>
Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHITextureViewDesc::Validate () const noexcept
{
    if ( not Texture or not Range.IsValid() )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidResource( "Texture view requires a texture and range" ) );
    }
    const FRHITextureDesc &Parent = Texture->GetDesc();
    if ( not Range.IsContainedIn( Parent.MipCount, Parent.LayerCount ) or
         ( Format != ERHIFormat::Unknown and Format != Parent.Format ) )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidResource( "Texture view is outside or incompatible with its parent" ) );
    }
    return {};
}
