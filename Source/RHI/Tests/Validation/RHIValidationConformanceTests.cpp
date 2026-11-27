/**
 * @file RHIValidationConformanceTests.cpp
 * @brief Replays portable conformance scenarios with validation enabled.
 */
#include <Device/RHIConformanceDeviceFactory.hpp>
#include <RHI/Bindings/IRHIBindingSet.hpp>
#include <RHI/Commands/IRHICommandAllocator.hpp>
#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Device/IRHIDevice.hpp>
#include <RHI/Queries/IRHIQueryPool.hpp>
#include <RHI/Queue/IRHIQueue.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace RHI
{
    namespace
    {
        LUMEN_TEST( RHIValidationConformance, CopiesSubmitsAndRecyclesWithValidation )
        {
            TExpected<TSharedRef<IRHIDevice>, FRHIError> DeviceResult =
                Tests::GetRHIConformanceDeviceFactory().CreateDevice( true );
            LUMEN_CHECK_TRUE( DeviceResult.HasValue() );
            TSharedRef<IRHIDevice> Device = *DeviceResult;
            TSharedRef<IRHIBuffer> Source = *Device->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource } );
            TSharedRef<IRHIBuffer> Destination =
                *Device->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopyDestination } );
            TSharedRef<IRHICommandAllocator> Allocator = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *Device->CreateCommandList( Allocator );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            LUMEN_CHECK_TRUE( List->CopyBuffer( Source, Destination, { .Size = 64ULL } ).HasValue() );
            LUMEN_CHECK_TRUE( List->End().HasValue() );
            TSharedRef<IRHIQueue> Queue = *Device->GetQueue( ERHIQueueType::Graphics, 0U );
            FRHISubmitDesc Submit;
            Submit.CommandLists = { List };
            Submit.Completion   = { 1ULL };
            LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
            LUMEN_CHECK_TRUE( Queue->Wait( { 1ULL }, 1.0 ).HasValue() );
            LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Executable );
            LUMEN_CHECK_TRUE( Allocator->Reset( Queue->GetCompletedValue() ).HasValue() );
            LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Initial );
        }

        LUMEN_TEST( RHIValidationConformance, PreservesBindingAndQueryResults )
        {
            TSharedRef<IRHIDevice> Device = *Tests::GetRHIConformanceDeviceFactory().CreateDevice( true );
            FRHIBindingLayoutDesc LayoutDesc;
            LayoutDesc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer } );
            TSharedRef<IRHIBindingLayout> Layout = *Device->CreateBindingLayout( LayoutDesc );
            TSharedRef<IRHIBindingSet> Set       = *Device->CreateBindingSet( Layout );
            TSharedRef<IRHIBuffer> Buffer =
                *Device->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::ShaderResource } );
            FRHIBindingWrite Write;
            Write.Resource = FRHIBufferBinding{ .Buffer = Buffer };
            LUMEN_CHECK_TRUE( Set->Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) ).HasValue() );
            FRHIQueryPoolDesc QueryDesc;
            QueryDesc.Count                            = 2U;
            TSharedRef<IRHIQueryPool> Pool             = *Device->CreateQueryPool( QueryDesc );
            TSharedRef<IRHICommandAllocator> Allocator = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
            TSharedRef<IRHICommandList> List           = *Device->CreateCommandList( Allocator );
            LUMEN_CHECK_TRUE( List->Begin().HasValue() );
            LUMEN_CHECK_TRUE( List->BeginQuery( Pool, 0U ).HasValue() );
            LUMEN_CHECK_TRUE( List->EndQuery( Pool, 0U ).HasValue() );
            LUMEN_CHECK_TRUE( List->BeginQuery( Pool, 1U ).HasValue() );
            LUMEN_CHECK_TRUE( List->EndQuery( Pool, 1U ).HasValue() );
            LUMEN_CHECK_TRUE( List->End().HasValue() );
            TSharedRef<IRHIQueue> Queue = *Device->GetQueue( ERHIQueueType::Graphics, 0U );
            FRHISubmitDesc Submit;
            Submit.CommandLists = { List };
            Submit.Completion   = { 1ULL };
            LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
            LUMEN_CHECK_TRUE( Queue->WaitIdle().HasValue() );
            TArray<UInt64, 2U> Results = {};
            LUMEN_CHECK_TRUE( Pool->ReadResults( 0U, 2U, Results, Queue->GetCompletedValue() ).HasValue() );
        }
    } // namespace
} // namespace RHI

} // namespace Lumen
