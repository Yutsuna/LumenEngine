/**
 * @file SIMDSSE.hpp
 * @brief Declaration of SSE utilities for mathematical operations.
 */

#pragma once

#if defined( __x86_64__ ) || defined( _M_X64 ) || defined( __i386__ )
    #include "Maths/SIMD/SIMDTypes.hpp"

    #include <immintrin.h>

namespace LumenEngine
{

namespace Maths
{

    namespace SIMD
    {

        struct alignas( 16 ) FSSEFloat32x4 final : public TSimdRegister<Float32>
        {
        public:

            FSSEFloat32x4 () noexcept;
            explicit FSSEFloat32x4 ( const __m128 InInternal ) noexcept;

        public:

            /**
             * @brief Loads 4 contiguous Float32 values from memory into an FSSEFloat32x4 register.
             * @param InAddress A pointer to the first Float32 value in memory to load. Must be 16-byte aligned.
             * @return An FSSEFloat32x4 register containing the loaded values.
             */
            static FSSEFloat32x4 Load ( const Float32 *const InAddress ) noexcept;

            /**
             * @brief Stores the values in an FSSEFloat32x4 register into memory.
             * @param OutAddress A pointer to the location in memory where the values should be stored. Must be 16-byte aligned.
             */
            void Store ( Float32 *const OutAddress ) const noexcept;

        private:

            __m128 Internal;
        };

        /**
         * @brief Multiplies two 4x4 column-major matrices A and B, storing the result in Out.
         * @param InLeft A pointer to the first element of the first 4x4 matrix (16 Float32 values, 16-byte aligned).
         * @param InRight A pointer to the first element of the second 4x4 matrix (16 Float32 values, 16-byte aligned).
         * @param OutResult A pointer to the output 4x4 matrix (16 Float32 values, 16-byte aligned).
         */
        inline void MatrixMul4x4 ( const Float32 *InLeft, const Float32 *InRight, Float32 *OutResult ) noexcept;

        /**
         * @brief Multiplies a 4x4 column-major matrix by a scalar, storing the result in Out.
         * @param InMatrix A pointer to the first element of the 4x4 matrix (16 Float32 values, 16-byte aligned).
         * @param Scalar The scalar value to multiply by.
         * @param OutResult A pointer to the output 4x4 matrix (16 Float32 values, 16-byte aligned).
         */
        inline void MatrixScalarMul4x4 ( const Float32 *InMatrix, const Float32 Scalar, Float32 *OutResult ) noexcept;

    } // namespace SIMD

} // namespace Maths

} // namespace LumenEngine

#include "Inline/SIMDSSE.inl"

#else
    #error "SIMDSSE.hpp should only be included on x86 platforms with SSE support."
#endif