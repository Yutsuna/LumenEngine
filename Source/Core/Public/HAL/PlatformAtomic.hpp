/**
 * @file PlatformAtomic.hpp
 * @brief Platform atomic operations
 */

#include <Core/CoreDefinitions.hpp>
#include <CoreExport.hpp>

namespace Lumen
{

namespace HAL
{

    /**
     * @brief Yields the CPU to allow other threads to execute
     */
    inline void CORE_EXPORT CpuRelax () noexcept
    {
#if defined( LUMEN_ARCH_X64 )
        __builtin_ia32_pause();
#elif defined( LUMEN_ARCH_ARM64 )
        __asm__ __volatile__( "yield" ::: "memory" );
#else
        /** INFO: no arch-specific relax hint; the compiler fence is enough */
        static_cast<void>( 0 );
#endif
    }

} // namespace HAL

} // namespace Lumen
