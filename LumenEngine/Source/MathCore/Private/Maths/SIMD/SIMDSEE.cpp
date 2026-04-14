/**
 * @file SIMDSEE.cpp
 * @brief Implementation of SSE utilities for mathematical operations.
 */

#include "Maths/SIMD/SIMD.hpp"

#if defined( LUMEN_SIMD_SSE )

    #include "Maths/SIMD/Backend/SIMDSSE.hpp"

void LumenEngine::Maths::SIMD::MatrixMul4x4 ( const Float32 *InLeft, const Float32 *InRight, Float32 *OutResult ) noexcept
{
    const __m128 L0 = _mm_loadu_ps( InLeft + 0 );
    const __m128 L1 = _mm_loadu_ps( InLeft + 4 );
    const __m128 L2 = _mm_loadu_ps( InLeft + 8 );
    const __m128 L3 = _mm_loadu_ps( InLeft + 12 );

    for ( USize Idx = 0; Idx < 4; ++Idx )
    {
        /**
         * Load the current column of the right matrix
         */
        const __m128 RCol = _mm_loadu_ps( InRight + ( Idx * 4 ) );

        const __m128 V0 = _mm_shuffle_ps( RCol, RCol, _MM_SHUFFLE( 0, 0, 0, 0 ) );
        const __m128 V1 = _mm_shuffle_ps( RCol, RCol, _MM_SHUFFLE( 1, 1, 1, 1 ) );
        const __m128 V2 = _mm_shuffle_ps( RCol, RCol, _MM_SHUFFLE( 2, 2, 2, 2 ) );
        const __m128 V3 = _mm_shuffle_ps( RCol, RCol, _MM_SHUFFLE( 3, 3, 3, 3 ) );

        /**
         * Compute the linear combination: Res = (L0 * V0) + (L1 * V1) + (L2 * V2) + (L3 * V3)
         */
        __m128 Res = _mm_mul_ps( V0, L0 );
        Res        = _mm_add_ps( Res, _mm_mul_ps( V1, L1 ) );
        Res        = _mm_add_ps( Res, _mm_mul_ps( V2, L2 ) );
        Res        = _mm_add_ps( Res, _mm_mul_ps( V3, L3 ) );

        _mm_storeu_ps( OutResult + ( Idx * 4 ), Res );
    }
}

void LumenEngine::Maths::SIMD::MatrixScalarMul4x4 ( const Float32 *InMatrix, const Float32 Scalar, Float32 *OutResult ) noexcept
{
    const __m128 ScalarVec = _mm_set1_ps( Scalar );

    for ( USize Idx = 0; Idx < 4; ++Idx )
    {
        const __m128 MatCol = _mm_loadu_ps( InMatrix + ( Idx * 4 ) );
        const __m128 Res    = _mm_mul_ps( MatCol, ScalarVec );
        _mm_storeu_ps( OutResult + ( Idx * 4 ), Res );
    }
}

#endif