/**
 * @file RHIDeviceBase.cpp
 * @brief Implements common device status and non-owning child tracking.
 */

#include <HAL/LockGuard.hpp>
#include <RHI/Device/RHIDeviceBase.hpp>
#include <Templates/Hash.hpp>

/**
 * Public
 */

Lumen::RHI::FRHIDeviceBase::FRHIDeviceBase ( FRHICapabilities InCapabilities,
                                             const FRHILimits InLimits,
                                             const UInt64 InBackendTypeTag ) noexcept
    : Identity( FRHIObjectId{ 1ULL }, InBackendTypeTag ), Capabilities( std::move( InCapabilities ) ), Limits( InLimits ),
      Status( ERHIDeviceStatus::Ready ), Registry( MakeShared<FObjectRegistry>() )
{
    /* */
}

Lumen::RHI::FRHIDeviceBase::~FRHIDeviceBase () noexcept
{
    SetStatus( ERHIDeviceStatus::Closed );
}

Lumen::RHI::FRHIObjectId Lumen::RHI::FRHIDeviceBase::GetId () const noexcept
{
    return Identity.GetId();
}

Lumen::FString Lumen::RHI::FRHIDeviceBase::GetDebugName () const noexcept
{
    return Identity.GetDebugName();
}

void Lumen::RHI::FRHIDeviceBase::SetDebugName ( const FStringView InName ) noexcept
{
    Identity.SetDebugName( InName );
}

Lumen::UInt64 Lumen::RHI::FRHIDeviceBase::GetBackendTypeTag () const noexcept
{
    return Identity.GetBackendTypeTag();
}

const Lumen::RHI::FRHICapabilities &Lumen::RHI::FRHIDeviceBase::GetCapabilities () const noexcept
{
    return Capabilities;
}

const Lumen::RHI::FRHILimits &Lumen::RHI::FRHIDeviceBase::GetLimits () const noexcept
{
    return Limits;
}

Lumen::RHI::ERHIDeviceStatus Lumen::RHI::FRHIDeviceBase::GetStatus () const noexcept
{
    return Status.load( std::memory_order_acquire );
}

Lumen::USize Lumen::RHI::FRHIDeviceBase::GetTrackedObjectCount () const noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Registry->Mutex );
    return Registry->Objects.size();
}

void Lumen::RHI::FRHIDeviceBase::SetStatus ( const ERHIDeviceStatus InStatus ) noexcept
{
    ERHIDeviceStatus Current = Status.load( std::memory_order_acquire );
    while ( static_cast<UInt8>( InStatus ) > static_cast<UInt8>( Current ) and
            not Status.compare_exchange_weak( Current, InStatus, std::memory_order_acq_rel ) )
    {
        /* */
    }
}

Lumen::RHI::FRHIObjectId Lumen::RHI::FRHIDeviceBase::NextObjectId () noexcept
{
    return Identifiers.Next<FRHIObjectIdTag>();
}

void Lumen::RHI::FRHIDeviceBase::UntrackObject ( IRHIObject &InObject ) noexcept
{
    const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Registry->Mutex );
    for ( USize Index = 0U; Index < Registry->Objects.size(); ++Index )
    {
        if ( Registry->Objects[Index] == &InObject )
        {
            Registry->Objects[Index] = Registry->Objects.back();
            Registry->Objects.pop_back();
            return;
        }
    }
}

Lumen::TOptional<Lumen::RHI::FRHIError>
Lumen::RHI::FRHIDeviceBase::GetUnavailableError ( const FStringView InOperation ) const noexcept
{
    const ERHIDeviceStatus Current = GetStatus();
    if ( Current == ERHIDeviceStatus::Ready )
    {
        return {};
    }
    const ERHIErrorCode Code = Current == ERHIDeviceStatus::Lost ? ERHIErrorCode::DeviceLost : ERHIErrorCode::DeviceClosed;
    return TOptional<FRHIError>( FRHIError( Code, ERHIErrorCondition::Device, InOperation ) );
}

/**
 * Private
 */

void Lumen::RHI::FRHIDeviceBase::TrackObject ( IRHIObject &InObject, FRHIObjectBase &InBase ) noexcept
{
    {
        const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Registry->Mutex );
        Registry->Objects.emplace_back( &InObject );
    }
    InBase.SetDestructionCallback( [Registry = Registry, &InObject]
    {
        const HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( Registry->Mutex );
        for ( USize Index = 0U; Index < Registry->Objects.size(); ++Index )
        {
            if ( Registry->Objects[Index] == &InObject )
            {
                Registry->Objects[Index] = Registry->Objects.back();
                Registry->Objects.pop_back();
                return;
            }
        }
    } );
}
