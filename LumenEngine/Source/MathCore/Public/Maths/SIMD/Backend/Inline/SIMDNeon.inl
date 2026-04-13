/**
 * @file SIMDNeon.inl
 * @brief Implementation of Neon utilities for mathematical operations.
 */

#pragma once

#include "Maths/SIMD/Backend/SIMDNeon.hpp"

namespace LumenEngine
{

namespace Maths
{

	namespace SIMD
	{

		constexpr FNeonFloat32x4::FNeonFloat32x4 () noexcept : Internal( vdupq_n_f32( 0.0f ) )
		{

		}

		constexpr FNeonFloat32x4::FNeonFloat32x4 ( const float32x4_t InInternal ) noexcept : Internal( InInternal )
		{

		}

		constexpr FNeonFloat32x4 FNeonFloat32x4::Load ( const Float32 *const InAddress ) noexcept
		{
			return FNeonFloat32x4( vld1q_f32( InAddress ) );
		}

		constexpr void FNeonFloat32x4::Store ( Float32 *const OutAddress ) const noexcept
		{
			vst1q_f32( OutAddress, Internal );
		}

		constexpr void MatrixMul4x4 ( const Float32 *InLeft, const Float32 *InRight, Float32 *OutResult ) noexcept
		{
			const float32x4_t L0 = vld1q_f32( InLeft + 0 );
			const float32x4_t L1 = vld1q_f32( InLeft + 4 );
			const float32x4_t L2 = vld1q_f32( InLeft + 8 );
			const float32x4_t L3 = vld1q_f32( InLeft + 12 );

			for ( USize Idx = 0; Idx < 4; ++Idx )
			{
				/**
				 * Load the current column of the right matrix.
				 */
				const float32x4_t RCol = vld1q_f32( InRight + ( Idx * 4 ) );

				/**
				 * Compute the linear combination for the current column of the output matrix.
				 */
				float32x4_t Res = vmulq_n_f32( L0, vgetq_lane_f32( RCol, 0 ) );
				Res = vmlaq_n_f32( Res, L1, vgetq_lane_f32( RCol, 1 ) );
				Res = vmlaq_n_f32( Res, L2, vgetq_lane_f32( RCol, 2 ) );
				Res = vmlaq_n_f32( Res, L3, vgetq_lane_f32( RCol, 3 ) );

				vst1q_f32( OutResult + ( Idx * 4 ), Res );
			}
		}

		constexpr void MatrixScalarMul4x4 ( const Float32 *InMatrix, const Float32 Scalar, Float32 *OutResult ) noexcept
		{
			const float32x4_t VecScalar = vdupq_n_f32( Scalar );

			for ( USize Idx = 0; Idx < 4; ++Idx )
			{
				const float32x4_t MatCol = vld1q_f32( InMatrix + ( Idx * 4 ) );
				const float32x4_t Res    = vmulq_f32( MatCol, VecScalar );
				vst1q_f32( OutResult + ( Idx * 4 ), Res );
			}
		}

	} // namespace SIMD

} // namespace Maths

} // namespace LumenEngine
