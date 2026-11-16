/**
 * @file RHICommandsConformanceTests.cpp
 * @brief Runs portable record and submit scenarios.
 */

#include "Device/RHIConformanceDeviceFactory.hpp"
#include <Commands/IRHICommandAllocator.hpp>
#include <Commands/IRHICommandList.hpp>
#include <Device/IRHIDevice.hpp>
#include <Queue/IRHIQueue.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    LUMEN_TEST( RHICommandsConformance, RecordsCopiesAndSubmitsExecutableList )
    {
        auto Device = Tests::GetRHIConformanceDeviceFactory().CreateDevice();
        LUMEN_CHECK_TRUE( Device.HasValue() );
        auto Allocator   = *( *Device )->CreateCommandAllocator( ERHIQueueType::Graphics );
        auto List        = *( *Device )->CreateCommandList( Allocator );
        auto Source      = *( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopySource } );
        auto Destination = *( *Device )->CreateBuffer( { .Size = 64ULL, .Usage = ERHIResourceUsage::CopyDestination } );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->CopyBuffer( Source, Destination, { .Size = 64ULL } ).HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        auto Queue = *( *Device )->GetQueue( ERHIQueueType::Graphics, 0U );
        FRHISubmitDesc Submit;
        Submit.CommandLists = { List };
        Submit.Completion   = { 1ULL };
        LUMEN_CHECK_TRUE( Queue->Submit( Submit ).HasValue() );
        LUMEN_CHECK_TRUE( Queue->Wait( { 1ULL }, 0.0 ).HasValue() );
        LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Executable );
        LUMEN_CHECK_TRUE( Allocator->Reset( Queue->GetCompletedValue() ).HasValue() );
    }

} // namespace

} // namespace Lumen
