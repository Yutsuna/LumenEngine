/**
 * @file RHIMemoryConformanceTests.cpp
 * @brief Runs portable heap and staging contracts.
 */

#include <Device/IRHIDevice.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>

#include <Memory/IRHIUploadAllocator.hpp>
#include <Memory/RHIHeapDesc.hpp>

#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIMemoryConformance, CreatesHeapsAndStagingAllocators )
{
    auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
    LUMEN_CHECK_TRUE( ( *Device )->CreateHeap( { .Size = 4096ULL, .MemoryClass = ERHIMemoryClass::Upload } ).HasValue() );

    auto Upload = ( *Device )->CreateUploadAllocator( 4096ULL );
    LUMEN_CHECK_TRUE( Upload.HasValue() );

    auto Allocation = ( *Upload )->Allocate( { .Size = 64ULL, .Alignment = 16ULL } );
    LUMEN_CHECK_TRUE( Allocation.HasValue() );
    LUMEN_CHECK_EQ( Allocation->MappedData.size(), 64U );
}

} // namespace Lumen
