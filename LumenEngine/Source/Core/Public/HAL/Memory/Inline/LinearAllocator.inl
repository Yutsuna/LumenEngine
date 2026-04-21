/**
 * @file LinearAllocator.inl
 * @brief Inline implementation of the linear allocator.
 */

#pragma once

#include <new>
#include <utility>

template <typename T, typename... TArgs> T *LumenEngine::HAL::FLinearAllocator::New ( TArgs &&...InArgs ) noexcept
{
    void *Memory = Allocate( sizeof( T ), alignof( T ) );
    if ( Memory == nullptr )
    {
        return nullptr;
    }

    return new ( Memory ) T( std::forward<TArgs>( InArgs )... );
}

inline LumenEngine::USize LumenEngine::HAL::FLinearAllocator::GetUsedMemory () const noexcept
{
    return Offset;
}

inline LumenEngine::USize LumenEngine::HAL::FLinearAllocator::GetHighWatermark () const noexcept
{
    return HighWatermark;
}

inline LumenEngine::USize LumenEngine::HAL::FLinearAllocator::GetTotalMemory () const noexcept
{
    return TotalSize;
}

inline void *LumenEngine::HAL::FLinearAllocator::GetBaseAddress () const noexcept
{
    return Buffer;
}

inline void LumenEngine::HAL::FLinearAllocator::ResetTo ( USize InOffset ) noexcept
{
    Offset = InOffset;
}

inline LumenEngine::HAL::FScopeLinear::FScopeLinear ( FLinearAllocator &InAllocator ) noexcept : Allocator( InAllocator ), PreviousOffset( InAllocator.GetUsedMemory() )
{
}

inline LumenEngine::HAL::FScopeLinear::~FScopeLinear () noexcept
{
    Allocator.ResetTo( PreviousOffset );
}
