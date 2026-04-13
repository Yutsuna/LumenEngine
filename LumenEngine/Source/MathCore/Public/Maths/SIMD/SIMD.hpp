/**
 * @file SIMD.hpp
 * @brief Declaration of SIMD utilities for mathematical operations.
 */

#pragma once

#include "Maths/SIMD/SIMDTypes.hpp"

#if defined( __x86_64__ ) || defined( _M_X64 ) || defined( __i386__ )
    #include "Maths/SIMD/Backend/SIMDSSE.hpp"
    #define LUMEN_SIMD_SSE 1
#elif defined( __aarch64__ ) || defined( _M_ARM64 )
    #include "Maths/SIMD/Backend/SIMDNeon.hpp"
    #define LUMEN_SIMD_NEON 1
#else
    #define LUMEN_SIMD_GENERIC 1
#endif

namespace LumenEngine
{

namespace Maths
{

    namespace SIMD
    {

#if LUMEN_SIMD_SSE
        using FSimdFloat = FSimdRegisterF32;
#elif LUMEN_SIMD_NEON
        using FSimdFloat = FSimdRegisterF32;
#endif

    } // namespace SIMD

} // namespace Maths

} // namespace LumenEngine