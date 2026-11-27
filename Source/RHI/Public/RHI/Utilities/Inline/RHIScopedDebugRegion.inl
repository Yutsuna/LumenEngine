/**
 * @file RHIScopedDebugRegion.inl
 * @brief Inline RAII implementation for command debug regions.
 */

#pragma once

#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Utilities/RHIScopedDebugRegion.hpp>

inline Lumen::RHI::FRHIScopedDebugRegion::FRHIScopedDebugRegion ( IRHICommandList &InCommandList,
                                                                  const FStringView InName ) noexcept
    : CommandList( &InCommandList )
{
    CommandList->BeginDebugRegion( InName );
}

inline Lumen::RHI::FRHIScopedDebugRegion::~FRHIScopedDebugRegion () noexcept
{
    CommandList->EndDebugRegion();
}
