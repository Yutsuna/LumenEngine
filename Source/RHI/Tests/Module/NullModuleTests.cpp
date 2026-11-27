/**
 * @file NullModuleTests.cpp
 * @brief Tests Null adapter discovery and device creation.
 */

#include <Module/NullModule.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( NullModule, DiscoversAdapterAndCreatesDevice )
    {
        Tests::FNullModule Module;
        LUMEN_CHECK_EQ( Module.GetApiVersion().Major, 1U );
        LUMEN_CHECK_EQ( Module.GetApiVersion().Minor, 1U );
        LUMEN_CHECK_EQ( Module.GetApiVersion().Patch, 0U );
        const TExpected<TVector<FRHIAdapterInfo>, FRHIError> Adapters = Module.EnumerateAdapters();
        LUMEN_CHECK_TRUE( Adapters.HasValue() );
        LUMEN_CHECK_EQ( Adapters->size(), 1U );
        LUMEN_CHECK_TRUE( Module.CreateDevice( FRHIDeviceDesc{} ).HasValue() );
        FRHIDeviceDesc Missing;
        Missing.AdapterId.Emplace( 2ULL );
        LUMEN_CHECK_HAS_ERROR( Module.CreateDevice( Missing ) );
    }

} // namespace RHI

} // namespace Lumen
