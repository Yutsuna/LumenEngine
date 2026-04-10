/**
 * @file RHITypes.inl
 * @brief Inline implementations for RHI types
 */

#pragma once

#include "RHI/RHITypes.hpp"

template <typename Tag> constexpr LumenEngine::RHI::TRenderResourceHandle<Tag>::TRenderResourceHandle ( UInt32 InID ) noexcept : ID( InID )
{
    //
}

template <typename Tag> constexpr LumenEngine::Bool LumenEngine::RHI::TRenderResourceHandle<Tag>::IsValid () const noexcept
{
    return ID != InvalidID;
}

template <typename Tag> constexpr LumenEngine::Bool LumenEngine::RHI::TRenderResourceHandle<Tag>::operator==( const TRenderResourceHandle &Other ) const noexcept
{
    return ID == Other.ID;
}

template <typename Tag> constexpr LumenEngine::Bool LumenEngine::RHI::TRenderResourceHandle<Tag>::operator!=( const TRenderResourceHandle &Other ) const noexcept
{
    return ID != Other.ID;
}
