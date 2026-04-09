/**
 * @file RenderTypes.inl
 * @brief Inline implementations for rendering types.
 */

#pragma once

#include "Graphics/RenderTypes.hpp"

template <typename Tag> constexpr LumenEngine::Renderer::TRenderResourceHandle<Tag>::TRenderResourceHandle ( UInt32 InID ) noexcept : ID( InID )
{
    //
}

template <typename Tag> constexpr LumenEngine::Bool LumenEngine::Renderer::TRenderResourceHandle<Tag>::IsValid () const noexcept
{
    return ID != InvalidID;
}

template <typename Tag> constexpr LumenEngine::Bool LumenEngine::Renderer::TRenderResourceHandle<Tag>::operator==( const TRenderResourceHandle &Other ) const noexcept
{
    return ID == Other.ID;
}

template <typename Tag> constexpr LumenEngine::Bool LumenEngine::Renderer::TRenderResourceHandle<Tag>::operator!=( const TRenderResourceHandle &Other ) const noexcept
{
    return ID != Other.ID;
}
