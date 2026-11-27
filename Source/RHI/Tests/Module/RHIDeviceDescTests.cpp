/**
 * @file RHIDeviceDescTests.cpp
 * @brief Tests device descriptor queue requests and debug configuration.
 */

#include <RHI/Debug/IRHIDebugSink.hpp>
#include <RHI/Module/RHIDeviceDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{

    namespace
    {
        class FDeviceDescDebugSink final : public IRHIDebugSink
        {
        public:

            void Emit ( const FRHIDebugMessage & ) noexcept override
            {
            }
        };
    } // namespace

    LUMEN_TEST( RHIDeviceDesc, RejectsEmptyDuplicateAndZeroQueues )
    {
        LUMEN_CHECK_TRUE( FRHIDeviceDesc{}.Validate().HasValue() );
        FRHIDeviceDesc Desc;
        Desc.Queues.clear();
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Queues = { { ERHIQueueType::Graphics, 1U }, { ERHIQueueType::Graphics, 1U } };
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
        Desc.Queues = { { ERHIQueueType::Compute, 0U } };
        LUMEN_CHECK_HAS_ERROR( Desc.Validate() );
    }

    LUMEN_TEST( RHIDeviceDesc, AcceptsIndependentNonOwningDebugSink )
    {
        FDeviceDescDebugSink Sink;
        FRHIDeviceDesc Desc;
        Desc.DebugSink = &Sink;
        LUMEN_CHECK_TRUE( Desc.DebugSink == &Sink );
        LUMEN_CHECK_FALSE( Desc.bEnableDebug );
        LUMEN_CHECK_TRUE( Desc.Validate().HasValue() );
    }

} // namespace RHI

} // namespace Lumen
