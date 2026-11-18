/**
 * @file RHIMemoryObserverTests.cpp
 * @brief Tests allocation lifecycle notifications from allocation and defragmentation.
 */

#include <Device/NullDevice.hpp>
#include <Memory/IRHIDefragmenter.hpp>
#include <Memory/RHILinearAllocator.hpp>
#include <Memory/RHIMemoryAllocator.hpp>
#include <Tests/UnitTest.hpp>
#include <Types/RHITimeline.hpp>

namespace Lumen
{
namespace
{

    enum class EAllocationEvent : UInt8
    {
        Created,
        Released,
        Moved,
    };

    class FRecordingAllocationObserver final : public IRHIAllocationObserver
    {
    public:

        void OnAllocationCreated ( const FRHIAllocation &InAllocation ) noexcept override
        {
            Events.push_back( EAllocationEvent::Created );
            LastAllocation = InAllocation;
        }

        void OnAllocationReleased ( const FRHIAllocation &InAllocation ) noexcept override
        {
            Events.push_back( EAllocationEvent::Released );
            LastAllocation = InAllocation;
        }

        void OnAllocationMoved ( const FRHIAllocation &InBefore, const FRHIAllocation &InAfter ) noexcept override
        {
            Events.push_back( EAllocationEvent::Moved );
            LastSource     = InBefore;
            LastAllocation = InAfter;
        }

        [[nodiscard]] const TVector<EAllocationEvent> &GetEvents () const noexcept
        {
            return Events;
        }

        [[nodiscard]] const FRHIAllocation &GetLastSource () const noexcept
        {
            return LastSource;
        }

        [[nodiscard]] const FRHIAllocation &GetLastAllocation () const noexcept
        {
            return LastAllocation;
        }

    private:

        TVector<EAllocationEvent> Events;
        FRHIAllocation LastSource;
        FRHIAllocation LastAllocation;
    };

    class FScopedObserverRegistration final
    {
    public:

        FScopedObserverRegistration ( FRHIMemoryAllocator &InFacade, IRHIAllocationObserver &InObserver ) noexcept
            : Facade( InFacade ), Observer( InObserver )
        {
            Facade.RegisterObserver( Observer );
        }

        ~FScopedObserverRegistration () noexcept
        {
            Facade.UnregisterObserver( Observer );
        }

        FScopedObserverRegistration ( const FScopedObserverRegistration & )           = delete;
        FScopedObserverRegistration &operator=( const FScopedObserverRegistration & ) = delete;

    private:

        FRHIMemoryAllocator &Facade;
        IRHIAllocationObserver &Observer;
    };

} // namespace

LUMEN_TEST( RHIAllocationObserver, ReceivesCreatedThenReleasedInOrder )
{
    FRHILinearAllocator Allocator( 128ULL );
    FRHIMemoryBudget Budget( 128ULL );
    FRHIMemoryAllocator Facade( Allocator, Budget );
    FRecordingAllocationObserver Observer;
    FScopedObserverRegistration Registration( Facade, Observer );

    const TExpected<FRHIAllocation, FRHIError> Allocation = Facade.Allocate( { 32ULL, 16ULL } );
    LUMEN_CHECK_TRUE( Allocation.HasValue() );
    LUMEN_CHECK_EQ( Observer.GetEvents().size(), 1U );
    LUMEN_CHECK_TRUE( Observer.GetEvents()[0U] == EAllocationEvent::Created );
    LUMEN_CHECK_EQ( Observer.GetLastAllocation().Size, 32ULL );

    LUMEN_CHECK_TRUE( Facade.Free( *Allocation ).HasValue() );
    LUMEN_CHECK_EQ( Observer.GetEvents().size(), 2U );
    LUMEN_CHECK_TRUE( Observer.GetEvents()[0U] == EAllocationEvent::Created );
    LUMEN_CHECK_TRUE( Observer.GetEvents()[1U] == EAllocationEvent::Released );
    LUMEN_CHECK_EQ( Observer.GetLastAllocation().Size, 32ULL );
}

LUMEN_TEST( RHIAllocationObserver, IgnoresFailedAllocationAndRelease )
{
    FRHILinearAllocator Allocator( 64ULL );
    FRHIMemoryBudget Budget( 32ULL );
    FRHIMemoryAllocator Facade( Allocator, Budget );
    FRecordingAllocationObserver Observer;
    FScopedObserverRegistration Registration( Facade, Observer );

    LUMEN_CHECK_HAS_ERROR( Facade.Allocate( { 64ULL, 1ULL } ) );
    LUMEN_CHECK_TRUE( Observer.GetEvents().empty() );

    const TExpected<FRHIAllocation, FRHIError> Allocation = Facade.Allocate( { 16ULL, 1ULL } );
    LUMEN_CHECK_TRUE( Allocation.HasValue() );
    LUMEN_CHECK_EQ( Observer.GetEvents().size(), 1U );

    FRHIAllocation Invalid = *Allocation;
    Invalid.Size           = 0ULL;
    LUMEN_CHECK_HAS_ERROR( Facade.Free( Invalid ) );
    LUMEN_CHECK_EQ( Observer.GetEvents().size(), 1U );
}

LUMEN_TEST( RHIAllocationObserver, ReceivesMovedForExecutedDefragMove )
{
    FRHILinearAllocator Allocator( 128ULL );
    FRHIMemoryBudget Budget( 128ULL );
    FRHIMemoryAllocator Facade( Allocator, Budget );
    FRecordingAllocationObserver Observer;
    FScopedObserverRegistration Registration( Facade, Observer );

    Tests::FNullDevice Device;
    const TSharedRef<IRHIDefragmenter> Defragmenter = *Device.CreateDefragmenter();
    const FRHIAllocation First{ .Id = FRHIAllocationId{ 11ULL }, .Offset = 0ULL, .Size = 64ULL, .Alignment = 16ULL };
    const FRHIAllocation Second{ .Id = FRHIAllocationId{ 12ULL }, .Offset = 128ULL, .Size = 64ULL, .Alignment = 16ULL };
    const FRHIAllocation LiveAllocations[] = { First, Second };

    const TExpected<FRHIDefragPlan, FRHIError> PlanResult = Defragmenter->BuildPlan( LiveAllocations );
    LUMEN_CHECK_TRUE( PlanResult.HasValue() );
    FRHIDefragPlan Plan = *PlanResult;
    LUMEN_CHECK_FALSE( Plan.Moves.empty() );

    LUMEN_CHECK_HAS_ERROR( Defragmenter->Execute( Plan ) );
    LUMEN_CHECK_TRUE( Observer.GetEvents().empty() );

    Plan.ObservedCompletion = FRHITimelineValue{ 1ULL };
    LUMEN_CHECK_TRUE( Defragmenter->Execute( Plan ).HasValue() );
    LUMEN_CHECK_EQ( Observer.GetEvents().size(), Plan.Moves.size() );
    for ( const EAllocationEvent Event : Observer.GetEvents() )
    {
        LUMEN_CHECK_TRUE( Event == EAllocationEvent::Moved );
    }
    LUMEN_CHECK_EQ( Observer.GetLastSource().Offset, Second.Offset );
    LUMEN_CHECK_EQ( Observer.GetLastAllocation().Offset, 64ULL );
}

} // namespace Lumen
