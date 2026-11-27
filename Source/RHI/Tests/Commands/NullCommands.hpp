/**
 * @file NullCommands.hpp
 * @brief Declares deterministic Null command allocators and lists.
 */

#pragma once

#include <RHI/Commands/IRHICommandList.hpp>
#include <RHI/Commands/RHICommandAllocatorBase.hpp>
#include <RHI/Types/RHIObjectForward.hpp>
#include <Templates/Optional.hpp>

namespace Lumen
{

namespace RHI
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
            SetInlineConstants,
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
            FRHIExpected Begin () noexcept override;
            FRHIExpected End () noexcept override;
            FRHIExpected Reset () noexcept override;
            FRHIExpected MarkPending ( FRHITimelineValue InCompletion ) noexcept override;
            FRHIExpected MarkCompleted ( FRHITimelineValue InCompleted ) noexcept override;
            FRHIExpected Transition ( TSpan<const FRHITransition> InTransitions ) noexcept override;
            FRHIExpected CopyBuffer ( TSharedRef<IRHIBuffer> InSource,
                                      TSharedRef<IRHIBuffer> InDestination,
                                      const FRHIBufferCopy &InCopy ) noexcept override;
            FRHIExpected CopyTexture ( TSharedRef<IRHITexture> InSource,
                                       TSharedRef<IRHITexture> InDestination,
                                       const FRHITextureCopy &InCopy ) noexcept override;
            FRHIExpected CopyBufferToTexture ( TSharedRef<IRHIBuffer> InSource,
                                               TSharedRef<IRHITexture> InDestination,
                                               const FRHIBufferTextureCopy &InCopy ) noexcept override;
            FRHIExpected CopyTextureToBuffer ( TSharedRef<IRHITexture> InSource,
                                               TSharedRef<IRHIBuffer> InDestination,
                                               const FRHIBufferTextureCopy &InCopy ) noexcept override;
            FRHIExpected BeginRendering ( const FRHIRenderingDesc &InDesc ) noexcept override;
            FRHIExpected EndRendering () noexcept override;
            FRHIExpected SetPipeline ( TSharedRef<IRHIPipeline> InPipeline ) noexcept override;
            FRHIExpected SetBindingSet ( UInt32 InIndex, TSharedRef<IRHIBindingSet> InSet ) noexcept override;
            FRHIExpected
            SetInlineConstants ( ERHIShaderStage InStages, UInt32 InOffset, TSpan<const Byte> InValues ) noexcept override;
            FRHIExpected SetVertexBuffer ( UInt32 InSlot, TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset ) noexcept override;
            FRHIExpected
            SetIndexBuffer ( TSharedRef<IRHIBuffer> InBuffer, UInt64 InOffset, ERHIIndexFormat InFormat ) noexcept override;
            FRHIExpected Draw ( const FRHIDrawArguments &InArguments ) noexcept override;
            FRHIExpected DrawIndexed ( const FRHIDrawIndexedArguments &InArguments ) noexcept override;
            FRHIExpected DrawIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                        const FRHIIndirectArguments &InDesc ) noexcept override;
            FRHIExpected Dispatch ( const FRHIDispatchArguments &InArguments ) noexcept override;
            FRHIExpected DispatchIndirect ( TSharedRef<IRHIBuffer> InArguments,
                                            const FRHIIndirectArguments &InDesc ) noexcept override;
            FRHIExpected SetViewport ( const FRHIViewport &InViewport ) noexcept override;
            FRHIExpected SetScissor ( const FRHIRect2D &InScissor ) noexcept override;
            FRHIExpected SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept override;
            FRHIExpected SetDepthBias ( Float32 InConstant, Float32 InSlope ) noexcept override;
            FRHIExpected SetStencilReference ( UInt32 InReference ) noexcept override;
            FRHIExpected BeginQuery ( TSharedRef<IRHIQueryPool> InPool, UInt32 InIndex ) noexcept override;
            FRHIExpected EndQuery ( TSharedRef<IRHIQueryPool> InPool, UInt32 InIndex ) noexcept override;
            FRHIExpected BeginDebugRegion ( FStringView InName ) noexcept override;
            FRHIExpected EndDebugRegion () noexcept override;

        private:

            FRHIExpected CheckRecording ( Bool bAllowRendering ) const noexcept;
            FRHIExpected CheckRendering () const noexcept;

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

} // namespace RHI

} // namespace Lumen
