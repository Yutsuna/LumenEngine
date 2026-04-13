/**
 * @file SIMDNeon.cpp
 * @brief Implementation of Neon utilities for mathematical operations.
 */

#include "Maths/SIMD/SIMD.hpp"

#if defined( LUMENENGINE_SIMD_NEON )

    #include "Maths/SIMD/Backend/SIMDNeon.hpp"

void LumenEngine::Maths::SIMD::MatrixMul4x4 ( const Float32 *InLeft, const Float32 *InRight, Float32 *OutResult ) noexcept
{
    [[assume( ( reinterpret_cast<UPtr>( InLeft ) & 15 ) == 0 )]];
    [[assume( ( reinterpret_cast<UPtr>( InRight ) & 15 ) == 0 )]];
    [[assume( ( reinterpret_cast<UPtr>( OutResult ) & 15 ) == 0 )]];

    const float32x4_t L0 = vld1q_f32( InLeft + 0 );
    const float32x4_t L1 = vld1q_f32( InLeft + 4 );
    const float32x4_t L2 = vld1q_f32( InLeft + 8 );
    const float32x4_t L3 = vld1q_f32( InLeft + 12 );

    for ( USize Idx = 0; Idx < 4; ++Idx )
    {
        const float32x4_t RCol = vld1q_f32( InRight + ( Idx * 4 ) );

        /** Compute linear combination using Neon intrinsics */
        float32x4_t Res = vmulq_n_f32( L0, vgetq_lane_f32( RCol, 0 ) );
        Res             = vmlaq_n_f32( Res, L1, vgetq_lane_f32( RCol, 1 ) );
        Res             = vmlaq_n_f32( Res, L2, vgetq_lane_f32( RCol, 2 ) );
        Res             = vmlaq_n_f32( Res, L3, vgetq_lane_f32( RCol, 3 ) );

        vst1q_f32( OutResult + ( Idx * 4 ), Res );
    }
}

void LumenEngine::Maths::SIMD::MatrixScalarMul4x4 ( const Float32 *InMatrix, const Float32 Scalar, Float32 *OutResult ) noexcept
{
    const float32x4_t VecScalar = vdupq_n_f32( Scalar );

    for ( USize Idx = 0; Idx < 4; ++Idx )
    {
        const float32x4_t MatCol = vld1q_f32( InMatrix + ( Idx * 4 ) );
        const float32x4_t Res    = vmulq_f32( MatCol, VecScalar );
        vst1q_f32( OutResult + ( Idx * 4 ), Res );
    }
}

#endif