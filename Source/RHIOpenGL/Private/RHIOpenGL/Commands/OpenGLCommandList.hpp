/**
 * @file OpenGLCommandList.hpp
 * @brief Declares the OpenGL command list and its portable recording state.
 */

#pragma once

#include <Core/NonCopyable.hpp>
#include <Core/NonMovable.hpp>

#include <Memory/SharedPtr.hpp>

#include <Templates/Optional.hpp>

#include <RHI/Commands/IRHICommandList.hpp>

#include <RHIOpenGL/Commands/OpenGLCommandAllocator.hpp>
#include <RHIOpenGL/Device/OpenGLDeviceContext.hpp>
#include <RHIOpenGL/Types/OpenGLObject.hpp>

namespace Lumen
{

namespace RHIOpenGL
{

    /**
     * @class FOpenGLCommandList
     * @brief Records fully validated commands for replay on the GL Executor.
     */
    class FOpenGLCommandList final : public RHI::IRHICommandList, public FNonCopyable, public FNonMovable
    {
    public:

        [[nodiscard]] static TExpected<TSharedRef<FOpenGLCommandList>, RHI::FRHIError>
        Create ( TSharedRef<FOpenGLCommandAllocator> InAllocator,
                 TSharedRef<FOpenGLDeviceContext> InContext,
                 RHI::FRHIObjectId InId ) noexcept;

        FOpenGLCommandList ( TSharedRef<FOpenGLCommandAllocator> InAllocator,
                             TSharedRef<FOpenGLDeviceContext> InContext,
                             RHI::FRHIObjectId InId ) noexcept;

        ~FOpenGLCommandList () noexcept override;

    public:

        LUMEN_RHI_OBJECT_DECLARATIONS;

        [[nodiscard]] RHI::ERHIQueueType GetQueueType () const noexcept override;

        [[nodiscard]] RHI::ERHICommandListState GetState () const noexcept override;

        [[nodiscard]] RHI::FRHITimelineValue GetCompletion () const noexcept override;

        [[nodiscard]] const RHI::FRHICommandStream &GetStream () const noexcept override;

        [[nodiscard]] RHI::FRHIExpected Begin () noexcept override;

        [[nodiscard]] RHI::FRHIExpected End () noexcept override;

        [[nodiscard]] RHI::FRHIExpected Reset () noexcept override;

        [[nodiscard]] RHI::FRHIExpected MarkPending ( RHI::FRHITimelineValue InCompletion ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected MarkCompleted ( RHI::FRHITimelineValue InCompleted ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected Transition ( TSpan<const RHI::FRHITransition> InTransitions ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected CopyBuffer ( TSharedRef<RHI::IRHIBuffer> InSource,
                                                     TSharedRef<RHI::IRHIBuffer> InDestination,
                                                     const RHI::FRHIBufferCopy &InCopy ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected CopyTexture ( TSharedRef<RHI::IRHITexture> InSource,
                                                      TSharedRef<RHI::IRHITexture> InDestination,
                                                      const RHI::FRHITextureCopy &InCopy ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected CopyBufferToTexture ( TSharedRef<RHI::IRHIBuffer> InSource,
                                                              TSharedRef<RHI::IRHITexture> InDestination,
                                                              const RHI::FRHIBufferTextureCopy &InCopy ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected CopyTextureToBuffer ( TSharedRef<RHI::IRHITexture> InSource,
                                                              TSharedRef<RHI::IRHIBuffer> InDestination,
                                                              const RHI::FRHIBufferTextureCopy &InCopy ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected BeginRendering ( const RHI::FRHIRenderingDesc &InDesc ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected EndRendering () noexcept override;

        [[nodiscard]] RHI::FRHIExpected SetPipeline ( TSharedRef<RHI::IRHIPipeline> InPipeline ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected SetBindingSet ( UInt32 InIndex, TSharedRef<RHI::IRHIBindingSet> InSet ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected
        SetInlineConstants ( RHI::ERHIShaderStage InStages, UInt32 InOffset, TSpan<const Byte> InValues ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected
        SetVertexBuffer ( UInt32 InSlot, TSharedRef<RHI::IRHIBuffer> InBuffer, UInt64 InOffset ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected
        SetIndexBuffer ( TSharedRef<RHI::IRHIBuffer> InBuffer, UInt64 InOffset, RHI::ERHIIndexFormat InFormat ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected Draw ( const RHI::FRHIDrawArguments &InArguments ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected DrawIndexed ( const RHI::FRHIDrawIndexedArguments &InArguments ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected DrawIndirect ( TSharedRef<RHI::IRHIBuffer> InArguments,
                                                       const RHI::FRHIIndirectArguments &InDesc ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected Dispatch ( const RHI::FRHIDispatchArguments &InArguments ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected DispatchIndirect ( TSharedRef<RHI::IRHIBuffer> InArguments,
                                                           const RHI::FRHIIndirectArguments &InDesc ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected SetViewport ( const RHI::FRHIViewport &InViewport ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected SetScissor ( const RHI::FRHIRect2D &InScissor ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected SetBlendConstants ( const Float32 ( &InConstants )[4] ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected SetDepthBias ( Float32 InConstant, Float32 InSlope ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected SetStencilReference ( UInt32 InReference ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected BeginQuery ( TSharedRef<RHI::IRHIQueryPool> InPool, UInt32 InIndex ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected EndQuery ( TSharedRef<RHI::IRHIQueryPool> InPool, UInt32 InIndex ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected BeginDebugRegion ( FStringView InName ) noexcept override;

        [[nodiscard]] RHI::FRHIExpected EndDebugRegion () noexcept override;

    private:

        [[nodiscard]] RHI::FRHIExpected RequireRecording ( Bool bAllowRendering ) const noexcept;
        [[nodiscard]] RHI::FRHIExpected RequireRendering () const noexcept;
        [[nodiscard]] RHI::FRHIExpected RequirePipeline ( RHI::ERHIPipelineType InType ) const noexcept;
        [[nodiscard]] static RHI::FRHIExpected UnsupportedCommand ( FStringView InOperation,
                                                                    RHI::ERHIErrorCondition InCondition ) noexcept;

    private:

        RHI::FRHIObjectBase Identity;
        TSharedRef<FOpenGLCommandAllocator> Allocator;
        TSharedRef<FOpenGLDeviceContext> Context;
        RHI::FRHICommandStream Stream;
        RHI::ERHICommandListState State = RHI::ERHICommandListState::Initial;
        RHI::FRHITimelineValue Completion;
        Bool bHasPipeline                       = false;
        RHI::ERHIPipelineType BoundPipelineType = RHI::ERHIPipelineType::Graphics;
        RHI::IRHIQueryPool *ActiveQueryPool     = nullptr;
        UInt32 ActiveQueryIndex                 = 0U;
        UInt32 DebugDepth                       = 0U;
    };

} // namespace RHIOpenGL

} // namespace Lumen
