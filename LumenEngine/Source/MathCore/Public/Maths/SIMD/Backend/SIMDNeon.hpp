/**
 * @file SIMDNeon.hpp
 * @brief Declaration of Neon utilities for mathematical operations.
 */

#pragma once

#include "Maths/SIMD/SIMDTypes.hpp"

#if defined( __aarch64__ ) || defined( _M_ARM64 )
    #include <arm_neon.h>
#endif

#if !defined( __aarch64__ ) && !defined( _M_ARM64 )
    #error "SIMDNeon.hpp should only be included on ARM64 platforms with Neon support."
#endif

namespace LumenEngine
{

namespace Maths
{

    namespace SIMD
    {

        struct alignas( 16 ) FNeonFloat32x4 final : public TSimdRegister<Float32>
        {
        public:

            FNeonFloat32x4 () noexcept;
            explicit FNeonFloat32x4 ( const float32x4_t InInternal ) noexcept;

        public:

            /**
             * @brief Loads 4 contiguous Float32 values from memory into an FNeonFloat32x4 register.
             * @param InAddress A pointer to the first Float32 value in memory to load.
             * @return An FNeonFloat32x4 register containing the loaded values.
             */
            static FNeonFloat32x4 Load ( const Float32 *const InAddress ) noexcept;

            /**
             * @brief Stores the values in an FNeonFloat32x4 register into memory.
             * @param OutAddress A pointer to the location in memory where the values should be stored.
             */
            void Store ( Float32 *const OutAddress ) const noexcept;

        private:

            float32x4_t Internal;
        };

        /**
         * @brief Multiplies two 4x4 matrices A and B, storing the result in Out.
         * @param InLeft A pointer to the first element of the first 4x4 matrix (16 Float32 values).
         * @param InRight A pointer to the first element of the second 4x4 matrix (16 Float32 values).
         * @param OutResult A pointer to the first element of the output 4x4 matrix where the result will be stored (16 Float32 values).
         */
        static void MatrixMul4x4 ( const Float32 *InLeft, const Float32 *InRight, Float32 *OutResult ) noexcept;

        /**
         * @brief Multiplies a 4x4 matrix by a scalar, storing the result in Out.
         * @param InMatrix A pointer to the first element of the 4x4 matrix (16 Float32 values).
         * @param Scalar The scalar value to multiply by.
         * @param OutResult A pointer to the first element of the output 4x4 matrix where the result will be stored (16 Float32 values).
         */
        static void MatrixScalarMul4x4 ( const Float32 *InMatrix, const Float32 Scalar, Float32 *OutResult ) noexcept;

    } // namespace SIMD

} // namespace Maths

} // namespace LumenEngine

#include "Inline/SIMDNeon.inl"