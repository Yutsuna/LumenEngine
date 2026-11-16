/**
 * @file RHICapabilityGateTests.cpp
 * @brief Tests adapter requirement negotiation through module creation.
 */

#include <Module/NullModule.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
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

} // namespace Lumen
