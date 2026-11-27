/**
 * @file RHIDeviceConformanceTests.cpp
 * @brief Runs the portable device contract on the linked backend.
 */

#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Memory/RHIHeapDesc.hpp>
#include <RHI/Queries/RHIQueryPoolDesc.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {

        LUMEN_TEST( RHIDeviceConformance, DevicePublishesNegotiatedContract )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            LUMEN_CHECK_TRUE( ( *Device )->GetLimits().Validate().HasValue() );
            LUMEN_CHECK_TRUE( ( *Device )->GetCapabilities().Validate().HasValue() );
            LUMEN_CHECK_TRUE( ( *Device )->GetId().IsValid() );
            LUMEN_CHECK_TRUE( ( *Device )->WaitIdle().HasValue() );
        }

        LUMEN_TEST( RHIDeviceConformance, InvalidFactoriesReturnStructuredErrors )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );

            const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer = ( *Device )->CreateBuffer( {} );
            LUMEN_CHECK_HAS_ERROR( Buffer );
            LUMEN_CHECK_EQ( Buffer.Error().Code, ERHIErrorCode::InvalidArgument );
            LUMEN_CHECK_EQ( Buffer.Error().Condition, ERHIErrorCondition::Resource );

            const TExpected<TSharedRef<IRHIHeap>, FRHIError> Heap = ( *Device )->CreateHeap( {} );
            LUMEN_CHECK_HAS_ERROR( Heap );
            LUMEN_CHECK_EQ( Heap.Error().Code, ERHIErrorCode::InvalidArgument );
            LUMEN_CHECK_EQ( Heap.Error().Condition, ERHIErrorCondition::Memory );

            const TExpected<TSharedRef<IRHIUploadAllocator>, FRHIError> Upload = ( *Device )->CreateUploadAllocator( 0ULL );
            LUMEN_CHECK_HAS_ERROR( Upload );
            LUMEN_CHECK_EQ( Upload.Error().Code, ERHIErrorCode::InvalidArgument );
            LUMEN_CHECK_EQ( Upload.Error().Condition, ERHIErrorCondition::Memory );

            const TExpected<TSharedRef<IRHICommandAllocator>, FRHIError> Commands =
                ( *Device )->CreateCommandAllocator( ERHIQueueType::Count );
            LUMEN_CHECK_HAS_ERROR( Commands );
            LUMEN_CHECK_EQ( Commands.Error().Code, ERHIErrorCode::InvalidArgument );
            LUMEN_CHECK_EQ( Commands.Error().Condition, ERHIErrorCondition::Command );

            FRHIQueryPoolDesc QueryDesc;
            QueryDesc.Count                                             = 0U;
            const TExpected<TSharedRef<IRHIQueryPool>, FRHIError> Query = ( *Device )->CreateQueryPool( QueryDesc );
            LUMEN_CHECK_HAS_ERROR( Query );
            LUMEN_CHECK_EQ( Query.Error().Code, ERHIErrorCode::InvalidArgument );
            LUMEN_CHECK_EQ( Query.Error().Condition, ERHIErrorCondition::Query );
        }

        LUMEN_TEST( RHIDeviceConformance, DeviceLossBlocksCreation )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
            LUMEN_CHECK_TRUE( Device.HasValue() );
            if ( not Tests::GetRHIConformanceDeviceFactory().InjectDeviceLost( **Device ) )
            {
                return;
            }
            LUMEN_CHECK_EQ( ( *Device )->GetStatus(), ERHIDeviceStatus::Lost );

            const TExpected<TSharedRef<IRHIBuffer>, FRHIError> Buffer =
                ( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource } );
            LUMEN_CHECK_HAS_ERROR( Buffer );
            LUMEN_CHECK_EQ( Buffer.Error().Code, ERHIErrorCode::DeviceLost );
            LUMEN_CHECK_EQ( Buffer.Error().Condition, ERHIErrorCondition::Device );
        }

    } // namespace
} // namespace RHI

} // namespace Lumen
