/// @file BenchmarkClobber.hpp
/// @brief Optimizer barriers keeping the compiler from deleting the code under measurement.

#pragma once

#include <Core/CoreDefinitions.hpp>
#include <Core/CoreTypes.hpp>

namespace Lumen
{

namespace Benchmarks
{

#if defined( LUMEN_COMPILER_CLANG ) || defined( LUMEN_COMPILER_GCC )

    /// @brief Forces Value to be considered "used".
    template <typename TObject> LUMEN_FORCE_INLINE void DoNotOptimize ( const TObject &Value ) noexcept
    {
        __asm__ volatile( "" : : "r,m"( Value ) : "memory" ); /// NOLINT(hicpp-no-assembler)
    }

    /// @brief Forces Value to be considered "used".
    template <typename T> LUMEN_FORCE_INLINE void DoNotOptimize ( T &Value ) noexcept
    {
    #if defined( LUMEN_COMPILER_CLANG )
        __asm__ volatile( "" : "+r,m"( Value ) : : "memory" ); /// NOLINT(hicpp-no-assembler)
    #elif defined( LUMEN_COMPILER_GCC )
        __asm__ volatile( "" : "+m,r"( Value ) : : "memory" ); /// NOLINT(hicpp-no-assembler)
    #endif
    }

    /// @brief Forces all pending memory writes to be considered visible.
    LUMEN_FORCE_INLINE void ClobberMemory () noexcept
    {
        __asm__ volatile( "" : : : "memory" ); /// NOLINT(hicpp-no-assembler)
    }

#else

    namespace Internal
    {
        inline const volatile void *volatile GBenchmarkSink = nullptr;
    } // namespace Internal

    template <typename T> LUMEN_FORCE_INLINE void DoNotOptimize ( T &&Value ) noexcept
    {
        Internal::GBenchmarkSink = std::addressof( Value );
        std::atomic_signal_fence( std::memory_order_seq_cst );
    }

    LUMEN_FORCE_INLINE void ClobberMemory () noexcept
    {
        std::atomic_signal_fence( std::memory_order_seq_cst );
    }

#endif

} // namespace Benchmarks

} // namespace Lumen
