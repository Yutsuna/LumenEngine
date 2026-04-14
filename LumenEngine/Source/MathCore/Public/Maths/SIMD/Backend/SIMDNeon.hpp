/**
 * @file SIMDNeon.hpp
 * @brief Declaration of Neon utilities for mathematical operations.
 */

#pragma once

#if defined( __aarch64__ ) || defined( _M_ARM64 )
    #include "Maths/SIMD/SIMDTypes.hpp"

    #include <arm_neon.h>

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
             * @brief Loads 4 contiguous Float32 values from memory.
             * @param InAddress Must be 16-byte aligned for optimal performance.
             * @return An FNeonFloat32x4 register containing the loaded values.
             */
            static FNeonFloat32x4 Load ( const Float32 *const InAddress ) noexcept;

            /**
             * @brief Stores the values in an FNeonFloat32x4 register into memory.
             * @param OutAddress Must be 16-byte aligned.
             */
            void Store ( Float32 *const OutAddress ) const noexcept;

        private:

            float32x4_t Internal;
        };

        /**
         * @brief Multiplies two 4x4 column-major matrices A and B, storing the result in Out.
         */
        void MatrixMul4x4 ( const Float32 *InLeft, const Float32 *InRight, Float32 *OutResult ) noexcept;

        /**
         * @brief Multiplies a 4x4 column-major matrix by a scalar.
         */
        void MatrixScalarMul4x4 ( const Float32 *InMatrix, const Float32 Scalar, Float32 *OutResult ) noexcept;

    } // namespace SIMD

} // namespace Maths

} // namespace LumenEngine

    #include "Inline/SIMDNeon.inl"

#else
    #error "SIMDNeon.hpp should only be included on ARM64 platforms with Neon support."
#endif