/**
 * @file RHIValidationConformanceTests.cpp
 * @brief Replays portable conformance scenarios with validation enabled.
 */
#include "Validation/RHIValidationTestUtils.hpp"
#include <Bindings/IRHIBindingSet.hpp>
#include <Commands/IRHICommandAllocator.hpp>
#include <Commands/IRHICommandList.hpp>
#include <Device/RHIConformanceDeviceFactory.hpp>
#include <Queries/IRHIQueryPool.hpp>
#include <Queue/IRHIQueue.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{
namespace
{
    LUMEN_TEST( RHIValidationConformance, CopiesSubmitsAndRecyclesWithValidation )
    {
        auto DeviceResult = Tests::GetRHIConformanceDeviceFactory().CreateDevice( true );
        LUMEN_CHECK_TRUE( DeviceResult.HasValue() );
        auto Device      = *DeviceResult;
        auto Source      = Tests::Validation::MakeBuffer( Device, 64ULL, ERHIResourceUsage::CopySource );
        auto Destination = Tests::Validation::MakeBuffer( Device, 64ULL, ERHIResourceUsage::CopyDestination );
        auto Allocator   = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
        auto List        = *Device->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->CopyBuffer( Source, Destination, { .Size = 64ULL } ).HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        auto Queue = *Device->GetQueue( ERHIQueueType::Graphics, 0U );
        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Completion   = { 1ULL };
        LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
        LUMEN_CHECK_TRUE( Queue->Wait( { 1ULL }, 0.0 ).HasValue() );
        LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Executable );
        LUMEN_CHECK_TRUE( Allocator->Reset( Queue->GetCompletedValue() ).HasValue() );
        LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Initial );
    }

    LUMEN_TEST( RHIValidationConformance, PreservesBindingAndQueryResults )
    {
        auto Device = *Tests::GetRHIConformanceDeviceFactory().CreateDevice( true );
        FRHIBindingLayoutDesc LayoutDesc;
        LayoutDesc.Entries.push_back( { .Slot = 0U, .Type = ERHIBindingType::ReadOnlyBuffer } );
        auto Layout = *Device->CreateBindingLayout( LayoutDesc );
        auto Set    = *Device->CreateBindingSet( Layout );
        auto Buffer = Tests::Validation::MakeBuffer( Device );
        FRHIBindingWrite Write;
        Write.Resource = FRHIBufferBinding{ .Buffer = Buffer };
        LUMEN_CHECK_TRUE( Set->Write( TSpan<const FRHIBindingWrite>( &Write, 1U ) ).HasValue() );
        FRHIQueryPoolDesc QueryDesc;
        QueryDesc.Count = 2U;
        auto Pool       = *Device->CreateQueryPool( QueryDesc );
        auto Allocator  = *Device->CreateCommandAllocator( ERHIQueueType::Graphics );
        auto List       = *Device->CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->BeginQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndQuery( *Pool, 0U ).HasValue() );
        LUMEN_CHECK_TRUE( List->BeginQuery( *Pool, 1U ).HasValue() );
        LUMEN_CHECK_TRUE( List->EndQuery( *Pool, 1U ).HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        auto Queue = *Device->GetQueue( ERHIQueueType::Graphics, 0U );
        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Completion   = { 1ULL };
        LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
        TArray<UInt64, 2U> Results = {};
        LUMEN_CHECK_TRUE( Pool->ReadResults( 0U, 2U, Results, Queue->GetCompletedValue() ).HasValue() );
        LUMEN_CHECK_EQ( Results[0], 1ULL );
        LUMEN_CHECK_EQ( Results[1], 2ULL );
    }
} // namespace
} // namespace Lumen
