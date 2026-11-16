/**
 * @file RHIScopedDebugRegion.inl
 * @brief Inline RAII implementation for command debug regions.
 */
#pragma once

#include <Commands/IRHICommandList.hpp>
#include <Utilities/RHIScopedDebugRegion.hpp>

inline Lumen::FRHIScopedDebugRegion::FRHIScopedDebugRegion ( IRHICommandList &InCommandList, const FStringView InName ) noexcept
    : CommandList( &InCommandList )
{
    CommandList->BeginDebugRegion( InName );
}

inline Lumen::FRHIScopedDebugRegion::~FRHIScopedDebugRegion () noexcept
{
    CommandList->EndDebugRegion();
}
