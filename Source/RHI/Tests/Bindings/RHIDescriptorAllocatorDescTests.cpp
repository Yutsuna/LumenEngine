/**
 * @file RHIDescriptorAllocatorDescTests.cpp
 * @brief Tests descriptor allocator capacity validation.
 */

#include <Bindings/RHIDescriptorAllocatorDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{
    LUMEN_TEST( RHIDescriptorAllocatorDesc, RequiresNonZeroOrderedCapacities )
    {
        FRHIDescriptorAllocatorDesc Desc;
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
        Desc.InitialCapacity = 0U;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.InitialCapacity = 32U;
        Desc.MaximumCapacity = 16U;
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.MaximumCapacity = Desc.InitialCapacity;
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
    }

} // namespace

} // namespace Lumen
