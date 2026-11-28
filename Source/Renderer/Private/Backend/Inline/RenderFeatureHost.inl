/**
 * @file RenderFeatureHost.inl
 * @brief Inline and template implementations of the private feature objects.
 */

#pragma once

#include <Backend/RenderFeatureHost.hpp>

inline Lumen::FRenderTextureObject::FRenderTextureObject ( TSharedRef<RHI::IRHITexture> InTexture,
                                                           RHI::ERHIResourceState InState ) noexcept
    : Texture( InTexture ), CommittedState( InState )
{
}

inline Lumen::FRenderTextureViewObject::FRenderTextureViewObject ( TSharedRef<RHI::IRHITextureView> InView,
                                                                   TSharedPtr<FRenderTextureObject> InTexture ) noexcept
    : View( InView ), Texture( InTexture )
{
}

inline Lumen::FRenderShaderObject::FRenderShaderObject ( TSharedRef<RHI::IRHIShader> InShader ) noexcept : Shader( InShader )
{
}

inline Lumen::FRenderSamplerObject::FRenderSamplerObject ( TSharedRef<RHI::IRHISampler> InSampler ) noexcept
    : Sampler( InSampler )
{
}

inline Lumen::FRenderBindingLayoutObject::FRenderBindingLayoutObject ( TSharedRef<RHI::IRHIBindingLayout> InLayout ) noexcept
    : Layout( InLayout )
{
}

inline Lumen::FRenderPipelineLayoutObject::FRenderPipelineLayoutObject ( TSharedRef<RHI::IRHIPipelineLayout> InLayout ) noexcept
    : Layout( InLayout )
{
}

inline Lumen::FRenderPipelineObject::FRenderPipelineObject ( TSharedRef<RHI::IRHIPipeline> InPipeline ) noexcept
    : Pipeline( InPipeline )
{
}

inline Lumen::FRenderBindingSetObject::FRenderBindingSetObject ( TSharedRef<RHI::IRHIBindingSet> InSet ) noexcept : Set( InSet )
{
}

inline Lumen::FRenderPendingState::FRenderPendingState ( TSharedPtr<FRenderTextureObject> InTexture,
                                                         RHI::ERHIResourceState InState ) noexcept
    : Texture( InTexture ), State( InState )
{
}

inline Lumen::RHI::IRHIDevice &Lumen::FRenderFeatureHost::GetDevice () const noexcept
{
    return *Device;
}

inline Lumen::RHI::IRHICommandList &Lumen::FRenderPassEncoderAdapter::GetCommandList () const noexcept
{
    return CommandList;
}

template <typename TType> TType *Lumen::AsOpaque ( const Lumen::TSharedPtr<Lumen::IRenderOpaqueObject> &InRef ) noexcept
{
    return InRef.IsValid() ? dynamic_cast<TType *>( InRef.Get() ) : nullptr;
}
