/**
 * @file RenderGraphTypes.hpp
 * @brief Value contracts shared by render graph declaration and compilation.
 */

#pragma once

#include <RendererExport.hpp>

#include <Core/CoreTypes.hpp>

#include <Memory/SharedPtr.hpp>
#include <RHI/Commands/RHITransition.hpp>
#include <RHI/Memory/RHIResourceLifetime.hpp>
#include <RHI/Memory/RHIResourceRequirements.hpp>
#include <RHI/Resources/IRHIBuffer.hpp>
#include <RHI/Resources/IRHITexture.hpp>
#include <RHI/Resources/RHIBufferDesc.hpp>
#include <RHI/Resources/RHITextureDesc.hpp>
#include <RHI/Types/RHIQueueType.hpp>
#include <RHI/Types/RHIResourceState.hpp>
#include <Templates/Expected.hpp>
#include <Templates/Optional.hpp>
#include <Templates/Variant.hpp>
#include <Templates/Vector.hpp>

namespace Lumen
{

inline constexpr UInt32 InvalidRenderPassOrdinal       = 0xFFFFFFFFU;
inline constexpr UInt32 InvalidRenderResourceIndex     = 0xFFFFFFFFU;
inline constexpr UInt32 InvalidRenderQueueSegmentIndex = 0xFFFFFFFFU;

/** @struct FRenderPassHandle
 * @brief Stable declaration ordinal of one pass in a graph.
 */
struct FRenderPassHandle
{
    UInt32 Ordinal = InvalidRenderPassOrdinal;

    [[nodiscard]] constexpr Bool IsValid () const noexcept
    {
        return Ordinal != InvalidRenderPassOrdinal;
    }

    friend constexpr auto operator<=>( const FRenderPassHandle &, const FRenderPassHandle & ) = default;
};

/** @struct FRenderResourceHandle
 * @brief Graph-scoped index and generation identifying one declared resource.
 */
struct FRenderResourceHandle
{
    UInt32 Index      = InvalidRenderResourceIndex;
    UInt32 Generation = 0U;

    [[nodiscard]] constexpr Bool IsValid () const noexcept
    {
        return Index != InvalidRenderResourceIndex and Generation != 0U;
    }

    friend constexpr auto operator<=>( const FRenderResourceHandle &, const FRenderResourceHandle & ) = default;
};

/** @struct FRenderGraphCapacities
 * @brief Hard capacities reserved once when declaration starts.
 */
struct FRenderGraphCapacities
{
    USize PassCount              = 0U;
    USize ResourceCount          = 0U;
    USize ReadCountPerPass       = 0U;
    USize WriteCountPerPass      = 0U;
    USize DependencyCountPerPass = 0U;
    USize TransitionCount        = 0U;
    USize QueueDependencyCount   = 0U;
};

/** @struct FRenderPassDesc
 * @brief Execution properties declared before accesses are appended.
 */
struct FRenderPassDesc
{
    RHI::ERHIQueueType Queue = RHI::ERHIQueueType::Graphics;
    Bool bHasSideEffects     = false;
};

/** @struct FRenderResourceAccess
 * @brief One whole-resource read or write in a requested RHI state.
 */
struct FRenderResourceAccess
{
    FRenderResourceHandle Resource;
    RHI::ERHIResourceState State = RHI::ERHIResourceState::Undefined;
};

/** @struct FRenderPass
 * @brief Immutable compiled view of a pass and its declaration-order adjacency.
 */
struct FRenderPass
{
    FRenderPassDesc Desc;
    UInt32 Ordinal = InvalidRenderPassOrdinal;
    TVector<FRenderResourceAccess> Reads;
    TVector<FRenderResourceAccess> Writes;
    TVector<UInt32> Dependencies;
    TVector<UInt32> Dependents;
    UInt32 TopologicalPosition = InvalidRenderPassOrdinal;
    Bool bActive               = false;
};

/** @struct FRenderResourceDesc
 * @brief Backend-neutral creation description and memory requirements of a transient.
 */
struct FRenderResourceDesc
{
    TVariant<RHI::FRHIBufferDesc, RHI::FRHITextureDesc> Creation;
    RHI::FRHIResourceRequirements Requirements;
};

/** @struct FRenderImportedResourceDesc
 * @brief Ownership states at graph entry and exit for an imported resource.
 */
struct FRenderImportedResourceDesc
{
    RHI::ERHIResourceState InitialState = RHI::ERHIResourceState::Undefined;
    RHI::ERHIResourceState FinalState   = RHI::ERHIResourceState::Undefined;
    RHI::ERHIQueueType InitialQueue     = RHI::ERHIQueueType::Graphics;
    RHI::ERHIQueueType FinalQueue       = RHI::ERHIQueueType::Graphics;
};

/** @struct FRenderResourceBinding
 * @brief Strong RHI possession front for an import or a materialized transient.
 */
struct FRenderResourceBinding
{
    TVariant<TSharedPtr<RHI::IRHIBuffer>, TSharedPtr<RHI::IRHITexture>> Resource;
    RHI::ERHIResourceState InitialState = RHI::ERHIResourceState::Undefined;
    RHI::ERHIResourceState FinalState   = RHI::ERHIResourceState::Undefined;
    RHI::ERHIQueueType InitialQueue     = RHI::ERHIQueueType::Graphics;
    RHI::ERHIQueueType FinalQueue       = RHI::ERHIQueueType::Count;
};

/** @struct FRenderResource
 * @brief Compiled resource metadata, half-open lifetime, and optional import possession.
 */
struct FRenderResource
{
    FRenderResourceHandle Handle;
    FRenderResourceDesc Desc;
    FRenderResourceBinding Binding;
    RHI::FRHISubresourceRange Range;
    RHI::FRHIResourceLifetime Lifetime;
    UInt32 TransientRequestIndex = InvalidRenderResourceIndex;
    Bool bImported               = false;
    Bool bExported               = false;
    Bool bActive                 = false;
};

/** @enum ERenderTransitionRecordPoint
 * @brief Position of a barrier relative to its recording pass.
 */
enum class ERenderTransitionRecordPoint : UInt8
{
    BeforePass,
    AfterPass,
};

/** @struct FRenderTransition
 * @brief Canonical barrier metadata; imports also carry their materialized RHI descriptor.
 */
struct FRenderTransition
{
    FRenderResourceHandle Resource;
    RHI::FRHISubresourceRange Range;
    RHI::ERHIResourceState Before            = RHI::ERHIResourceState::Undefined;
    RHI::ERHIResourceState After             = RHI::ERHIResourceState::Undefined;
    RHI::ERHIQueueType SourceQueue           = RHI::ERHIQueueType::Graphics;
    RHI::ERHIQueueType DestinationQueue      = RHI::ERHIQueueType::Graphics;
    UInt32 SourcePassOrdinal                 = InvalidRenderPassOrdinal;
    UInt32 DestinationPassOrdinal            = InvalidRenderPassOrdinal;
    UInt32 RecordPassOrdinal                 = InvalidRenderPassOrdinal;
    UInt32 SegmentIndex                      = InvalidRenderQueueSegmentIndex;
    ERenderTransitionRecordPoint RecordPoint = ERenderTransitionRecordPoint::BeforePass;
    TOptional<RHI::FRHITransition> RHITransition;

    [[nodiscard]] constexpr Bool IsQueueTransfer () const noexcept
    {
        return SourceQueue != DestinationQueue;
    }
};

/** @struct FRenderQueueSegment
 * @brief Maximal topological range recorded into one command list for one queue.
 */
struct FRenderQueueSegment
{
    RHI::ERHIQueueType Queue = RHI::ERHIQueueType::Graphics;
    UInt32 BeginPass         = 0U;
    UInt32 EndPass           = 0U;
    UInt32 FirstDependency   = 0U;
    UInt32 DependencyCount   = 0U;
    UInt64 SignalValue       = 0ULL;
};

/** @struct FRenderQueueDependency
 * @brief Timeline-ready edge between two queue segments.
 */
struct FRenderQueueDependency
{
    UInt32 SourceSegmentIndex      = InvalidRenderQueueSegmentIndex;
    UInt32 DestinationSegmentIndex = InvalidRenderQueueSegmentIndex;
    UInt64 WaitValue               = 0ULL;
};

/** @enum ERenderErrorCode
 * @brief Stable render graph declaration, compilation, and binding failures.
 */
enum class ERenderErrorCode : UInt16
{
    CapacityExceeded,
    InvalidResourceHandle,
    InvalidPassHandle,
    InvalidResourceDescriptor,
    InvalidAccess,
    AmbiguousWrite,
    DependencyCycle,
    ReadBeforeWrite,
    ExportNotProduced,
    InvalidState,
    InvalidQueue,
    InvalidInitialQueue,
    InvalidTransition,
    UnsupportedQueueTransfer,
    InvalidTransientBatch,
    ResourceTypeMismatch,
    ResourceCulled,
    TransientResourceNotBound,
    TransitionOutOfRange,
    GraphAlreadyCompiled,
};

/** @struct FRenderError
 * @brief Non-allocating structured error with stable graph coordinates.
 */
struct FRenderError
{
    ERenderErrorCode Code     = ERenderErrorCode::InvalidAccess;
    UInt32 PassOrdinal        = InvalidRenderPassOrdinal;
    UInt32 ResourceIndex      = InvalidRenderResourceIndex;
    UInt32 RelatedPassOrdinal = InvalidRenderPassOrdinal;
};

using FRenderExpected = TExpected<void, FRenderError>;

} // namespace Lumen
