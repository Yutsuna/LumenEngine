/**
 * @file RHITypes.inl
 * @brief Inline implementations for RHI types
 */

#pragma once

#include "RHI/RHITypes.hpp"

template <typename Tag> constexpr LumenEngine::RHI::TRenderResourceHandle<Tag>::TRenderResourceHandle( UInt32 InID ) noexcept : ID( InID )
{
    /** Ctor */
}

template <typename Tag>
constexpr LumenEngine::RHI::TRenderResourceHandle<Tag>::TRenderResourceHandle( UInt16 InIndex, UInt16 InGeneration ) noexcept
    : ID( ( static_cast<UInt32>( InGeneration ) << 16 ) | static_cast<UInt32>( InIndex ) )
{
    /** Ctor */
}

template <typename Tag> constexpr LumenEngine::UInt16 LumenEngine::RHI::TRenderResourceHandle<Tag>::GetIndex () const noexcept
{
    return static_cast<UInt16>( ID & 0xFFFF );
}

template <typename Tag> constexpr LumenEngine::UInt16 LumenEngine::RHI::TRenderResourceHandle<Tag>::GetGeneration () const noexcept
{
    return static_cast<UInt16>( ( ID >> 16 ) & 0xFFFF );
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
