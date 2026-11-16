/**
 * @file NullMemoryTests.cpp
 * @brief Tests Null heap mapping rules.
 */

#include <Memory/NullMemory.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( NullMemory, MapsOnlyCpuVisibleRanges )
{
    Tests::FNullHeap Heap( { .Size = 128ULL, .MemoryClass = ERHIMemoryClass::Upload }, FRHIObjectId{ 1ULL } );
    LUMEN_CHECK_TRUE( Heap.Map( 16ULL, 32ULL ).HasValue() );
    LUMEN_CHECK_HAS_ERROR( Heap.Map( 120ULL, 16ULL ) );
    Tests::FNullHeap DeviceHeap( { .Size = 128ULL, .MemoryClass = ERHIMemoryClass::DeviceLocal }, FRHIObjectId{ 2ULL } );
    LUMEN_CHECK_HAS_ERROR( DeviceHeap.Map( 0ULL, 1ULL ) );
}

} // namespace Lumen
