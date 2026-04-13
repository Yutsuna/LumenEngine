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

        constexpr FSSEFloat32x4::FSSEFloat32x4 () noexcept : Internal( _mm_setzero_ps() )
        {

        }

        constexpr FSSEFloat32x4::FSSEFloat32x4 ( const __m128 InInternal ) noexcept : Internal( InInternal )
        {

        }

        constexpr FSSEFloat32x4 FSSEFloat32x4::Load ( const Float32 *const InAddress ) noexcept
        {
            return FSSEFloat32x4( _mm_load_ps( InAddress ) );
        }

        constexpr void FSSEFloat32x4::Store ( Float32 *const OutAddress ) const noexcept
        {
            _mm_store_ps( OutAddress, Internal );
        }

        constexpr void MatrixMul4x4 ( const Float32* InLeft, const Float32* InRight, Float32* OutResult ) noexcept
        {
            const __m128 L0 = _mm_load_ps( InLeft + 0 );
            const __m128 L1 = _mm_load_ps( InLeft + 4 );
            const __m128 L2 = _mm_load_ps( InLeft + 8 );
            const __m128 L3 = _mm_load_ps( InLeft + 12 );

            for ( USize Idx = 0; Idx < 4; ++Idx )
            {
            /**
             * Load the current column of the right matrix
             * This register contains [R[Idx][0], R[Idx][1], R[Idx][2], R[Idx][3]]
             */
            const __m128 RCol = _mm_load_ps( InRight + ( Idx * 4 ) );

            /**
             * V0 = [R[Idx][0], R[Idx][0], R[Idx][0], R[Idx][0]]
             * V1 = [R[Idx][1], R[Idx][1], R[Idx][1], R[Idx][1]]
             * V2 = [R[Idx][2], R[Idx][2], R[Idx][2], R[Idx][2]]
             * V3 = [R[Idx][3], R[Idx][3], R[Idx][3], R[Idx][3]]
             */
            const __m128 V0 = _mm_shuffle_ps( RCol, RCol, _MM_SHUFFLE( 0, 0, 0, 0 ) );
            const __m128 V1 = _mm_shuffle_ps( RCol, RCol, _MM_SHUFFLE( 1, 1, 1, 1 ) );
            const __m128 V2 = _mm_shuffle_ps( RCol, RCol, _MM_SHUFFLE( 2, 2, 2, 2 ) );
            const __m128 V3 = _mm_shuffle_ps( RCol, RCol, _MM_SHUFFLE( 3, 3, 3, 3 ) );

            /**
             * Compute the linear combination for the current column of the output matrix:
             * Res = (L0 * V0) + (L1 * V1) + (L2 * V2) + (L3 * V3)
             * This results in the column of the output matrix corresponding to the current column of the right matrix.
             */
            __m128 Res = _mm_mul_ps( V0, L0 );
            Res = _mm_add_ps( Res, _mm_mul_ps( V1, L1 ) );
            Res = _mm_add_ps( Res, _mm_mul_ps( V2, L2 ) );
            Res = _mm_add_ps( Res, _mm_mul_ps( V3, L3 ) );

            /** Store the resulting column */
            _mm_store_ps( OutResult + ( Idx * 4 ), Res );
            }
        }

    } // namespace SIMD

} // namespace Maths

} // namespace LumenEngine