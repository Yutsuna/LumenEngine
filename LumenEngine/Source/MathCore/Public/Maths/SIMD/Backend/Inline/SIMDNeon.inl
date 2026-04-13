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

        FNeonFloat32x4::FNeonFloat32x4 () noexcept : Internal( vdupq_n_f32( 0.0F ) )
        {
            /* */
        }

        FNeonFloat32x4::FNeonFloat32x4 ( const float32x4_t InInternal ) noexcept : Internal( InInternal )
        {
            /* */
        }

        FNeonFloat32x4 FNeonFloat32x4::Load ( const Float32 *const InAddress ) noexcept
        {
            return FNeonFloat32x4( vld1q_f32( InAddress ) );
        }

        void FNeonFloat32x4::Store ( Float32 *const OutAddress ) const noexcept
        {
            vst1q_f32( OutAddress, Internal );
        }

    } // namespace SIMD

} // namespace Maths

} // namespace LumenEngine