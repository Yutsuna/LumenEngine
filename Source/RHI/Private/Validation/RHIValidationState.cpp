/**
 * @file RHIValidationState.cpp
 * @brief Implements shared validation registries and subresource transitions.
 */

#include "RHIValidationState.hpp"
#include <HAL/LockGuard.hpp>
#include <RHI/Validation/RHIValidation.hpp>
#include <variant>

Lumen::RHI::FRHIValidationState::FRHIValidationState ( TSharedPtr<FRHIValidationReporter> InReporter ) noexcept
    : Reporter( std::move( InReporter ) )
{
}

Lumen::RHI::FRHIValidationState::~FRHIValidationState () noexcept = default;

void Lumen::RHI::FRHIValidationState::Track ( IRHIObject &InObject, const ERHIValidationObjectKind InKind ) noexcept
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

void Lumen::RHI::FRHIValidationState::TrackResource ( IRHIResource &InResource, const ERHIValidationObjectKind InKind ) noexcept
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

void Lumen::RHI::FRHIValidationState::Untrack ( IRHIObject &InObject ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformSharedMutex> Lock( Mutex );
    const TMap<UInt64, FEntry>::iterator Found = Objects.find( UnwrapRHIObject( &InObject )->GetId().GetValue() );
    if ( Found != Objects.end() and Found->second.Object == UnwrapRHIObject( &InObject ) )
    {
        Objects.erase( Found );
    }
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationState::ValidateObject ( const IRHIObject &InObject,
                                                  const ERHIValidationObjectKind InKind ) const noexcept
{
    HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    return CheckObject( Objects, InObject, InKind );
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIValidationState::CheckObject (
    const TMap<UInt64, FEntry> &InObjects, const IRHIObject &InObject, const ERHIValidationObjectKind InKind ) const noexcept
{
    const IRHIObject *Unwrapped                      = UnwrapRHIObject( &InObject );
    const TMap<UInt64, FEntry>::const_iterator Found = InObjects.find( Unwrapped->GetId().GetValue() );
    if ( Found == InObjects.end() or Found->second.Object != Unwrapped )
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

Lumen::Bool Lumen::RHI::FRHIValidationState::Contains ( const IRHIObject &InObject ) const noexcept
{
    const IRHIObject *Unwrapped = UnwrapRHIObject( &InObject );
    HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    const TMap<UInt64, FEntry>::const_iterator Found = Objects.find( Unwrapped->GetId().GetValue() );
    return Found != Objects.end() and Found->second.Object == Unwrapped;
}

Lumen::USize Lumen::RHI::FRHIValidationState::GetLiveObjectCount () const noexcept
{
    HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( Mutex );
    return Objects.size();
}

Lumen::TOptional<Lumen::RHI::ERHIResourceState>
Lumen::RHI::FRHIValidationState::GetResourceState ( const IRHIResource &InResource ) const noexcept
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

Lumen::TOptional<Lumen::RHI::ERHIResourceState> Lumen::RHI::FRHIValidationState::GetTextureState (
    const IRHITexture &InTexture, const UInt32 InMip, const UInt32 InLayer ) const noexcept
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

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationState::ValidateTransition ( const IRHIResource &InResource,
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

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIValidationState::TransitionBuffer (
    const FRHIBufferTransition &InTransition, const ERHIQueueType InRecordingQueue, const Bool bApply ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformSharedMutex> Lock( Mutex );
    return ValidateBufferIn( Objects, InTransition, InRecordingQueue, bApply );
}

Lumen::TExpected<void, Lumen::RHI::FRHIError> Lumen::RHI::FRHIValidationState::TransitionTexture (
    const FRHITextureTransition &InTransition, const ERHIQueueType InRecordingQueue, const Bool bApply ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformSharedMutex> Lock( Mutex );
    return ValidateTextureIn( Objects, InTransition, InRecordingQueue, bApply );
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationState::ValidateBufferIn ( TMap<UInt64, FEntry> &InObjects,
                                                    const FRHIBufferTransition &InTransition,
                                                    const ERHIQueueType InRecordingQueue,
                                                    const Bool bApply ) const noexcept
{
    if ( not InTransition.Buffer )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Resource,
                                 "Buffer transition has no resource" );
    }
    IRHIBuffer &Buffer                     = *UnwrapRHI( InTransition.Buffer.Get() );
    const TExpected<void, FRHIError> Owned = CheckObject( InObjects, Buffer, ERHIValidationObjectKind::Buffer );
    if ( not Owned )
    {
        return Owned;
    }
    const TMap<UInt64, FEntry>::iterator Found = InObjects.find( Buffer.GetId().GetValue() );
    FEntry &Entry                              = Found->second;
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
        ApplyBuffer( InObjects, InTransition );
    }
    return {};
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationState::ValidateTextureIn ( TMap<UInt64, FEntry> &InObjects,
                                                     const FRHITextureTransition &InTransition,
                                                     const ERHIQueueType InRecordingQueue,
                                                     const Bool bApply ) const noexcept
{
    if ( not InTransition.Texture )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Resource,
                                 "Texture transition has no resource" );
    }
    IRHITexture &Texture                   = *UnwrapRHI( InTransition.Texture.Get() );
    const TExpected<void, FRHIError> Owned = CheckObject( InObjects, Texture, ERHIValidationObjectKind::Texture );
    if ( not Owned )
    {
        return Owned;
    }
    if ( not InTransition.Range.IsContainedIn( Texture.GetDesc().MipCount, Texture.GetDesc().LayerCount ) )
    {
        return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Resource,
                                 "Texture transition range is out of bounds", &Texture );
    }
    const TMap<UInt64, FEntry>::iterator Found = InObjects.find( Texture.GetId().GetValue() );
    FEntry &Entry                              = Found->second;
    const TExpected<void, FRHIError> TransitionValid =
        ValidateTransition( Texture, Entry, InTransition.Before, InTransition.After, InTransition.SourceQueue,
                            InTransition.DestinationQueue, InRecordingQueue );
    if ( not TransitionValid )
    {
        return TransitionValid;
    }
    const USize Stride = static_cast<USize>( Texture.GetDesc().LayerCount );
    for ( UInt32 Mip = InTransition.Range.BaseMip; Mip < InTransition.Range.BaseMip + InTransition.Range.MipCount; ++Mip )
    {
        for ( UInt32 Layer = InTransition.Range.BaseLayer; Layer < InTransition.Range.BaseLayer + InTransition.Range.LayerCount;
              ++Layer )
        {
            const USize Index = static_cast<USize>( Mip ) * Stride + Layer;
            if ( InTransition.Before != ERHIResourceState::Undefined and Entry.States[Index] != InTransition.Before )
            {
                return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Resource,
                                         "Texture transition does not match every selected subresource", &Texture );
            }
        }
    }
    if ( bApply )
    {
        ApplyTexture( InObjects, InTransition );
    }
    return {};
}

void Lumen::RHI::FRHIValidationState::ApplyBuffer ( TMap<UInt64, FEntry> &InObjects,
                                                    const FRHIBufferTransition &InTransition ) const noexcept
{
    if ( not InTransition.Buffer )
    {
        return;
    }
    IRHIBuffer &Buffer                         = *UnwrapRHI( InTransition.Buffer.Get() );
    const TMap<UInt64, FEntry>::iterator Found = InObjects.find( Buffer.GetId().GetValue() );
    if ( Found != InObjects.end() and not Found->second.States.empty() )
    {
        Found->second.States.front() = InTransition.After;
        Found->second.OwnerQueue     = InTransition.DestinationQueue;
    }
}

void Lumen::RHI::FRHIValidationState::ApplyTexture ( TMap<UInt64, FEntry> &InObjects,
                                                     const FRHITextureTransition &InTransition ) const noexcept
{
    if ( not InTransition.Texture )
    {
        return;
    }
    IRHITexture &Texture                       = *UnwrapRHI( InTransition.Texture.Get() );
    const TMap<UInt64, FEntry>::iterator Found = InObjects.find( Texture.GetId().GetValue() );
    if ( Found == InObjects.end() )
    {
        return;
    }
    FEntry &Entry      = Found->second;
    const USize Stride = static_cast<USize>( Texture.GetDesc().LayerCount );
    for ( UInt32 Mip = InTransition.Range.BaseMip; Mip < InTransition.Range.BaseMip + InTransition.Range.MipCount; ++Mip )
    {
        for ( UInt32 Layer = InTransition.Range.BaseLayer; Layer < InTransition.Range.BaseLayer + InTransition.Range.LayerCount;
              ++Layer )
        {
            const USize Index = static_cast<USize>( Mip ) * Stride + Layer;
            if ( Index < Entry.States.size() )
            {
                Entry.States[Index] = InTransition.After;
            }
        }
    }
    Entry.OwnerQueue = InTransition.DestinationQueue;
}

Lumen::TExpected<void, Lumen::RHI::FRHIError>
Lumen::RHI::FRHIValidationState::ValidateTransitions ( const TSpan<const FRHITransition> InTransitions,
                                                       const ERHIQueueType InRecordingQueue ) const noexcept
{
    TMap<UInt64, FEntry> Speculative;
    {
        HAL::TSharedLock<HAL::FPlatformSharedMutex> Lock( Mutex );
        Speculative = Objects;
    }
    for ( const FRHITransition &Transition : InTransitions )
    {
        if ( const FRHIBufferTransition *Buffer = std::get_if<FRHIBufferTransition>( &Transition ) )
        {
            const TExpected<void, FRHIError> Valid = ValidateBufferIn( Speculative, *Buffer, InRecordingQueue, true );
            if ( not Valid )
            {
                return Valid;
            }
        }
        else if ( const FRHITextureTransition *Texture = std::get_if<FRHITextureTransition>( &Transition ) )
        {
            const TExpected<void, FRHIError> Valid = ValidateTextureIn( Speculative, *Texture, InRecordingQueue, true );
            if ( not Valid )
            {
                return Valid;
            }
        }
        else
        {
            return Reporter->Reject( ERHIValidationMessageCode::InvalidResourceTransition, ERHIErrorCondition::Resource,
                                     "Transition has no active payload" );
        }
    }
    return {};
}

void Lumen::RHI::FRHIValidationState::CommitTransitions ( const TSpan<const FRHITransition> InTransitions ) noexcept
{
    HAL::TLockGuard<HAL::FPlatformSharedMutex> Lock( Mutex );
    for ( const FRHITransition &Transition : InTransitions )
    {
        if ( const FRHIBufferTransition *Buffer = std::get_if<FRHIBufferTransition>( &Transition ) )
        {
            ApplyBuffer( Objects, *Buffer );
        }
        else if ( const FRHITextureTransition *Texture = std::get_if<FRHITextureTransition>( &Transition ) )
        {
            ApplyTexture( Objects, *Texture );
        }
    }
}

void Lumen::RHI::FRHIValidationState::ReportLeaks () noexcept
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

const Lumen::TSharedPtr<Lumen::RHI::FRHIValidationReporter> &Lumen::RHI::FRHIValidationState::GetReporter () const noexcept
{
    return Reporter;
}
