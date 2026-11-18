/**
 * @file RHICommandAllocatorBaseTests.cpp
 * @brief Tests pending-safe command-list recycling.
 */

#include "Device/NullDevice.hpp"
#include <Commands/IRHICommandAllocator.hpp>
#include <Commands/IRHICommandList.hpp>
#include <Tests/UnitTest.hpp>

namespace Lumen
{

namespace
{

    LUMEN_TEST( RHICommandAllocatorBase, RefusesPendingResetAndRecyclesAfterCompletion )
    {
        Tests::FNullDevice Device;
        TSharedRef<IRHICommandAllocator> Allocator = *Device.CreateCommandAllocator( ERHIQueueType::Graphics );
        TSharedRef<IRHICommandList> List           = *Device.CreateCommandList( Allocator );
        LUMEN_CHECK_TRUE( List->Begin().HasValue() );
        LUMEN_CHECK_TRUE( List->End().HasValue() );
        LUMEN_CHECK_TRUE( List->MarkPending( { 4ULL } ).HasValue() );
        LUMEN_CHECK_HAS_ERROR( Allocator->Reset( { 3ULL } ) );
        LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Pending );
        LUMEN_CHECK_TRUE( Allocator->Reset( { 4ULL } ).HasValue() );
        LUMEN_CHECK_EQ( List->GetState(), ERHICommandListState::Initial );
        LUMEN_CHECK_EQ( List->GetStream().GetCommandCount(), 0U );
    }

} // namespace

} // namespace Lumen
