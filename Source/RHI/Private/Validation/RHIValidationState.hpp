/**
 * @file RHIValidationState.hpp
 * @brief Declares shared object ownership and subresource-state tracking.
 */

#pragma once

#include "RHIValidationReporter.hpp"
#include <Commands/RHITransition.hpp>
#include <HAL/PlatformMutex.hpp>
#include <Memory/Span.hpp>
#include <Resources/IRHIBuffer.hpp>
#include <Resources/IRHITexture.hpp>
#include <Templates/Hash.hpp>

#define LUMEN_RHI_VALIDATION_OBJECT_FORWARDERS( Class )                                                                          \
    Lumen::FRHIObjectId Lumen::Class::GetId() const noexcept                                                                     \
    {                                                                                                                            \
        return Underlying->GetId();                                                                                              \
    }                                                                                                                            \
    Lumen::FString Lumen::Class::GetDebugName() const noexcept                                                                   \
    {                                                                                                                            \
        return Underlying->GetDebugName();                                                                                       \
    }                                                                                                                            \
    void Lumen::Class::SetDebugName( const FStringView InName ) noexcept                                                         \
    {                                                                                                                            \
        Underlying->SetDebugName( InName );                                                                                      \
    }                                                                                                                            \
    Lumen::UInt64 Lumen::Class::GetBackendTypeTag() const noexcept                                                               \
    {                                                                                                                            \
        return FHash::TypeHash<Class>();                                                                                         \
    }                                                                                                                            \
    Lumen::IRHIObject &Lumen::Class::GetValidationWrappedObject() const noexcept                                                 \
    {                                                                                                                            \
        return *Underlying.Get();                                                                                                \
    }                                                                                                                            \
    Lumen::TSharedRef<Lumen::IRHIObject> Lumen::Class::GetValidationWrappedShared() const noexcept                               \
    {                                                                                                                            \
        return StaticCastSharedRef<IRHIObject>( Underlying );                                                                    \
    }

namespace Lumen
{

/**
 * @class IRHIValidationObject
 * @brief Private cross-cast interface implemented by every decorator.
 */
class IRHIValidationObject
{
public:

    virtual ~IRHIValidationObject () noexcept = default;
    /** @return The immediately wrapped object, which may itself be a decorator. */
    [[nodiscard]] virtual IRHIObject &GetValidationWrappedObject () const noexcept = 0;
    /** @return Shared ownership of the immediately wrapped object. */
    [[nodiscard]] virtual TSharedRef<IRHIObject> GetValidationWrappedShared () const noexcept = 0;
};

/** @enum ERHIValidationObjectKind @brief Object categories retained by the live registry. */
enum class ERHIValidationObjectKind : UInt8
{
    Device,
    Buffer,
    Texture,
    TextureView,
    Sampler,
    BindingLayout,
    BindingSet,
    CommandAllocator,
    CommandList,
    Queue,
    Swapchain,
    TransientAllocator,
    Defragmenter
};

/**
 * @class FRHIValidationState
 * @brief Thread-safe registry shared by every decorator of one device tree.
 */
class RHI_EXPORT FRHIValidationState final
{
public:

    explicit FRHIValidationState ( TSharedPtr<FRHIValidationReporter> InReporter ) noexcept;
    ~FRHIValidationState () noexcept;

    void Track ( IRHIObject &InObject, ERHIValidationObjectKind InKind ) noexcept;
    void TrackResource ( IRHIResource &InResource, ERHIValidationObjectKind InKind ) noexcept;
    void Untrack ( IRHIObject &InObject ) noexcept;
    /** @return Success when the object belongs to this device tree and has the expected category. */
    [[nodiscard]] TExpected<void, FRHIError> ValidateObject ( const IRHIObject &InObject,
                                                              ERHIValidationObjectKind InKind ) const noexcept;
    /** @return True when the unwrapped object is currently tracked. */
    [[nodiscard]] Bool Contains ( const IRHIObject &InObject ) const noexcept;
    [[nodiscard]] USize GetLiveObjectCount () const noexcept;
    /** @return The state of a buffer, or of a texture when all its subresources agree. */
    [[nodiscard]] TOptional<ERHIResourceState> GetResourceState ( const IRHIResource &InResource ) const noexcept;
    /** @return The state of one texture subresource. */
    [[nodiscard]] TOptional<ERHIResourceState>
    GetTextureState ( const IRHITexture &InTexture, UInt32 InMip, UInt32 InLayer ) const noexcept;
    /** @brief Validates and applies one buffer transition, including queue ownership transfer. */
    [[nodiscard]] TExpected<void, FRHIError>
    TransitionBuffer ( const FRHIBufferTransition &InTransition, ERHIQueueType InRecordingQueue, Bool bApply = true ) noexcept;
    /** @brief Validates and applies a transition independently to every selected subresource. */
    [[nodiscard]] TExpected<void, FRHIError>
    TransitionTexture ( const FRHITextureTransition &InTransition, ERHIQueueType InRecordingQueue, Bool bApply = true ) noexcept;
    /**
     * @brief Validates an entire batch against a speculative copy of the tracked state.
     * @details Each entry observes the state produced by the preceding entries, so a rejection is
     *          produced here rather than after the backend has recorded the batch. No registry is changed.
     * @return Success, or the first structured error raised while replaying the batch.
     */
    [[nodiscard]] TExpected<void, FRHIError> ValidateTransitions ( TSpan<const FRHITransition> InTransitions,
                                                                   ERHIQueueType InRecordingQueue ) const noexcept;
    /** @brief Commits a previously validated batch; this operation cannot reject or report an error. */
    void CommitTransitions ( TSpan<const FRHITransition> InTransitions ) noexcept;
    /** @brief Emits one snapshot of objects that remain live at validation shutdown. */
    void ReportLeaks () noexcept;
    /** @return The shared diagnostic reporter. */
    [[nodiscard]] const TSharedPtr<FRHIValidationReporter> &GetReporter () const noexcept;

private:

    struct FEntry
    {
        IRHIObject *Object            = nullptr;
        ERHIValidationObjectKind Kind = ERHIValidationObjectKind::Device;
        TVector<ERHIResourceState> States;
        ERHIQueueType OwnerQueue = ERHIQueueType::Graphics;
    };

    [[nodiscard]] TExpected<void, FRHIError> ValidateTransition ( const IRHIResource &InResource,
                                                                  const FEntry &InEntry,
                                                                  ERHIResourceState InBefore,
                                                                  ERHIResourceState InAfter,
                                                                  ERHIQueueType InSourceQueue,
                                                                  ERHIQueueType InDestinationQueue,
                                                                  ERHIQueueType InRecordingQueue ) const noexcept;
    /** @return Success when the object belongs to the supplied registry and has the expected category. */
    [[nodiscard]] TExpected<void, FRHIError> CheckObject ( const TMap<UInt64, FEntry> &InObjects,
                                                           const IRHIObject &InObject,
                                                           ERHIValidationObjectKind InKind ) const noexcept;
    /** @brief Replays one buffer barrier against a registry, optionally writing the resulting state. */
    [[nodiscard]] TExpected<void, FRHIError> ValidateBufferIn ( TMap<UInt64, FEntry> &InObjects,
                                                                const FRHIBufferTransition &InTransition,
                                                                ERHIQueueType InRecordingQueue,
                                                                Bool bApply ) const noexcept;
    /** @brief Replays one texture barrier against a registry, optionally writing the resulting states. */
    [[nodiscard]] TExpected<void, FRHIError> ValidateTextureIn ( TMap<UInt64, FEntry> &InObjects,
                                                                 const FRHITextureTransition &InTransition,
                                                                 ERHIQueueType InRecordingQueue,
                                                                 Bool bApply ) const noexcept;
    /** @brief Writes a validated buffer barrier result without any further checks. */
    void ApplyBuffer ( TMap<UInt64, FEntry> &InObjects, const FRHIBufferTransition &InTransition ) const noexcept;
    /** @brief Writes a validated texture barrier result without any further checks. */
    void ApplyTexture ( TMap<UInt64, FEntry> &InObjects, const FRHITextureTransition &InTransition ) const noexcept;

private:

    TSharedPtr<FRHIValidationReporter> Reporter;
    TMap<UInt64, FEntry> Objects;
    mutable HAL::FPlatformSharedMutex Mutex;
    Bool bLeakReported = false;
};

} // namespace Lumen
