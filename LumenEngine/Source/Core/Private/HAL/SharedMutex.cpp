/**
 * @file SharedMutex.cpp
 * @brief Implementation of FSharedMutex
 */

#include "HAL/SharedMutex.hpp"

/** Maximum number of busy-spin iterations before falling back to atomic::wait. */
static constexpr LumenEngine::UInt32 GMaxSpinCount = 64U;

void LumenEngine::FSharedMutex::Lock () noexcept
{
    /** INFO: Only one thread can win the CAS; every other writer spins via WaitForWriterFlag */
    UInt32 Expected = 0U;
    while ( not WriterFlag.compare_exchange_weak( Expected, 1U, std::memory_order::acquire, std::memory_order::relaxed ) )
    {
        Expected = 0U;
        WaitForWriterFlag( 0U );
    }

    /** INFO: Waiting for all active readers to finish */
    WaitForReadersTodrain();
}

LumenEngine::Bool LumenEngine::FSharedMutex::TryLock () noexcept
{
    UInt32 Expected = 0U;

    /** INFO: Succeed only when there is no active writer AND no active readers */
    if ( not WriterFlag.compare_exchange_strong( Expected, 1U, std::memory_order::acquire, std::memory_order::relaxed ) )
    {
        return false;
    }

    /** INFO: Check if there are any active readers */
    if ( ReaderCount.load( std::memory_order::acquire ) != 0U )
    {
        WriterFlag.store( 0U, std::memory_order::release );
        WriterFlag.notify_all();
        return false;
    }

    return true;
}

void LumenEngine::FSharedMutex::Unlock () noexcept
{
    /** INFO: Release the exclusive lock */
    WriterFlag.store( 0U, std::memory_order::release );
    WriterFlag.notify_all();
}

void LumenEngine::FSharedMutex::LockShared () noexcept
{
    for ( ;; )
    {
        WaitForWriterFlag( 0U );

        ReaderCount.fetch_add( 1U, std::memory_order::acquire );

        if ( WriterFlag.load( std::memory_order::relaxed ) == 0U )
        {
            return;
        }

        ReaderCount.fetch_sub( 1U, std::memory_order::release );
        ReaderCount.notify_all();
    }
}

LumenEngine::Bool LumenEngine::FSharedMutex::TryLockShared () noexcept
{
    if ( WriterFlag.load( std::memory_order::relaxed ) != 0U )
    {
        return false;
    }

    ReaderCount.fetch_add( 1U, std::memory_order::acquire );

    if ( WriterFlag.load( std::memory_order::relaxed ) != 0U )
    {
        ReaderCount.fetch_sub( 1U, std::memory_order::release );
        ReaderCount.notify_all();
        return false;
    }

    return true;
}

void LumenEngine::FSharedMutex::UnlockShared () noexcept
{
    ReaderCount.fetch_sub( 1U, std::memory_order::release );
    ReaderCount.notify_all();
}

LumenEngine::Bool LumenEngine::FSharedMutex::IsWriteLocked () const noexcept
{
    return WriterFlag.load( std::memory_order::relaxed ) != 0U;
}

LumenEngine::UInt32 LumenEngine::FSharedMutex::SharedLockCount () const noexcept
{
    return ReaderCount.load( std::memory_order::relaxed );
}

void LumenEngine::FSharedMutex::WaitForWriterFlag ( UInt32 InExpected ) noexcept
{
    UInt32 SpinCount = 0U;

    while ( WriterFlag.load( std::memory_order::relaxed ) != InExpected )
    {
        if ( SpinCount < GMaxSpinCount )
        {
            /**
             * INFO: Busy-spinning with exponential back-off:
             * issue a pause hint to reduce pipeline pressure and cache-line bouncing
             * on x86/ARM.  The spin count is capped to prevent excessive CPU usage
             * under high contention, at which point we transition to an OS-assisted wait.
             */
            for ( UInt32 Pause = 0U; Pause < ( 1U << SpinCount ); ++Pause )
            {
#if defined( __x86_64__ ) or defined( _M_X64 )
                __builtin_ia32_pause();
#elif defined( __aarch64__ ) or defined( _M_ARM64 )
                __asm__ __volatile__( "yield" ::: "memory" );
#endif
            }

            ++SpinCount;
        }
        else
        {
            WriterFlag.wait( WriterFlag.load( std::memory_order::relaxed ), std::memory_order::relaxed );
            SpinCount = 0U;
        }
    }
}

void LumenEngine::FSharedMutex::WaitForReadersTodrain () noexcept
{
    UInt32 SpinCount = 0U;

    while ( ReaderCount.load( std::memory_order::acquire ) != 0U )
    {
        if ( SpinCount < GMaxSpinCount )
        {
            for ( UInt32 Pause = 0U; Pause < ( 1U << SpinCount ); ++Pause )
            {
#if defined( __x86_64__ ) or defined( _M_X64 )
                __builtin_ia32_pause();
#elif defined( __aarch64__ ) or defined( _M_ARM64 )
                __asm__ __volatile__( "yield" ::: "memory" );
#endif
            }

            ++SpinCount;
        }
        else
        {
            ReaderCount.wait( ReaderCount.load( std::memory_order::relaxed ), std::memory_order::relaxed );
            SpinCount = 0U;
        }
    }
}
