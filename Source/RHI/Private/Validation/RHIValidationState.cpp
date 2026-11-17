/**
 * @file RHIValidationState.cpp
 * @brief Implements shared validation registries and subresource transitions.
 */
#include "RHIValidationState.hpp"
#include <HAL/LockGuard.hpp>
#include <Validation/RHIValidation.hpp>

Lumen::FRHIValidationState::FRHIValidationState ( TSharedPtr<FRHIValidationReporter> InReporter ) noexcept
    : Reporter( std::move( InReporter ) )
{
}

Lumen::FRHIValidationState::~FRHIValidationState () noexcept = default;

void Lumen::FRHIValidationState::Track ( IRHIObject &InObject, const ERHIValidationObjectKind InKind ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformSharedMutex> Lock( Mutex );
    const UInt64 Key = InObject.GetId().GetValue();
    if ( Objects.find( Key ) != Objects.end() )
    {
        return;
    }
    FEntry Entry;
    Entry.Object = UnwrapRHIObject( &InObject );
    Entry.Kind   = InKind;
    Objects.emplace( Key, std::move( Entry ) );
}

void Lumen::FRHIValidationState::TrackResource ( IRHIResource &InResource, const ERHIValidationObjectKind InKind ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformSharedMutex> Lock( Mutex );
    const UInt64 Key = InResource.GetId().GetValue();
    if ( Objects.find( Key ) != Objects.end() )
    {
        return;
    }
    FEntry Entry;
    Entry.Object = UnwrapRHIObject( &InResource );
    Entry.Kind   = InKind;
    if ( InKind == ERHIValidationObjectKind::Texture )
    {
        const auto &Texture = static_cast<const IRHITexture &>( InResource );
        const USize Count   = static_cast<USize>( Texture.GetDesc().MipCount ) * Texture.GetDesc().LayerCount;
        Entry.States.assign( Count, InResource.GetInitialState() );
    }
    else
    {
        Entry.States.push_back( InResource.GetInitialState() );
    }
    Objects.emplace( Key, std::move( Entry ) );
}

void Lumen::FRHIValidationState::Untrack ( IRHIObject &InObject ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformSharedMutex> Lock( Mutex );
    const TMap<UInt64, FEntry>::iterator Found = Objects.find( UnwrapRHIObject( &InObject )->GetId().GetValue() );
    if ( Found != Objects.end() and Found->second.Object == UnwrapRHIObject( &InObject ) )
    {
        Objects.erase( Found );
    }
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationState::ValidateObject ( const IRHIObject &InObject, const ERHIValidationObjectKind InKind ) const noexcept
{
    const IRHIObject *Unwrapped = UnwrapRHIObject( &InObject );
    HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    const TMap<UInt64, FEntry>::const_iterator Found = Objects.find( Unwrapped->GetId().GetValue() );
    if ( Found == Objects.end() or Found->second.Object != Unwrapped )
    {
        return Reporter->Reject( ERHIValidationMessageCode::ForeignObject, ERHIErrorCondition::Resource,
                                 "Object does not belong to this validation device", &InObject );
    }
    if ( Found->second.Kind != InKind )
    {
        return Reporter->Reject( ERHIValidationMessageCode::UnknownObject, ERHIErrorCondition::Resource,
                                 "Object category does not match the validated operation", &InObject );
    }
    return {};
}

Lumen::Bool Lumen::FRHIValidationState::Contains ( const IRHIObject &InObject ) const noexcept
{
    const IRHIObject *Unwrapped = UnwrapRHIObject( &InObject );
    HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    const TMap<UInt64, FEntry>::const_iterator Found = Objects.find( Unwrapped->GetId().GetValue() );
    return Found != Objects.end() and Found->second.Object == Unwrapped;
}

Lumen::USize Lumen::FRHIValidationState::GetLiveObjectCount () const noexcept
{
    HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    return Objects.size();
}

Lumen::TVector<Lumen::FRHIObjectId> Lumen::FRHIValidationState::GetLiveObjectIds () const noexcept
{
    HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    TVector<FRHIObjectId> Result;
    Result.reserve( Objects.size() );
    for ( const auto &Pair : Objects )
    {
        Result.push_back( FRHIObjectId{ Pair.first } );
    }
    return Result;
}

Lumen::TOptional<Lumen::ERHIResourceState>
Lumen::FRHIValidationState::GetResourceState ( const IRHIResource &InResource ) const noexcept
{
    const IRHIObject *Unwrapped = UnwrapRHIObject( &InResource );
    HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    const TMap<UInt64, FEntry>::const_iterator Found = Objects.find( Unwrapped->GetId().GetValue() );
    if ( Found == Objects.end() or Found->second.States.empty() )
    {
        return {};
    }
    const ERHIResourceState State = Found->second.States.front();
    for ( const ERHIResourceState Candidate : Found->second.States )
    {
        if ( Candidate != State )
        {
            return {};
        }
    }
    return TOptional<ERHIResourceState>( State );
}

Lumen::TOptional<Lumen::ERHIResourceState> Lumen::FRHIValidationState::GetTextureState ( const IRHITexture &InTexture,
                                                                                         const UInt32 InMip,
                                                                                         const UInt32 InLayer ) const noexcept
{
    const IRHIObject *Unwrapped = UnwrapRHIObject( &InTexture );
    HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    const TMap<UInt64, FEntry>::const_iterator Found = Objects.find( Unwrapped->GetId().GetValue() );
    if ( Found == Objects.end() or Found->second.Kind != ERHIValidationObjectKind::Texture or
         InMip >= InTexture.GetDesc().MipCount or InLayer >= InTexture.GetDesc().LayerCount )
    {
        return {};
    }
    const USize Index = static_cast<USize>( InMip ) * InTexture.GetDesc().LayerCount + InLayer;
    return TOptional<ERHIResourceState>( Found->second.States[Index] );
}

Lumen::TExpected<void, Lumen::FRHIError>
Lumen::FRHIValidationState::ValidateTransition ( const IRHIResource &InResource,
                                                 const FEntry &InEntry,
                                                 const ERHIResourceState InBefore,
                                                 const ERHIResourceState InAfter,
                                                 const ERHIQueueType InSourceQueue,
                                                 const ERHIQueueType InDestinationQueue,
                                                 const ERHIQueueType InRecordingQueue ) const noexcept
{
    if ( not IsValidRHIResourceTransition( InBefore, InAfter ) or InAfter == ERHIResourceState::Undefined )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Resource,
                                 "Resource transition has an invalid state pair", &InResource );
    }
    if ( InSourceQueue != InDestinationQueue and ( InEntry.OwnerQueue != InSourceQueue or InRecordingQueue != InSourceQueue ) )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Synchronization,
                                 "Queue transfer originates from the wrong queue", &InResource );
    }
    if ( InSourceQueue == InDestinationQueue and InRecordingQueue != InSourceQueue )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Synchronization,
                                 "Transition queue does not match the recording queue", &InResource );
    }
    return {};
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationState::TransitionBuffer ( const FRHIBufferTransition &InTransition,
                                                                                        const ERHIQueueType InRecordingQueue,
                                                                                        const Bool bApply ) noexcept
{
    if ( not InTransition.Buffer )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Resource,
                                 "Buffer transition has no resource" );
    }
    IRHIBuffer &Buffer                     = *UnwrapRHI( InTransition.Buffer.Get() );
    const TExpected<void, FRHIError> Valid = ValidateObject( Buffer, ERHIValidationObjectKind::Buffer );
    if ( not Valid )
    {
        return Valid;
    }
    HAL::TLockGuard<HAL::FPlatformSharedMutex> Lock( Mutex );
    FEntry &Entry = Objects.find( Buffer.GetId().GetValue() )->second;
    const TExpected<void, FRHIError> TransitionValid =
        ValidateTransition( Buffer, Entry, InTransition.Before, InTransition.After, InTransition.SourceQueue,
                            InTransition.DestinationQueue, InRecordingQueue );
    if ( not TransitionValid )
    {
        return TransitionValid;
    }
    if ( InTransition.Before != ERHIResourceState::Undefined and Entry.States.front() != InTransition.Before )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Resource,
                                 "Buffer transition does not match its current state", &Buffer );
    }
    if ( bApply )
    {
        Entry.States.front() = InTransition.After;
        Entry.OwnerQueue     = InTransition.DestinationQueue;
    }
    return {};
}

Lumen::TExpected<void, Lumen::FRHIError> Lumen::FRHIValidationState::TransitionTexture (
    const FRHITextureTransition &InTransition, const ERHIQueueType InRecordingQueue, const Bool bApply ) noexcept
{
    if ( not InTransition.Texture )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Resource,
                                 "Texture transition has no resource" );
    }
    IRHITexture &Texture                   = *UnwrapRHI( InTransition.Texture.Get() );
    const TExpected<void, FRHIError> Valid = ValidateObject( Texture, ERHIValidationObjectKind::Texture );
    if ( not Valid )
    {
        return Valid;
    }
    if ( not InTransition.Range.IsContainedIn( Texture.GetDesc().MipCount, Texture.GetDesc().LayerCount ) )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Resource,
                                 "Texture transition range is out of bounds", &Texture );
    }
    HAL::TLockGuard<HAL::FPlatformSharedMutex> Lock( Mutex );
    FEntry &Entry = Objects.find( Texture.GetId().GetValue() )->second;
    const TExpected<void, FRHIError> TransitionValid =
        ValidateTransition( Texture, Entry, InTransition.Before, InTransition.After, InTransition.SourceQueue,
                            InTransition.DestinationQueue, InRecordingQueue );
    if ( not TransitionValid )
    {
        return TransitionValid;
    }
    for ( UInt32 Mip = InTransition.Range.BaseMip; Mip < InTransition.Range.BaseMip + InTransition.Range.MipCount; ++Mip )
    {
        for ( UInt32 Layer = InTransition.Range.BaseLayer; Layer < InTransition.Range.BaseLayer + InTransition.Range.LayerCount;
              ++Layer )
        {
            const USize Index = static_cast<USize>( Mip ) * Texture.GetDesc().LayerCount + Layer;
            if ( InTransition.Before != ERHIResourceState::Undefined and Entry.States[Index] != InTransition.Before )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Resource,
                                         "Texture transition does not match every selected subresource", &Texture );
            }
        }
    }
    if ( bApply )
    {
        for ( UInt32 Mip = InTransition.Range.BaseMip; Mip < InTransition.Range.BaseMip + InTransition.Range.MipCount; ++Mip )
        {
            for ( UInt32 Layer = InTransition.Range.BaseLayer;
                  Layer < InTransition.Range.BaseLayer + InTransition.Range.LayerCount; ++Layer )
            {
                const USize Index   = static_cast<USize>( Mip ) * Texture.GetDesc().LayerCount + Layer;
                Entry.States[Index] = InTransition.After;
            }
        }
        Entry.OwnerQueue = InTransition.DestinationQueue;
    }
    return {};
}

void Lumen::FRHIValidationState::ReportLeaks () noexcept
{
    if ( not Reporter->GetDesc().IsEnabled( ERHIValidationFlags::ObjectLifetime ) )
    {
        return;
    }
    TVector<FEntry> Leaked;
    {
        HAL::TLockGuard<HAL::FPlatformSharedMutex> Lock( Mutex );
        if ( bLeakReported )
        {
            return;
        }
        bLeakReported = true;
        for ( const auto &Pair : Objects )
        {
            if ( Pair.second.Kind != ERHIValidationObjectKind::Device )
            {
                Leaked.push_back( Pair.second );
            }
        }
    }
    for ( const FEntry &Entry : Leaked )
    {
        Reporter->Report( ERHIValidationMessageCode::ObjectLeak, "RHI object remains live at validation shutdown", Entry.Object,
                          ERHIDebugSeverity::Warning );
    }
}

const Lumen::TSharedPtr<Lumen::FRHIValidationReporter> &Lumen::FRHIValidationState::GetReporter () const noexcept
{
    return Reporter;
}
