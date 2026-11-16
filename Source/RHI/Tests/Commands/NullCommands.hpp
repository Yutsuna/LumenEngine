/**
 * @file NullCommands.hpp
 * @brief Declares deterministic Null command allocators and lists.
 */

#pragma once

#include "Types/RHIObjectForward.hpp"
#include <Commands/IRHICommandList.hpp>
#include <Commands/RHICommandAllocatorBase.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

namespace Tests
{

    class FNullQueryPool;

    enum class ERHINullCommandOpcode : UInt32
    {
        Transition = 1U,
        CopyBuffer,
        CopyTexture,
        CopyBufferToTexture,
        CopyTextureToBuffer,
        BeginRendering,
        EndRendering,
        SetPipeline,
        SetBindingSet,
        SetVertexBuffer,
        SetIndexBuffer,
        Draw,
        DrawIndexed,
        DrawIndirect,
        Dispatch,
        DispatchIndirect,
        SetViewport,
        SetScissor,
        SetBlendConstants,
        SetDepthBias,
        SetStencilReference,
        BeginQuery,
        EndQuery,
        BeginDebugRegion,
        EndDebugRegion
    };
    class FNullCommandAllocator final : public FRHICommandAllocatorBase
    {
    public:

        FNullCommandAllocator ( ERHIQueueType InQueueType, FRHIObjectId InId ) noexcept;
        LUMEN_RHI_OBJECT_DECLARATIONS;

    private:

        FRHIObjectBase Identity;
    };
    class FNullCommandList final : public IRHICommandList
    {
    public:

        FNullCommandList ( TSharedPtr<FNullCommandAllocator> InAllocator, FRHIObjectId InId ) noexcept;
        ~FNullCommandList () noexcept override;
        LUMEN_RHI_OBJECT_DECLARATIONS;
        ERHIQueueType GetQueueType () const noexcept override;
        ERHICommandListState GetState () const noexcept override;
        FRHITimelineValue GetCompletion () const noexcept override;
        const FRHICommandStream &GetStream () const noexcept override;
        TExpected<void, FRHIError> Begin () noexcept override;
        TExpected<void, FRHIError> End () noexcept override;
        TExpected<void, FRHIError> Reset () noexcept override;
        TExpected<void, FRHIError> MarkPending ( FRHITimelineValue InCompletion ) noexcept override;
        TExpected<void, FRHIError> MarkCompleted ( FRHITimelineValue InCompleted ) noexcept override;
        TExpected<void, FRHIError> Transition ( TSpan<const FRHITransition> InTransitions ) noexcept override;
        TExpected<void, FRHIError> CopyBuffer ( TSharedRef<IRHIBuffer> InSource,
                                                TSharedRef<IRHIBuffer> InDestination,
                                                const FRHIBufferCopy &InCopy ) noexcept override;
        TExpected<void, FRHIError> CopyTexture ( TSharedRef<IRHITexture> InSource,
                                                 TSharedRef<IRHITexture> InDestination,
                                                 const FRHITextureCopy &InCopy ) noexcept override;
        TExpected<void, FRHIError> CopyBufferToTexture ( TSharedRef<IRHIBuffer> InSource,
                                                         TSharedRef<IRHITexture> InDestination,
                                                         const FRHIBufferTextureCopy &InCopy ) noexcept override;
        TExpected<void, FRHIError> CopyTextureToBuffer ( TSharedRef<IRHITexture> InSource,
                                                         TSharedRef<IRHIBuffer> InDestination,
                                                         const FRHIBufferTextureCopy &InCopy ) noexcept override;
        TExpected<void, FRHIError> BeginRendering ( const FRHIRenderingDesc &InDesc ) noexcept override;
        TExpected<void, FRHIError> EndRendering () noexcept override;
        TExpected<void, FRHIError> SetPipeline ( TSharedRef<IRHIPipeline> InPipeline ) noexcept override;
        TExpected<void, FRHIError> SetBindingSet ( UInt32 InIndex, TSharedRef<IRHIBindingSet> InSet ) noexcept override;
        TExpected<void, FRHIError>
        SetVertexBuffer ( UInt32 InSlot, TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset ) noexcept override;
        TExpected<void, FRHIError>
        SetIndexBuffer ( TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset, ERHIIndexFormat InFormat ) noexcept override;
        TExpected<void, FRHIError> Draw ( const FRHIDrawArguments &InArguments ) noexcept override;
        TExpected<void, FRHIError> DrawIndexed ( const FRHIDrawIndexedArguments &InArguments ) noexcept override;
        TExpected<void, FRHIError> DrawIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                  const FRHIIndirectArguments &InDesc ) noexcept override;
        TExpected<void, FRHIError> Dispatch ( const FRHIDispatchArguments &InArguments ) noexcept override;
        TExpected<void, FRHIError> DispatchIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                                      const FRHIIndirectArguments &InDesc ) noexcept override;
        TExpected<void, FRHIError> SetViewport ( const FRHIViewport &InViewport ) noexcept override;
        TExpected<void, FRHIError> SetScissor ( const FRHIRect2D &InScissor ) noexcept override;
        TExpected<void, FRHIError> SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept override;
        TExpected<void, FRHIError> SetDepthBias ( Float32 InConstant, Float32 InSlope ) noexcept override;
        TExpected<void, FRHIError> SetStencilReference ( UInt32 InReference ) noexcept override;
        TExpected<void, FRHIError> BeginQuery ( IRHIQueryPool &InPool, UInt32 InIndex ) noexcept override;
        TExpected<void, FRHIError> EndQuery ( IRHIQueryPool &InPool, UInt32 InIndex ) noexcept override;
        TExpected<void, FRHIError> BeginDebugRegion ( FStringView InName ) noexcept override;
        TExpected<void, FRHIError> EndDebugRegion () noexcept override;

    private:

        TExpected<void, FRHIError> CheckRecording ( Bool bAllowRendering ) const noexcept;
        TExpected<void, FRHIError> CheckRendering () const noexcept;

    private:

        FRHIObjectBase Identity;
        TSharedPtr<FNullCommandAllocator> Allocator;
        ERHICommandListState State = ERHICommandListState::Initial;
        FRHITimelineValue Completion;
        FRHICommandStream Stream;
        TOptional<ERHIPipelineType> BoundPipeline;
        FNullQueryPool *ActiveQueryPool = nullptr;
        UInt32 ActiveQueryIndex         = 0U;
        UInt32 DebugDepth               = 0U;
    };

} // namespace Tests

} // namespace Lumen
