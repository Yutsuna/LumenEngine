/**
 * @file RHICapabilityGateTests.cpp
 * @brief Tests adapter requirement negotiation through module creation.
 */

#include <Device/NullDeviceDesc.hpp>
#include <Module/NullModule.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    LUMEN_TEST( RHICapabilityGate, RejectsMissingAdapterFeatureAndQueues )
    {
        Tests::FNullModule Module;
        LUMEN_CHECK_TRUE( Module.CreateDevice( FRHIDeviceDesc{} ).HasValue() );
        FRHIDeviceDesc MissingFeature;
        MissingFeature.RequiredCapabilities.Features = ERHICapabilityFlags::RayTracing;
        LUMEN_CHECK_HAS_ERROR( Module.CreateDevice( MissingFeature ) );
        FRHIDeviceDesc MissingAdapter;
        MissingAdapter.AdapterId.Emplace( 99ULL );
        LUMEN_CHECK_HAS_ERROR( Module.CreateDevice( MissingAdapter ) );
        FRHIDeviceDesc TooManyQueues;
        TooManyQueues.Queues = { { ERHIQueueType::Copy, 2U } };
        LUMEN_CHECK_HAS_ERROR( Module.CreateDevice( TooManyQueues ) );
    }

    LUMEN_TEST( RHICapabilityGate, RejectsHigherRequiredShaderModelMajor )
    {
        Tests::FNullModule Module;
        FRHIDeviceDesc Request;
        Request.RequiredCapabilities.ShaderModelMajor = 1U;
        LUMEN_CHECK_HAS_ERROR( Module.CreateDevice( Request ) );
    }

    LUMEN_TEST( RHICapabilityGate, RejectsHigherRequiredShaderModelMinor )
    {
        Tests::FNullDeviceDesc Desc;
        Desc.Adapter.Capabilities.ShaderModelMajor = 6U;
        Desc.Adapter.Capabilities.ShaderModelMinor = 0U;
        Tests::FNullModule Module( Desc );
        FRHIDeviceDesc Request;
        Request.RequiredCapabilities.ShaderModelMajor = 6U;
        Request.RequiredCapabilities.ShaderModelMinor = 1U;
        LUMEN_CHECK_HAS_ERROR( Module.CreateDevice( Request ) );
    }

    LUMEN_TEST( RHICapabilityGate, AcceptsEqualOrLowerShaderModel )
    {
        Tests::FNullDeviceDesc Desc;
        Desc.Adapter.Capabilities.ShaderModelMajor = 6U;
        Desc.Adapter.Capabilities.ShaderModelMinor = 3U;
        Tests::FNullModule Module( Desc );

        FRHIDeviceDesc Equal;
        Equal.RequiredCapabilities.ShaderModelMajor = 6U;
        Equal.RequiredCapabilities.ShaderModelMinor = 3U;
        LUMEN_CHECK_TRUE( Module.CreateDevice( Equal ).HasValue() );

        FRHIDeviceDesc LowerMajor;
        LowerMajor.RequiredCapabilities.ShaderModelMajor = 5U;
        LowerMajor.RequiredCapabilities.ShaderModelMinor = 9U;
        LUMEN_CHECK_TRUE( Module.CreateDevice( LowerMajor ).HasValue() );

        FRHIDeviceDesc LowerMinor;
        LowerMinor.RequiredCapabilities.ShaderModelMajor = 6U;
        LowerMinor.RequiredCapabilities.ShaderModelMinor = 1U;
        LUMEN_CHECK_TRUE( Module.CreateDevice( LowerMinor ).HasValue() );

        LUMEN_CHECK_TRUE( Module.CreateDevice( FRHIDeviceDesc{} ).HasValue() );
    }

} // namespace RHI

} // namespace Lumen
