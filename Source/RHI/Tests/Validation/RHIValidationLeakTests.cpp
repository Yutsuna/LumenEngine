/**
 * @file RHIValidationLeakTests.cpp
 * @brief Tests live-object accounting and shutdown leak diagnostics.
 */
#include "Validation/RHIValidationReporter.hpp"
#include "Validation/RHIValidationTestUtils.hpp"
#include <Resources/IRHIBuffer.hpp>
#include <Resources/IRHITexture.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIValidationLeak, ReportsChildrenThatOutliveValidatedDevice )
    {
        Tests::FRecordingDebugSink Sink;
        TSharedPtr<IRHIDevice> Device = Tests::Validation::MakeDevice( Sink );
        FRHIBufferDesc BufferDesc;
        BufferDesc.Size               = 64ULL;
        BufferDesc.Usage              = ERHIResourceUsage::ShaderResource;
        TSharedPtr<IRHIBuffer> Buffer = *Device->CreateBuffer( BufferDesc );
        Device.Reset();
        LUMEN_CHECK_TRUE( Sink.ContainsCode( static_cast<UInt32>( ERHIValidationMessageCode::ObjectLeak ) ) );
        LUMEN_CHECK_EQ( Sink.GetCount(), 1U );
        Buffer.Reset();
        LUMEN_CHECK_EQ( Sink.GetCount(), 1U );
    }

    LUMEN_TEST( RHIValidationLeak, DoesNotReportNormallyDestroyedChildren )
    {
        Tests::FRecordingDebugSink Sink;
        {
            TSharedRef<IRHIDevice> Device = Tests::Validation::MakeDevice( Sink );
            {
                TSharedRef<IRHIBuffer> Buffer   = Tests::Validation::MakeBuffer( Device );
                TSharedRef<IRHITexture> Texture = Tests::Validation::MakeTexture( Device );
                LUMEN_CHECK_TRUE( Buffer->GetId().IsValid() );
                LUMEN_CHECK_TRUE( Texture->GetId().IsValid() );
            }
            TExpected<TSharedRef<IRHIQueue>, FRHIError> Queue = Device->GetQueue( ERHIQueueType::Graphics, 0U );
            LUMEN_CHECK_TRUE( Queue.HasValue() );
        }
        LUMEN_CHECK_EQ( Sink.GetCount(), 0U );
    }
} // namespace
} // namespace Lumen
