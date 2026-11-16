/**
 * @file RHIModuleConformanceTests.cpp
 * @brief Runs portable module discovery on the linked backend.
 */

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

LUMEN_TEST( RHIModuleConformance, EnumeratesAtLeastOneUsableAdapter )
{
    const TSharedRef<IRHIModule> Module = Tests::GetRHIConformanceDeviceFactory().GetModule();
    LUMEN_CHECK_EQ( Module->GetApiVersion().Major, 1U );

    const auto Adapters = Module->EnumerateAdapters();
    LUMEN_CHECK_TRUE( Adapters.HasValue() );
    LUMEN_CHECK_FALSE( Adapters->empty() );
    LUMEN_CHECK_TRUE( ( *Adapters )[0].Validate().HasValue() );
}

} // namespace Lumen
