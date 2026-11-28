/**
 * @file AllocationTracking.cpp
 * @brief Single set of test-only global allocation overrides for RendererTests.
 */

#include <AllocationTracking.hpp>

#include <cstddef>
#include <cstdlib>
#include <new>

#if defined( _WIN32 )
    #include <malloc.h>
#endif

namespace
{
/** @brief Releases a block with the heap API matching Lumen::TrackedAllocate. */
void FreeTrackedAllocation ( void *const InPointer, const std::size_t InAlignment = alignof( std::max_align_t ) ) noexcept
{
#if defined( _WIN32 )
    // UCRT provides _aligned_malloc instead of std::aligned_alloc. Over-aligned blocks
    // must be released with _aligned_free; default-aligned blocks come from malloc.
    if ( InAlignment > alignof( std::max_align_t ) )
    {
        _aligned_free( InPointer );
        return;
    }
#endif
    std::free( InPointer );
}
} // namespace

Lumen::TAtomic<Lumen::UInt64> Lumen::GTrackedAllocations{ 0ULL };
Lumen::TAtomic<Lumen::Bool> Lumen::bTrackingAllocations{ false };

[[nodiscard]] void *Lumen::TrackedAllocate ( const std::size_t InSize, const std::size_t InAlignment ) noexcept
{
    if ( bTrackingAllocations.load( std::memory_order_relaxed ) )
    {
        GTrackedAllocations.fetch_add( 1ULL, std::memory_order_relaxed );
    }

    if ( InAlignment <= alignof( std::max_align_t ) )
    {
        return std::malloc( InSize );
    }

#if defined( _WIN32 )
    return _aligned_malloc( InSize, InAlignment );
#else
    const std::size_t AlignedSize = ( ( InSize + InAlignment - 1U ) / InAlignment ) * InAlignment;
    return std::aligned_alloc( InAlignment, AlignedSize );
#endif
}

/** INFO: Test-only global allocation hooks; engine code never replaces the global allocator. */
void *operator new( const std::size_t InSize )
{
    void *const Result = Lumen::TrackedAllocate( InSize, alignof( std::max_align_t ) );
    if ( Result == nullptr ) [[unlikely]]
    {
        std::abort();
    }
    return Result;
}

void *operator new[]( const std::size_t InSize )
{
    return ::operator new( InSize );
}

void *operator new( const std::size_t InSize, const std::align_val_t InAlignment )
{
    void *const Result = Lumen::TrackedAllocate( InSize, static_cast<std::size_t>( InAlignment ) );
    if ( Result == nullptr ) [[unlikely]]
    {
        std::abort();
    }
    return Result;
}

void *operator new[]( const std::size_t InSize, const std::align_val_t InAlignment )
{
    return ::operator new( InSize, InAlignment );
}

void *operator new( const std::size_t InSize, const std::nothrow_t & ) noexcept
{
    return Lumen::TrackedAllocate( InSize, alignof( std::max_align_t ) );
}

void *operator new[]( const std::size_t InSize, const std::nothrow_t & ) noexcept
{
    return Lumen::TrackedAllocate( InSize, alignof( std::max_align_t ) );
}

void *operator new( const std::size_t InSize, const std::align_val_t InAlignment, const std::nothrow_t & ) noexcept
{
    return Lumen::TrackedAllocate( InSize, static_cast<std::size_t>( InAlignment ) );
}

void *operator new[]( const std::size_t InSize, const std::align_val_t InAlignment, const std::nothrow_t & ) noexcept
{
    return Lumen::TrackedAllocate( InSize, static_cast<std::size_t>( InAlignment ) );
}

void operator delete( void *const InPointer ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete[]( void *const InPointer ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete( void *const InPointer, const std::size_t ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete[]( void *const InPointer, const std::size_t ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete( void *const InPointer, const std::align_val_t InAlignment ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<std::size_t>( InAlignment ) );
}

void operator delete[]( void *const InPointer, const std::align_val_t InAlignment ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<std::size_t>( InAlignment ) );
}

void operator delete( void *const InPointer, const std::size_t, const std::align_val_t InAlignment ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<std::size_t>( InAlignment ) );
}

void operator delete[]( void *const InPointer, const std::size_t, const std::align_val_t InAlignment ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<std::size_t>( InAlignment ) );
}

void operator delete( void *const InPointer, const std::nothrow_t & ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete[]( void *const InPointer, const std::nothrow_t & ) noexcept
{
    FreeTrackedAllocation( InPointer );
}

void operator delete( void *const InPointer, const std::align_val_t InAlignment, const std::nothrow_t & ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<std::size_t>( InAlignment ) );
}

void operator delete[]( void *const InPointer, const std::align_val_t InAlignment, const std::nothrow_t & ) noexcept
{
    FreeTrackedAllocation( InPointer, static_cast<std::size_t>( InAlignment ) );
}
