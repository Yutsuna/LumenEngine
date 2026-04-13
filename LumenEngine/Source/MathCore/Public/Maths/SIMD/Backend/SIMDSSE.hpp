/**
 * @file SIMD_SSE.hpp
 * @brief Declaration of SSE utilities for mathematical operations.
 */

#pragma once

#include "Maths/SIMD/SIMDTypes.hpp"

#if defined( __x86_64__ ) || defined( _M_X64 ) || defined( __i386__ )
    #include <immintrin.h>
#else
    #error "SIMDSSE.hpp should only be included on x86 platforms with SSE support."
#endif

namespace LumenEngine
{

namespace Maths
{

    namespace SIMD
    {

        struct alignas( 16 ) FSSEFloat32x4 final : public TSimdRegister<Float32>
        {
        public:

            constexpr FSSEFloat32x4 () noexcept;
            explicit constexpr FSSEFloat32x4 ( const __m128 InInternal ) noexcept;

        public:

            /**
             * @brief Loads 4 contiguous Float32 values from memory into an FSSEFloat32x4 register.
             * @param InAddress A pointer to the first Float32 value in memory to load.
             * @return An FSSEFloat32x4 register containing the loaded values.
             */
            static constexpr FSSEFloat32x4 Load ( const Float32 *const InAddress ) noexcept;

            /**
             * @brief Stores the values in an FSSEFloat32x4 register into memory.
             * @param OutAddress A pointer to the location in memory where the values should be stored.
             */
            constexpr void Store ( Float32 *const OutAddress ) const noexcept;

        private:

            __m128 Internal;
        };

        /**
         * @brief Multiplies two 4x4 matrices A and B, storing the result in Out.
         * @param InLeft A pointer to the first element of the first 4x4 matrix (16 Float32 values).
         * @param InRight A pointer to the first element of the second 4x4 matrix (16 Float32 values).
         * @param OutResult A pointer to the first element of the output 4x4 matrix where the result will be stored (16 Float32 values).
         */
        static constexpr void MatrixMul4x4 ( const Float32 *InLeft, const Float32 *InRight, Float32 *OutResult ) noexcept;

        /**
         * @brief Multiplies a 4x4 matrix by a scalar, storing the result in Out.
         * @param InMatrix A pointer to the first element of the 4x4 matrix (16 Float32 values).
         * @param Scalar The scalar value to multiply by.
         * @param OutResult A pointer to the first element of the output 4x4 matrix where the result will be stored (16 Float32 values).
         */
        static constexpr void MatrixScalarMul4x4 ( const Float32 *InMatrix, const Float32 Scalar, Float32 *OutResult ) noexcept;

    } // namespace SIMD

} // namespace Maths

} // namespace LumenEngine

#include "Inline/SIMDSSE.inl"