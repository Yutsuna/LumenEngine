/**
 * @file RHIDeviceBase.cpp
 * @brief Implements common device status and non-owning child tracking.
 */
#include <Device/RHIDeviceBase.hpp>
#include <HAL/LockGuard.hpp>
#include <Templates/Hash.hpp>

Lumen::FRHIDeviceBase::FRHIDeviceBase ( const FRHICapabilities InCapabilities,
                                        const FRHILimits InLimits,
                                        const UInt64 InBackendTypeTag ) noexcept
    : Identity( FRHIObjectId{ 1ULL }, InBackendTypeTag ), Capabilities( InCapabilities ), Limits( InLimits ),
      Status( ERHIDeviceStatus::Ready )
{
}

Lumen::FRHIDeviceBase::~FRHIDeviceBase () noexcept
{
    SetStatus( ERHIDeviceStatus::Closed );
}

Lumen::FRHIObjectId Lumen::FRHIDeviceBase::GetId () const noexcept
{
    return Identity.GetId();
}

Lumen::FString Lumen::FRHIDeviceBase::GetDebugName () const noexcept
{
    return Identity.GetDebugName();
}

void Lumen::FRHIDeviceBase::SetDebugName ( const FStringView InName ) noexcept
{
    Identity.SetDebugName( InName );
}

Lumen::UInt64 Lumen::FRHIDeviceBase::GetBackendTypeTag () const noexcept
{
    return Identity.GetBackendTypeTag();
}

const Lumen::FRHICapabilities &Lumen::FRHIDeviceBase::GetCapabilities () const noexcept
{
    return Capabilities;
}

const Lumen::FRHILimits &Lumen::FRHIDeviceBase::GetLimits () const noexcept
{
    return Limits;
}

Lumen::ERHIDeviceStatus Lumen::FRHIDeviceBase::GetStatus () const noexcept
{
    return Status.load( std::memory_order_acquire );
}

Lumen::USize Lumen::FRHIDeviceBase::GetTrackedObjectCount () const noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( ObjectsMutex );
    return Objects.size();
}

void Lumen::FRHIDeviceBase::SetStatus ( const ERHIDeviceStatus InStatus ) noexcept
{
    ERHIDeviceStatus Current = Status.load( std::memory_order_acquire );
    while ( static_cast<UInt8>( InStatus ) > static_cast<UInt8>( Current ) and
            not Status.compare_exchange_weak( Current, InStatus, std::memory_order_acq_rel ) )
    {
    }
}

Lumen::FRHIObjectId Lumen::FRHIDeviceBase::NextObjectId () noexcept
{
    return Identifiers.Next<FRHIObjectIdTag>();
}

void Lumen::FRHIDeviceBase::TrackObject ( IRHIObject &InObject ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( ObjectsMutex );
    Objects.emplace_back( &InObject );
}

void Lumen::FRHIDeviceBase::UntrackObject ( IRHIObject &InObject ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformRecursiveSharedMutex> Lock( ObjectsMutex );
    for ( USize Index = 0U; Index < Objects.size(); ++Index )
    {
        if ( Objects[Index] == &InObject )
        {
            Objects[Index] = Objects.back();
            Objects.pop_back();
            return;
        }
    }
}

Lumen::TOptional<Lumen::FRHIError> Lumen::FRHIDeviceBase::GetUnavailableError ( const FStringView InOperation ) const noexcept
{
    const ERHIDeviceStatus Current = GetStatus();
    if ( Current == ERHIDeviceStatus::Ready )
    {
        return {};
    }
    const ERHIErrorCode Code = Current == ERHIDeviceStatus::Lost ? ERHIErrorCode::DeviceLost : ERHIErrorCode::DeviceClosed;
    return TOptional<FRHIError>( FRHIError( Code, ERHIErrorCondition::Device, InOperation ) );
}
