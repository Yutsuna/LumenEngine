/**
 * @file RHIDescriptorAllocatorBase.cpp
 * @brief Implements descriptor free-list allocation.
 */

#include <Bindings/RHIDescriptorAllocatorBase.hpp>
#include <HAL/LockGuard.hpp>
Lumen::FRHIDescriptorAllocatorBase::FRHIDescriptorAllocatorBase ( const FRHIDescriptorAllocatorDesc &InDesc,
                                                                  IRHIDescriptorPoolBackend &InBackend ) noexcept
    : Desc( InDesc ), Indices( MakeUnique<FIndiceAllocator>( InDesc.InitialCapacity ) ), Backend( &InBackend )
{
    const auto Pool = Backend->CreatePool( InDesc.InitialCapacity );
    if ( Pool )
    {
        Pools.push_back( *Pool );
    }
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
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Descriptor count is zero" ) );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    const auto Allocation = Indices->Allocate( InCount );
    if ( not Allocation )
    {
        return TUnexpected<FRHIError>(
            FRHIError( ERHIErrorCode::OutOfMemory, ERHIErrorCondition::Binding, "Descriptor allocator exhausted" ) );
    }
    return FRHIDescriptorAllocation{ Allocation->Start, Allocation->Count };
}
Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIDescriptorAllocatorBase::Free ( const FRHIDescriptorAllocation InAllocation ) noexcept
{
    if ( InAllocation.Count == 0U )
    {
        return TUnexpected<FRHIError>( MakeRHIInvalidArgument( ERHIErrorCondition::Binding, "Invalid descriptor range" ) );
    }
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Indices->Free( FIndiceAllocation( InAllocation.Offset, InAllocation.Count ) );
    return {};
}
void Lumen::FRHIDescriptorAllocatorBase::Reset () noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Mutex );
    Indices = MakeUnique<FIndiceAllocator>( Desc.InitialCapacity );
}
void Lumen::FRHIDescriptorAllocatorBase::ReleasePools () noexcept
{
    for ( const UInt64 Pool : Pools )
    {
        Backend->ReleasePool( Pool );
    }
    Pools.clear();
}
