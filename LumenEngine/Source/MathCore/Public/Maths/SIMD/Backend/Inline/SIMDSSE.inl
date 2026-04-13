/**
 * @file SIMDSSE.inl
 * @brief Implementation of SSE utilities for mathematical operations.
 */

#pragma once

#include "Maths/SIMD/Backend/SIMDSSE.hpp"

namespace LumenEngine
{

namespace Maths
{

    namespace SIMD
    {

        inline FSSEFloat32x4::FSSEFloat32x4 () noexcept : Internal( _mm_setzero_ps() )
        {
            /* */
        }

        inline FSSEFloat32x4::FSSEFloat32x4 ( const __m128 InInternal ) noexcept : Internal( InInternal )
        {
            /* */
        }

        inline FSSEFloat32x4 FSSEFloat32x4::Load ( const Float32 *const InAddress ) noexcept
        {
            [[assume( ( reinterpret_cast<UPtr>( InAddress ) & 15 ) == 0 )]];
            return FSSEFloat32x4( _mm_load_ps( InAddress ) );
        }

        inline void FSSEFloat32x4::Store ( Float32 *const OutAddress ) const noexcept
        {
            [[assume( ( reinterpret_cast<UPtr>( OutAddress ) & 15 ) == 0 )]];
            _mm_store_ps( OutAddress, Internal );
        }

    } // namespace SIMD

} // namespace Maths

} // namespace LumenEngine