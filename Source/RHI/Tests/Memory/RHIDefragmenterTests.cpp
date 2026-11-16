/**
 * @file RHIDefragmenterTests.cpp
 * @brief Tests conservative defragmentation.
 */

#include <Device/NullDevice.hpp>
#include <Memory/IRHIDefragmenter.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIDefragmenter, EmptyPlanIsValidAndExecutable )
{
    Tests::FNullDevice Device;
    auto Defragmenter = *Device.CreateDefragmenter();
    auto Plan         = Defragmenter->BuildPlan( {} );
    LUMEN_CHECK_TRUE( Plan.HasValue() );
    LUMEN_CHECK_TRUE( Defragmenter->Execute( *Plan ).HasValue() );
}

} // namespace Lumen
