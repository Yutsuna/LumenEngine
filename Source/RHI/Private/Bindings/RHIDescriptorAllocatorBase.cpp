/**
 * @file RHIDescriptorAllocatorBase.cpp
 * @brief Implements descriptor free lists, pool growth, and range recycling.
 */

#include <Bindings/RHIDescriptorAllocatorBase.hpp>

#include <Algo/ForEach.hpp>
#include <HAL/LockGuard.hpp>

Lumen::FRHIDescriptorAllocatorBase::FRHIDescriptorAllocatorBase ( const FRHIDescriptorAllocatorDesc &InDesc,
                                                                  IRHIDescriptorPoolBackend &InBackend ) noexcept
    : Desc( InDesc ), Backend( &InBackend )
{
    Backend->CreatePool( InDesc.InitialCapacity )
        .Inspect( [this, &InDesc] ( const UInt64 InPool ) noexcept
    {
        Pools.push_back(
            FPool{ .BaseOffset = 0U, .BackendPool = InPool, .Indices = MakeUnique<FIndiceAllocator>( InDesc.InitialCapacity ) } );
    } );
}

Lumen::FRHIDescriptorAllocatorBase::~FRHIDescriptorAllocatorBase () noexcept
{
    ReleasePools();
}

Lumen::TExpected<Lumen::FRHIDescriptorAllocation, Lumen::FRHIError>
Lumen::FRHIDescriptorAllocatorBase::Allocate ( const UInt32 InCount ) noexcept
{
    if ( InCount == 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Descriptor count is zero" ) );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );

    /** NOTE: Algo::ForEachUntil does not fit here — the allocation is both probe and result. */
    for ( FPool &Pool : Pools )
    {
        const TOptional<FIndiceAllocation> Allocation = Pool.Indices->Allocate( InCount );
        if ( Allocation )
        {
            const FRHIDescriptorAllocation Result = { .Offset = Pool.BaseOffset + Allocation->Start, .Count = Allocation->Count };
            Active.push_back( Result );
            return Result;
        }
    }

    UInt32 BaseOffset = 0U;
    if ( not Pools.empty() )
    {
        const FPool &Last = Pools.back();
        BaseOffset        = Last.BaseOffset + Last.Indices->GetCapacity();
    }
    if ( BaseOffset >= Desc.MaximumCapacity or InCount > Desc.MaximumCapacity - BaseOffset )
    {
        return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Binding, "Descriptor allocator exhausted" ) );
    }

    const UInt32 Remaining                         = Desc.MaximumCapacity - BaseOffset;
    const UInt32 Requested                         = Desc.InitialCapacity < InCount ? InCount : Desc.InitialCapacity;
    const UInt32 Capacity                          = Remaining < Requested ? Remaining : Requested;
    const TExpected<UInt64, FRHIError> BackendPool = Backend->CreatePool( Capacity );

    if ( not BackendPool )
    {
        return TUnexpected<FRHIError>( BackendPool.Error() );
    }
    Pools.push_back(
        FPool{ .BaseOffset = BaseOffset, .BackendPool = *BackendPool, .Indices = MakeUnique<FIndiceAllocator>( Capacity ) } );
    const TOptional<FIndiceAllocation> Allocation = Pools.back().Indices->Allocate( InCount );
    if ( not Allocation )
    {
        return TUnexpected<FRHIError>( MakeRHIOutOfMemory( ERHIErrorCondition::Binding, "Descriptor pool growth failed" ) );
    }
    const FRHIDescriptorAllocation Result{ .Offset = BaseOffset + Allocation->Start, .Count = Allocation->Count };
    Active.push_back( Result );
    return Result;
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIDescriptorAllocatorBase::Free ( const FRHIDescriptorAllocation InAllocation ) noexcept
{
    if ( InAllocation.Count == 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Invalid descriptor range" ) );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Bool bActive = false;
    for ( USize Index = 0U; Index < Active.size(); ++Index )
    {
        if ( Active[Index].Offset == InAllocation.Offset and Active[Index].Count == InAllocation.Count )
        {
            Active.erase( Active.begin() + static_cast<ISize>( Index ) );
            bActive = true;
            break;
        }
    }
    if ( not bActive )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Descriptor range is not active" ) );
    }
    for ( FPool &Pool : Pools )
    {
        if ( InAllocation.Offset < Pool.BaseOffset )
        {
            continue;
        }
        const UInt32 LocalOffset = InAllocation.Offset - Pool.BaseOffset;
        const UInt32 Capacity    = Pool.Indices->GetCapacity();
        if ( LocalOffset < Capacity and InAllocation.Count <= Capacity - LocalOffset )
        {
            Pool.Indices->Free( FIndiceAllocation( LocalOffset, InAllocation.Count ) );
            return {};
        }
    }
    return TUnexpected<FRHIError>( MakeRHIInvalidBinding( "Descriptor pool is not present" ) );
}

void Lumen::FRHIDescriptorAllocatorBase::Reset () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    ReleasePools();
    Pools.clear();
    Active.clear();
    const TExpected<UInt64, FRHIError> Pool = Backend->CreatePool( Desc.InitialCapacity );

    if ( Pool )
    {
        Pools.push_back(
            FPool{ .BaseOffset = 0U, .BackendPool = *Pool, .Indices = MakeUnique<FIndiceAllocator>( Desc.InitialCapacity ) } );
    }
}

void Lumen::FRHIDescriptorAllocatorBase::ReleasePools () noexcept
{
    Algo::ForEach( Pools, [&] ( const FPool &Pool ) noexcept -> void
    {
        Backend->ReleasePool( Pool.BackendPool );
    } );
}
